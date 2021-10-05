/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#include <ismc_p.h>
#include <ismc.h>

/*
 * TODO: Implement map and stream messages
 */

/**
 * Create a message.
 *
 * @param consumer  The consumer object.
 * @return A return code: 0=good.  If non-zero see ismc_getLastError for more error details.
 */
ismc_message_t * ismc_createMessage(ismc_session_t * session, enum msgtype_e msgtype) {
    ismc_message_t * message = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,6),1, sizeof(ismc_message_t));
    message->h.id = OBJID_Message;
    pthread_spin_init(&message->h.lock, 0);
    message->session = session;
    message->msgtype = msgtype;
    message->priority = ismMESSAGE_PRIORITY_DEFAULT;
    message->qos = -1;

	message->body.buf = NULL;
	message->body.len = 0;
	message->body.used = 0;
	message->body.inheap = 0;

    if (msgtype == MTYPE_StreamMessage) {
    	ism_common_allocAllocBuffer(&message->body, 64, 0);
    	message->body.buf[0] = 0x9E;
    	message->body.used = 1;
    } else if (msgtype == MTYPE_MapMessage) {
    	ism_common_allocAllocBuffer(&message->body, 64, 0);
    	message->body.buf[0] = 0x9F;
    	message->body.used = 1;
    }

    return message;
}


/**
 * Create a message with extended properties.
 *
 * @param  session  The session the message is in
 * @param  type     The message type
 * @param  priority The message priority
 * @param  qos      The quality of service.
 * @param  retain   The message retain flag
 * @param  ttl      The message time to live
 */
ismc_message_t * ismc_createMessageX(ismc_session_t * session, enum msgtype_e msgtype,
        int priority, int qos, int retain, uint64_t ttl) {
    ismc_message_t * message = ismc_createMessage(session, msgtype);
    message->priority = (priority > ismMESSAGE_PRIORITY_MINIMUM && priority < ismMESSAGE_PRIORITY_MAXIMUM)
    		? priority
    		: ismMESSAGE_PRIORITY_DEFAULT;
    message->qos    = qos;
    message->retain = !!retain;
    message->ttl = ttl;
    return message;
}

/**
 * Put a map value from properties.
 *
 * A map is represented as a byte array a bytes and a length.  The bytes contain
 * alternating names and values.
 *
 * @param map      The map/message/action to put to
 * @param props    The properties object
 * @param message  The message
 */
void ismc_putJMSValues(ism_actionbuf_t * map, ism_prop_t * props, ismc_message_t * message, const char * topic) {
    int i;
    int savepos = map->used;
    int len;
    const char * name;
    ism_field_t field;
	const char * msgId;
	const char * jmsType;
	const char * corrId;
    int domain;
    const char * destName;

    /* Ensure we have some buffer */
    ism_protocol_ensureBuffer(map, 32);

    map->buf[map->used] = (char)(S_Map+3);
    map->used += 4;

    /* Put the JMS header at the start of the properties */
    if (message->timestamp != 0) {
        ism_protocol_putNameIndex(map, ID_Timestamp);
        ism_protocol_putLongValue(map, ismc_getTimestamp(message));
    }
    if (message->expire != 0) {
        ism_protocol_putNameIndex(map, ID_Expire);
        ism_protocol_putLongValue(map, ismc_getExpiration(message));
    }

    if (message->has & HAS_MSGID) {
	    msgId = ismc_getMessageID(message);
        if (msgId != NULL) {
            ism_protocol_putNameIndex(map, ID_MsgID);
            ism_protocol_putStringValue(map, msgId);
        }
    }

    if (message->has & HAS_CORRID) {
        corrId = ismc_getCorrelationID(message);
        if (corrId != NULL) {
            ism_protocol_putNameIndex(map, ID_CorrID);
            ism_protocol_putStringValue(map, corrId);
        }
        }

    if (message->has & HAS_TYPE) {
        jmsType = ismc_getTypeString(message);
        if (jmsType != NULL) {
            ism_protocol_putNameIndex(map, ID_JMSType);
            ism_protocol_putStringValue(map, jmsType);
        }
    }

    if (message->has & HAS_REPLY) {
        destName = ismc_getReplyTo(message, &domain);
        if (destName != NULL) {
        	ism_protocol_putNameIndex(map, (domain == ismc_Topic)?ID_ReplyToT : ID_ReplyToQ);
            ism_protocol_putStringValue(map, destName);
        }
    }

    /* Put out the original topic name as a property so we know it if we do a wildcard subscribe */
    if (topic && *topic) {
    	ism_protocol_putNameIndex(map, ID_Topic);
    	ism_protocol_putStringValue(map, topic);
    }

    /*
     * If we have a null map an no header fields, put in a null map entry
     */
    if (props == NULL || ism_common_getPropertyIndex(props, 0, &name) != 0) {
        if (map->used == (savepos + 4)) {
            map->buf[savepos] = (char)S_Map;
            map->used = savepos + 1;
            return;
        }
    } else {
        /* Put in all property entries */
        for (i = 0; ism_common_getPropertyIndex(props, i, &name) == 0; i++) {
            /* Do not copy system properties */
            if (name[0]!='J' || name[1]!='M' || name[2]!='S' || name[3] == 'X') {
                ism_common_getProperty(props, name, &field);
                ism_protocol_putNameValue(map, name);
                ism_protocol_putObjectValue(map, &field);
            }
        }
    }

    len = map->used - savepos - 4;
    map->buf[savepos+1] = (char)(len >> 16);
    map->buf[savepos+2] = (char)((len >> 8) & 0xff);
    map->buf[savepos+3] = (char)(len & 0xff);
}


/**
 * Get JMS message properties from the map.
 *
 * A map is represented as a byte array a bytes and a length.  The bytes contain
 * alternating names and values.
 *
 * @param map      The map/message/action to read from
 * @param message  The message
 */
int ismc_getJMSValues(ism_actionbuf_t * map, ismc_message_t * message) {
    ism_actionbuf_t savepos;
    ism_field_t field, obj;
	int maplen;
	unsigned char otype;

    if (map->pos >= map->used) {
        return 0;
    }

    otype = (map->buf[map->pos] & 0xFF);
    if (otype == S_Map) {
        map->pos++;
        return 0;
    }
    if (otype<S_Map || otype>S_Map+4) {
        return ismc_setError(ISMRC_MessageNotValid, "Message properties not valid");
    }

    ism_protocol_getObjectValue(map, &field);
    if (field.type != VT_Map) {
        char buf[256];

        ismc_setError(ISMRC_MessageNotValid, "Field \"%s\" found instead of message properties map\n",
        		ism_protocol_dumpField(&field, buf, sizeof(buf)));

        return ISMRC_MessageNotValid;
    }

    maplen = field.len;
    if ((maplen > 0 && field.val.s == NULL) || (map->pos - map->used > 0)) {
        return ismc_setError(ISMRC_MessageNotValid, "The message data is not valid");
    }

    savepos.buf  = field.val.s;
    savepos.len  = maplen;
    savepos.used = maplen;
    savepos.pos  = 0;

    while (savepos.pos < savepos.used) {
        otype = (savepos.buf[savepos.pos] & 0xFF);

        if (otype == (S_ID + 1)) {
			int which;

        	savepos.pos++;
            which = (savepos.buf[savepos.pos++] & 0xFF);
            ism_protocol_getObjectValue(&savepos, &obj);
            switch (which) {
            case ID_Timestamp:  ismc_setTimestamp(message, obj.val.l);               break;
            case ID_Expire:     ismc_setExpiration(message, obj.val.l);              break;
            case ID_MsgID:      ismc_setMessageID(message, obj.val.s);               break;
            case ID_CorrID:     ismc_setCorrelationID(message, obj.val.s);           break;
            case ID_JMSType:    ismc_setTypeString(message, obj.val.s);              break;
            case ID_ReplyToT:   ismc_setReplyTo(message, obj.val.s, ismc_Topic);     break;
            case ID_ReplyToQ:   ismc_setReplyTo(message, obj.val.s, ismc_Queue);     break;
            case ID_Topic:      ismc_setDestination(message, obj.val.s, ismc_Topic); break;
            }
        } else {
            int rc = ism_protocol_getObjectValue(&savepos, &obj);
            if (!rc && obj.type == VT_Name) {
            	const char * key = obj.val.s;
            	ism_protocol_getObjectValue(&savepos, &obj);
            	ismc_setProperty(message, key, &obj);
            }
        }
    }

    if (message->expire && message->timestamp < message->expire) {
        message->ttl = message->expire - message->timestamp;
    }

    if (savepos.pos != savepos.used) {
        return ismc_setError(ISMRC_PropertiesNotValid, "Message properties length not valid");
    }

    return ISMRC_Good;
}


/**
 * Free the message and release resources associated with it
 * @param  message  The message
 */
void ismc_freeMessage(ismc_message_t * message) {
	if (message) {
	    ism_common_freeAllocBuffer(&message->body);
        ismc_free(message);
	}
}

/**
 * Create a message from the response received from the server.
 * @param session A session.
 * @param action  An action containing server response.
 * @return A new message (malloc)
 */
ismc_message_t * ismc_makeMessage(ismc_consumer_t * consumer, action_t * action) {
	ismc_session_t * session = consumer->session;
    ismc_message_t * message;
    concat_alloc_t   map;
    ism_field_t      pfield;
    actionhdr        hdr;

    if (!consumer) {
    	ismc_setError(ISMRC_NullArgument, "NULL consumer was passed");
    	return NULL;
    }
    if (!action) {
    	ismc_setError(ISMRC_NullArgument, "NULL data passed as a message");
    	return NULL;
    }

    message = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,7),1, sizeof(ismc_message_t));
    message->h.id = OBJID_Message;
    pthread_spin_init(&message->h.lock, 0);
    message->session = session;
	hdr = action->hdr;

    /* Set message properties from the action header */
    message->ack_sqn     = endian_int64(hdr.msgid);
    message->priority    = hdr.priority;
    message->qos         = hdr.flags & ACTFLAG_QoS;
    if (hdr.flags & (ACTFLAG_Retain | ACTFLAG_RetainPublish)) {
        message->retain      = (hdr.flags & ACTFLAG_Retain) ? 3 : 2;
    }
    message->msgtype     = hdr.bodytype;
    message->redelivered = hdr.dup;
    message->order_id    = 0;                 /* Default value */
    message->suspend     = hdr.flags & ACTFLAG_Suspended;

    if (hdr.flags & 0x80) {
    	message->has |= IS_PERSIST;
    } else if (message->qos > 1) {
    	/* Non-persistent messages cannot be delivered with QoS > 1. */
   		message->qos = 1;
    }

    map.len    = 0;
    map.buf    = action->buf.buf;
    map.used   = action->buf.used;
    map.pos    = 0;

    /*
     * If the order ID has been requested, then we should have 1 header field
     * containing the 64-bit order ID.
     */
    if (consumer->requestOrderID) {
    	if ((hdr.hdrcount > 0) &&
    		(ism_protocol_getObjectValue(&map, &pfield) == 0) &&
    		(pfield.type == VT_Long)) {
    		message->order_id = pfield.val.l;
    	} else {
    		ismc_setError(ISMRC_MessageNotValid, "Order ID was requested, but not provided");
    		return NULL;
    	}
    }

    /* Retrieve message header fields and properties */
    if (ismc_getJMSValues(&map, message)) {
        return NULL;
    }

    /* Retrieve the message body */
    ism_protocol_getObjectValue(&map, &pfield);
    if (pfield.type != VT_Null) {
    	ismc_setContent(message, pfield.val.s, pfield.len);
    }

    return message;
}


/*
 * Do message selection on a message
 */
int ismc_filterMessage(ismc_message_t * message, ismRule_t * rule) {
    return ism_common_filter(rule, message->h.props, NULL, NULL);
}


/**
 * Get the type of the message
 * @param  The message
 * @return THe type of the message (MTYPE_)
 */
int ismc_getMessageType(ismc_message_t * message) {
    return message->msgtype;
}


/**
 * Set a property on an ISM object.
 *
 * All messaging objects have properties.  For objects other than messages, the
 * properties are inherited from the object used to create the object.
 * Properties can be changed at any time, but some only have an effect when
 * a sub-object is created from this object.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The ISM C client does not check for valid
 *                 JMS names.
 * @param  field   The field containing the value.  Most field types are allowed,
 *                 but JMS folds many of these together (or instance the unsigned
 *                 scalar fields).  Name and NameIndex field types cannot be used.
 * @return  A return code: 0=good
 */
int ismc_setProperty(void * object, const char * name, ism_field_t * field) {
    int  rc;
    int  unlock = 0;
    ism_obj_hdr_t * obj = (ism_obj_hdr_t *)object;

    if (!object || !name || !*name || !field)
        return ISMRC_NullArgument;
    if ((obj->id&0xffffff00) != OBJID_ISM )
        return ISMRC_ObjectNotValid;
    if (field->type < 0 || field->type >= VT_Name)
        return ISMRC_ArgNotValid;

    if (obj->id == OBJID_Message) {
        if (name[0]=='J' && name[1]=='M' && name[2]=='S' && name[3]!='X') {
            /* Do not allow setting of reserved properties */
            return ISMRC_BadPropertyName;
        }
    } else {
        pthread_spin_lock(&obj->lock);
        unlock = 1;
    }
    if (!obj->props) {
        obj->props = ism_common_newProperties(20);
    }
    rc = ism_common_setProperty(obj->props, name, field);
    if (rc)
        rc = ISMRC_BadPropertyValue;

    if (unlock) {
        pthread_spin_unlock(&obj->lock);
    }
    return rc;
}


/*
 * Set a system property on a message.
 *
 * Make the properties if they do not exist, and add the specified property.
 * This can be used for any property as it does not check the name.
 * This only works on messages as it does not lock.
 */
static int setSystemProperty(ismc_message_t * message, const char * name, const char * value) {
    ism_field_t field = {0};
    if (!message->h.props) {
        message->h.props = ism_common_newProperties(20);
    }
    if (value) {
        field.type = VT_String;
        field.val.s = (char *)value;
    }
    return ism_common_setProperty(message->h.props, name, &field);
}


/**
 * Set a string property on an ISM object.
 *
 * Set the property for an object with the a type of null terminated string.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The ISM C client does not check for valid
 *                 JMS names.
 * @param  value   The value as a null terminated string encoded in UTF-8.
 * @return A return code: 0=good
 */
int ismc_setStringProperty(void * object, const char * name, const char * value) {
    ism_field_t field;
    if (value == NULL) {
        field.type = VT_Null;
    } else {
        field.type = VT_String;
        field.val.s = (char *)value;
    }
    return ismc_setProperty(object, name, &field);
}


/**
 * Set an integer property on an ISM object.
 *
 * This can be used to set any of the data types represented as integer including:
 * Null, Boolean, Byte, Integer, Short, UByte, UInt, and UShort.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The ISM C client does not check for valid
 *                 JMS names.
 * @param  datatype The datatype of the integer field
 * @param  value   The integer value.
 * @return A return code: 0=good
 *
 */
int ismc_setIntProperty(void * object, const char * name, int value, int datatype) {
    ism_field_t field;
    if (datatype < VT_Byte || datatype > VT_UInt)
        return ISMRC_BadPropertyValue;
    field.type = datatype;
    field.val.i = value;
    return ismc_setProperty(object, name, &field);
}


/**
 * Get a property from an ISM object.
 *
 * Most of the defined field types are possible, but JMS only produces a subset
 * of them.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The ISM C client does not check for valid
 *                 JMS names.
 * @param  field   A field into which to return the property.
 * @return A return code: 0=good
 */
int ismc_getProperty(void * object, const char * name, ism_field_t * field) {
    int  rc;
    int  unlock = 0;
    ism_obj_hdr_t * obj = (ism_obj_hdr_t *)object;

    if (!object || !name || !*name || !field) {
        return ISMRC_NullArgument;
    }
    if ((obj->id&0xffffff00) != OBJID_ISM ) {
        return ISMRC_ObjectNotValid;
    }

    if (!obj->props) {
        field->type = VT_Null;
        return ISMRC_PropertyNotFound;
    }

    if (obj->id != OBJID_Message) {
        pthread_spin_lock(&obj->lock);
        unlock = 1;
    }
    rc = ism_common_getProperty(obj->props, name, field);
    if (rc)
        rc = ISMRC_PropertyNotFound;
    if (unlock) {
        pthread_spin_unlock(&obj->lock);
    }
    return rc;
}


/**
 * Get a string property from an ISM object.
 *
 * Type conversion is not done with the restricted type conversion of JMS.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The ISM C client does not check for valid
 *                 JMS names.
 * @return A string or NULL if the property does not exist or cannot be converted to a string.
 */
const char * ismc_getStringProperty(void * object, const char * name) {
    int   rc;
    ism_field_t field;
    ism_obj_hdr_t * obj;
    const char * str = NULL;
    rc = ismc_getProperty(object, name, &field);
    if (!rc) {
        switch(field.type) {
        case VT_Null:
            return NULL;
        case VT_String:
            return field.val.s;                                   /* BEAM suppression: uninitialized */
        default: break;
        }
        obj = (ism_obj_hdr_t *)object;
        if (obj->id == OBJID_Message) {
            str = ism_common_convertToString(obj->props, &field);
        } else {
            pthread_spin_lock(&obj->lock);
            str = ism_common_convertToString(obj->props, &field);
            pthread_spin_unlock(&obj->lock);
        }

    }
    return str;
}



/*
 * Modified version of strtol which handles hex and decimal, but not octal
 */
static int str2l(const char * str, char * * endp) {
    const char * cp = str;
    int  ret;
    while (*cp==' ' || *cp=='\t') cp++;
    if (*cp == '0' && cp[1]=='x')
        ret = strtol(cp+2, endp, 16);
    else
        ret = strtol(cp, endp, 10);
    if (endp && *endp > str && **endp) {
        cp = *endp;
        while (*cp==' ' || *cp=='\t') cp++;
        *endp = (char *)cp;
    }
    return ret;
}

/**
 * Get an integer property from an ISM object
 *
 * Type conversion is not done with the restricted type conversion of JMS.
 * This method can also be used to get a boolean property.
 *
 * @param  object  The object which can be a connection, session, destination,
 *                 consumer, producer, or message.
 * @param  name    The name of the property.  This must be non-null and of length
 *                 greater than zero.  The ISM C client does not check for valid
 *                 JMS names.
 * @param  default_val  The default value to return when the property is missing
 *                 or cannot be converted to an integer.
 * @return A return code: integer value
 */
int ismc_getIntProperty(void * object, const char * name, int default_val) {
    int    rc;
    char * endp;
    int    val;
    ism_field_t f;
    rc = ismc_getProperty(object, name, &f);
    if (rc)
        return default_val;

    switch (f.type) {
    case VT_Null:                      /* Null value                       */
    case VT_ByteArray:                 /* Type of Byte Array               */
    default:
        return default_val;

    case VT_String:                    /* Type of String                   */
        if (!f.val.s)                  /* BEAM suppression: uninitialized  */
            return default_val;
        val = str2l(f.val.s, &endp);
        if (*endp) {
            if (strcmpi(f.val.s, "false"))
                return 0;
            if (strcmpi(f.val.s, "true"))
                return 1;
            return default_val;
        }
        return val;

    case VT_Boolean:                   /* Type of boolean                  */
        return f.val.i ? 1 : 0;        /* BEAM suppression: uninitialized  */

    case VT_Byte:                      /* Signed 8 bit value               */
    case VT_UByte:                     /* Unsigned 8 bit value             */
    case VT_Short:                     /* Signed 16 bit value              */
    case VT_UShort:                    /* Unsigned 16 bit value            */
    case VT_Integer:                   /* Type of Integer                  */
    case VT_UInt:                      /* Type of Integer                  */
        return f.val.i;                /* BEAM suppression: uninitialized  */

    case VT_Long:                      /* Type of Long Integer             */
    case VT_ULong:                     /* Type of Long Integer             */
        return (int32_t)f.val.l;       /* BEAM suppression: uninitialized  */

    case VT_Float:                     /* Type of Float                    */
        return (int32_t)f.val.f;       /* BEAM suppression: uninitialized  */

    case VT_Double:                    /* Type of Double                   */
        return (int32_t)f.val.d;       /* BEAM suppression: uninitialized  */
    };
}


/**
 * Set the correlation ID of the message.
 *
 * The correlation ID is defined by the application.
 * @param message  A message
 * @param corrid   A NULL-terminated string representing correlation identifier.
 * @return A return code: 0=good
 */
int ismc_setCorrelationID(ismc_message_t * message, const char * corrid) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    setSystemProperty(message, "JMSCorrelationID", corrid);
    message->has |= HAS_CORRID;
    return 0;
}


/**
 * Get the correction ID of the message.
 *
 * @param message  A message
 */
const char * ismc_getCorrelationID(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return NULL;
    }
    return ismc_getStringProperty(message, "JMSCorrelationID");
}


/**
 * Set the delivery mode of the message as an integer value.
 * @param message  A message
 * @param delmode the delivery mode 0=non-persistent, 1=persistent
 * @return A return code: 0=good
 *
 */
int ismc_setDeliveryMode(ismc_message_t * message, int delmode) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    if (delmode < 0 || delmode > 9)
        return ISMRC_ArgNotValid;

    if (delmode == ISMC_PERSISTENT) {
    	message->has |= IS_PERSIST;
    }
    else {
        message->has &= ~IS_PERSIST;
    }

    return 0;
}

/**
 * Get the delivery mode of the message as an integer value.
 * @param message  A message
 * @return the delivery mode 0=non-persistent, 1=persistent
 */
int ismc_getDeliveryMode(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    return (message->has & IS_PERSIST)?ISMC_PERSISTENT:ISMC_NON_PERSISTENT;
}

/**
 * Set the message ID of the message.
 * This messageID is replaced by the system generated messageID when a message is sent.
 * @param message  A message
 * @return A return code: 0=good
 */
int ismc_setMessageID(ismc_message_t * message, const char * msgid) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }

    setSystemProperty(message, "JMSMessageID", msgid);
    message->has |= HAS_MSGID;
    return 0;
}

/**
 * Get the message ID of the message.
 * The message ID is automatically created when a message is sent unless the
 * DisableMessageID property is set on the message producer in which case the
 * message ID of the message is set to null.
 * @param message  A message
 * @return The message ID or NULL to indicate there is no message ID
 */
const char * ismc_getMessageID(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return NULL;
    }
    return ismc_getStringProperty(message, "JMSMessageID");
}

/**
 * Set the priority of the message.
 * @param message  A message
 * @param priority The message priority as a value 0 to 9.  The value 4 is a normal message.
 * @return A return code: 0=good
 */
int ismc_setPriority(ismc_message_t * message, int priority) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }

    if (priority<ismMESSAGE_PRIORITY_MINIMUM || priority>ismMESSAGE_PRIORITY_MAXIMUM)
        return ISMRC_ArgNotValid;
    message->priority = priority;
    return 0;
}

/**
 * Get the message priority.
 * @param message  A message
 * @return The message priority as a value 0 to 9.  The value 4 is a normal priority.
 */
int ismc_getPriority(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    return message->priority;
}

/**
 * Set the redelivered flag.
 * This flag is automatically set when a message is received.
 * @param message  A message
 * @param redelivered The redelived flag, 0=not redelivered
 * @return A return code: 0=good
 */
int ismc_setRedelivered(ismc_message_t * message, int redelivered) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }

    message->redelivered = 1;
    return 0;
}

/**
 * Return the redelivered flag
 * @param message  A message
 * @return The redelivered flag, 0=not redelivered
 */
int ismc_getRedelivered(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    return message->redelivered;
}


/**
 * Set the reply to destination
 * @param message  A message
 * @param The destination name
 * @param The destination domain (1=queue, 2=topic)
 * @return A return code: 0=good
 */
int ismc_setReplyTo(ismc_message_t * message, const char * dest, int domain) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }

    if (domain != ismc_Queue && domain != ismc_Topic)
        return ISMRC_ArgNotValid;
    setSystemProperty(message, "JMSReplyTo", dest);
    message->replyDomain = (uint8_t)domain;
    message->has |= HAS_REPLY;
    return 0;
}

/*
 * Return the reply to destination
 *
 * @param message  A message
 * @param domain   The return location for the domain.  If non-null return the domain in this variable.
 *                 This can have the values: 0=uknnown, 1=queue, 2=topic.
 * @return The name of the destination or NULL to indicate the destination name is unknown.
 */
const char * ismc_getReplyTo(ismc_message_t * message, int * domain) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return NULL;
    }
    if (domain)
        *domain = (int)message->replyDomain;
    return ismc_getStringProperty(message, "JMSReplyTo");
}


/*
 * Set the original destination.
 *
 * The original destination is reset when a message is sent.
 * @param message  A message
 * @param The destination name
 * @param The destination domain (1=queue, 2=topic)
 * @return A return code: 0=good
 */
int ismc_setDestination(ismc_message_t * message, const char * dest, int domain) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    if (domain != ismc_Queue && domain != ismc_Topic)
        return ISMRC_ArgNotValid;
    setSystemProperty(message, "JMSDestination", dest);
    message->destDomain = (uint8_t)domain;
    message->has |= HAS_DEST;
    return 0;
}

/*
 * Return the original destination.
 *
 * If the message contains its original destination, retrun it.  This can be used to find
 * the original topic on which the message was published when using wild card subscriptions.
 *
 * @param message  A message
 * @param domain   The return location for the domain.  If non-null return the domain in this variable.
 *                 This can have the values: 0=unknown, 1=queue, 2=topic.
 * @return The name of the sent to destination or NULL to indicate the destination name is unknown.
 */
const char * ismc_getDestination(ismc_message_t * message, int * domain) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return NULL;
    }
    if (domain)
        *domain = (int)message->destDomain;
    return ismc_getStringProperty(message, "JMSDestination");
}

/*
 * Set the quality of service of a message
 *
 * The quality of service is not used by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  The quality of service has a value of 0-7.
 *
 * @param message  A message
 * @param quality  The quality of service (0-7)
 * @return A return code: 0=good
 */
int ismc_setQuality(ismc_message_t * message, int qos) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
   if (qos<0 || qos>7)
       return ISMRC_ArgNotValid;
   message->qos = qos;

   return 0;
}


/*
 * Return quality of service of a message
 *
 * The quality of service is not used by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  The quality of service has a value of 0-7.
 *
 * @param message  A message
 * @return The quality of service.
 */
int ismc_getQuality(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    return message->qos;
}

/*
 * Set the retain flag of a message
 *
 * Retain is not defined by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.
 *
 * @param message  A message
 * @param retain  The retain flag
 * @return A return code: 0=good
 */
int ismc_setRetain(ismc_message_t * message, int retain) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    message->retain = !!retain;
    return 0;
}


/*
 * Return the retain flag of a message
 *
 * Retain is not defined by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.
 *
 * @param message  A message
 * @return The retain flag.
 */
int ismc_getRetain(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    return message->retain&1;
}

/*
 * Return the retain flag of a message as publshed
 *
 * Retain is defined by JMS, but can be set and returned for messages
 * interchanged with other messaging protocols.  This flag indicates whether
 * the retain property should be propagated if this client is acting as a
 * forwarder of messages.
 *
 * @param message  A message
 * @return The as published retain flag.
 */
int ismc_getProgagateRetain(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    return message->retain>>1;
}


/**
 * Set the timestamp of the message as a millisecond timestamp.
 * The message timestamp is automatically set when a message is sent.
 * @param message  A message
 * @param tsmillis The timestamp
 * @return A return code: 0=good
 */
int ismc_setTimestamp(ismc_message_t * message, int64_t tsmillis) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    message->timestamp = tsmillis;
    return 0;
}

/**
 * Get the timestamp of the message as a millisecond timestamp.
 * The timestamp uses the Java and Unix epoch starting on 1970-01-01:T00UTC and
 * ignoring leap seconds.
 * @param message  A message
 */
int64_t ismc_getTimestamp(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    return message->timestamp;
}

/**
 * Set the expiration time of a message.
 * The expiration time is automatically set when a message is sent.
 * @param message  A message
 * @param tsmilli  The expiration time
 * @return A return code: 0=good
 */
int ismc_setExpiration(ismc_message_t * message, int64_t tsmillis) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    message->expire = tsmillis*(1000*1000);
    return 0;
}

/**
 * Get the expiration time of a message.
 * @param message  A message
 * @return The expiration time, or zero to indicate the message does not expire.
 */
int64_t ismc_getExpiration(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return 0;
    }
    return message->expire/(1000*1000);
}

/**
 * Set the time to live of a message.
 * The time to live is in milliseconds.
 * @param message  A message
 * @param millis  The time to live or zero to indicate that the message does not expire
 * @return A return code: 0=good
 */
int ismc_setTimeToLive(ismc_message_t * message, int64_t millis) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }

	message->ttl = millis;

	return 0;
}

/**
 * Get the time to live of a message.
 * The time to live is in milliseconds.
 * @param message  A message
 * @return The time to live, or zero to indicate the message does not expire.
 */
int64_t ismc_getTimeToLive(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }

	return message->ttl;
}

/**
 * Set a message type string.
 * This is a user defined string.
 * @param message  A message
 * @param jmstype  The type string.
 * @return A return code: 0=good
 */
int ismc_setTypeString(ismc_message_t * message, const char * jmstype) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return -1;
    }
    setSystemProperty(message, "JMSType", jmstype);
    message->has |= HAS_TYPE;
    return 0;
}

/**
 * Get the message type string.
 * This is a user defined string.
 * @param message  A message
 * @return A type string or NULL to indicate there is no type string
 */
const char * ismc_getTypeString(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return NULL;
    }
    return ismc_getStringProperty(message, "JMSType");
}

/**
 * Get the connection associated with an object.
 *
 * Return the Connection property associated with the object as a connection object.
 * @param An IMS messaging object
 * @return A connection object or NULL to indicate there is no associated connection.
 */
ismc_connection_t * ismc_getConnection(void * object) {
    ism_obj_hdr_t * obj = (ism_obj_hdr_t *)object;
    if (!object) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return NULL;
    }
    if ((obj->id&0xffffff00) != OBJID_ISM) {
        ismc_setError(ISMRC_ObjectNotValid, NULL);
        return NULL;
    }
    switch(obj->id) {
    case OBJID_Connection:
        return (ismc_connection_t *)object;
    case OBJID_Session:
        return ((ismc_session_t *)object)->connect;
    case OBJID_Consumer:
        return ((ismc_consumer_t *)object)->session->connect;
    case OBJID_Producer:
        return ((ismc_producer_t *)object)->session->connect;
    case OBJID_Message:
        return ((ismc_message_t *)object)->session->connect;
    }
    return NULL;
}


/**
 * Get the session associated with an object.
 *
 * Return the Session property associated with the object as a session object.
 * @param An IMS messaging object
 * @return A connection object or NULL to indicate there is no associated session.
 */
ismc_session_t * ismc_getSession(void * object) {
    ism_obj_hdr_t * obj = (ism_obj_hdr_t *)object;
    if (!object) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return NULL;
    }
    if ((obj->id&0xffffff00) != OBJID_ISM) {
        ismc_setError(ISMRC_ObjectNotValid, NULL);
        return NULL;
    }
    switch(obj->id) {
    case OBJID_Session:
        return ((ismc_session_t *)object);
    case OBJID_Consumer:
        return ((ismc_consumer_t *)object)->session;
    case OBJID_Producer:
        return ((ismc_producer_t *)object)->session;
    case OBJID_Message:
        return ((ismc_message_t *)object)->session;
    }
    return NULL;
}


/**
 * Clear the properties of an object.
 * This can clear the properties of any ISM client object.
 * @return A return code: 0=good
 */
int ismc_clearProperties(void * object) {
    ism_obj_hdr_t * obj = (ism_obj_hdr_t *)object;
    if (!object)
        return ISMRC_NullArgument;
    if ((obj->id&0xffffff00) != OBJID_ISM)
        return ISMRC_ObjectNotValid;
    if (obj->id != OBJID_Message) {
        ism_common_clearProperties(obj->props);
    } else {
        pthread_spin_lock(&obj->lock);
        if (obj->props)
            ism_common_clearProperties(obj->props);
        pthread_spin_unlock(&obj->lock);
    }
    return 0;
}


/**
 * Set the bytes contents of a message.
 * Replace the bytes content of the message.
 * @return A return code: 0=good
 */
int ismc_setContent(ismc_message_t * message, const char * bytes, int len) {
	message->body.used = 0;
    if (message->body.len <= len) {
    	ism_common_allocAllocBuffer(&message->body, len + 1, 0);
    }
    memcpy(message->body.buf, bytes, len);
    message->body.buf[len] = 0;
    message->body.used = len;

    return 0;
}


/**
 * Get the size of the content of a message in bytes.
 * The content of any message type has a message length in bytes.
 * @param message The message
 * @return A return code: 0=good
 */
int ismc_getContentSize(ismc_message_t * message) {
    return message->body.used;
}


/**
 * Get the count of content items in the message.
 * For text and bytes messages this is either 0 or 1.
 * For map and stream messages it gives the count of items.
 * @param message The message
 * @return A count of items in the message
 */
int ismc_getContentCount(ismc_message_t * message) {
	int count = 0;
	concat_alloc_t map;
	ism_field_t field;
	switch (message->msgtype) {
	case MTYPE_MapMessage:
	case MTYPE_StreamMessage:
		map = message->body;
		map.pos = 1;
		while (ism_protocol_getObjectValue(&map, &field) == 0) {
			/* Ignore property names */
			if (field.type != VT_Name) {
				count++;
			}
		}

		break;

	default:
		count = message->body.used ? 1 : 0;
		break;
	}
    return count;
}


/**
 * Get the content of a message as bytes.
 * The content of any message type can be returned as bytes.
 * @param message  The message
 * @param buffer   The address to put the data
 * @param start    The starting offset within the message data
 * @param len      The length of the buffer to return the data
 * @return The number of bytes returned.  If start is beyond the content size, this is negative.
 */
int ismc_getContent(ismc_message_t * message, char * buffer, int start, int len) {
    int  ret;

    if (!buffer)
        return ISMRC_NullArgument;
    if (start < 0 || len < 0)
        return ISMRC_ArgNotValid;

    ret = message->body.used - start;
    if (ret >= 0) {
        if (ret < len) {
            memcpy(buffer, message->body.buf+start, ret);
            buffer[ret] = 0;
        } else {
            memcpy(buffer, message->body.buf+start, len);
            ret = len;
        }
    }
    return ret;
}


/**
 * Set the contents of a message as a string.
 * Replace the current contents of the message with the specified string.
 * @param message  The message
 * @param string   The string as UTF-8 to use as the text of the message
 * @return A return code: 0=good
 */
int ismc_setContentString(ismc_message_t * message, const char * string) {
	message->body.used = 0;
	if (string != NULL) {
		int slen = (int)strlen(string) + 1;
		if (message->body.len < slen) {
			ism_common_allocAllocBuffer(&message->body, slen - message->body.len, 0);
		}
		memcpy(message->body.buf, string, slen);
		message->body.used = slen-1;
	}
    return 0;
}


/**
 * Set the contents of a message as a name,type,value field.
 *
 * @param message  The message
 * @param name     The name of the field.  This must be a valid name.
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
int ismc_addMapContent(ismc_message_t * message, const char * name, ism_field_t * field) {
	if (message == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The message cannot be NULL");
	}

	if (message->h.id != OBJID_Message) {
		return ismc_setError(ISMRC_ObjectNotValid, "The input is not a message");
	}

	if (message->msgtype != MTYPE_MapMessage || message->body.buf[0] != 0x9F) {
		return ismc_setError(ISMRC_MessageNotValid, "Not a map message");
	}

	if (name == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The input field name cannot be NULL");
	}

	if (field == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The input field cannot be NULL");
	}

	ism_protocol_putNameValue(&message->body, name);
	ism_protocol_putObjectValue(&message->body, field);

	return 0;
}


/**
 * Set the contents of a message as a type,value field.
 * Fields are put in the message in the order they are added.
 * @param message  The message
 * @param name     The name of the field
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
int ismc_addStreamContent(ismc_message_t * message, ism_field_t * field) {
	if (message == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The message cannot be NULL");
	}

	if (message->h.id != OBJID_Message) {
		return ismc_setError(ISMRC_ObjectNotValid, "The input is not a message");
	}

	if (message->msgtype != MTYPE_StreamMessage || message->body.buf[0] != 0x9E) {
		return ismc_setError(ISMRC_MessageNotValid, "Not a stream message");
	}

	if (field == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The input field cannot be NULL");
	}

	ism_protocol_putObjectValue(&message->body, field);

	return 0;
}


/**
 * Get the contents of a message as a name,type,value field
 * This implements the MapMessage pattern.
 * When the field contains a pointer it is into the message data and is only valid as long
 * as the message is valid.
 * @param message  The message
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
int ismc_getContentField(ismc_message_t * message, const char * name, ism_field_t * field) {
	concat_alloc_t map;
	int found = 0;

	if (message == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The message cannot be NULL");
	}

	if (message->h.id != OBJID_Message) {
		return ismc_setError(ISMRC_ObjectNotValid, "The input is not a message");
	}

	if (message->msgtype != MTYPE_MapMessage || message->body.buf[0] != 0x9F) {
		return ismc_setError(ISMRC_MessageNotValid, "Not a map message");
	}

	if (name == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The field name cannot be NULL");
	}

	if (field == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The output field cannot be NULL");
	}

	map = message->body;
	map.pos = 1;
	while (ism_protocol_getObjectValue(&map, field) == 0) {
		/* Check property names */
		if (field->type == VT_Name) {
			if (!strcmp(field->val.s, name)) {
				found = 1;
				break;
			}
		}
	}

	if (found) {
		return 0;
	} else {
		field->type = VT_Null;
		return ISMRC_NotFound;
	}
}


/**
 * Get the contents of a message as a type,value field by index.
 * This implements the StreamMessage pattern.
 * When the field contains a pointer it is into the message data and is only valid as long
 * as the message is valid.
 * @param message  The message
 * @param index    The index of the field to return.
 * @param field The return location of the data.
 * @return A return code: 0=good
 */
int ismc_getContentFieldIx(ismc_message_t * message, int index, ism_field_t * field) {
	concat_alloc_t map;
	int found = 0;
	int count = 0;

	if (message == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The message cannot be NULL");
	}

	if (message->h.id != OBJID_Message) {
		return ismc_setError(ISMRC_ObjectNotValid, "The input is not a message");
	}

	if (message->msgtype != MTYPE_StreamMessage || message->body.buf[0] != 0x9E) {
		return ismc_setError(ISMRC_MessageNotValid, "Not a stream message");
	}

	if (index < 0) {
		return ismc_setError(ISMRC_ArgNotValid, "The field index cannot be negative");
	}

	if (field == NULL) {
		return ismc_setError(ISMRC_NullPointer, "The output field cannot be NULL");
	}

	map = message->body;
	map.pos = 1;
	while (ism_protocol_getObjectValue(&map, field) == 0) {
		/* Check the property index */
		if (index == count) {
			found = 1;
			break;
		}
		count++;
	}

	if (found) {
		return 0;
	} else {
		field->type = VT_Null;
		return ISMRC_NotFound;
	}
}


/**
 * Clear the content of a message
 * @param message  The message to modify.
 * @return A return code: 0=good
 */
int ismc_clearContent(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return 0;
    }

    message->body.used = 0;
    return 0;
}

/**
 * Get the order ID for this message.
 *
 * @param message   The message object.
 * @return An order ID, if available; otherwise 0.
 */
uint64_t ismc_getOrderID(ismc_message_t * message) {
    if (!message) {
        ismc_setError(ISMRC_NullArgument, NULL);
        return 0;
    }

	return message->order_id;
}
