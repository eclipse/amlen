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
/*
 */

#define TRACE_COMP Monitoring

#include <monitoring.h>
#include <ismutil.h>
#include <monitoringutil.h>

/***************Memory extern functions*********************/
extern void * ism_monitoring_newMemoryHistDataObject(void);
extern int ism_monitoring_recordMemorySnapShot(ism_monitoring_snap_t * slist, ism_monitoring_snap_t * llist, void *stat) ;

/***************Store extern functions*********************/
extern void * ism_monitoring_newStoreHistDataObject(void);
extern int ism_monitoring_recordStoreSnapShot(ism_monitoring_snap_t * slist, ism_monitoring_snap_t * llist, void *stat) ;


/*Create a new snapshot list*/
static int createNewMonSnapList(ism_monitoring_snap_t ** monlist, ism_snaptime_t snap_interval) {
	(*monlist)                 = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,13),sizeof(ism_monitoring_snap_t));
	if (!(*monlist)) {
		TRACE(9, "Monitoring: monitoring list initial failed on memory allocation\n");
	    return ISMRC_AllocateError;
	}
	(*monlist)->last_snap_shot = ism_monitoring_currentTimeSec();
	(*monlist)->snap_interval  = snap_interval;
	if (snap_interval == DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL)
	    (*monlist)->max_count      = 60*60/snap_interval +1;
	    

	else if (snap_interval == DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL)
		(*monlist)->max_count      = 24*60*60/snap_interval + 1;
		
	(*monlist)->range_list       = NULL;
	return ISMRC_OK;
}

/*Initialize snapshot lists*/
XAPI int ism_monitoring_initMonitoringSnapList(ism_monitoring_snap_list_t **snapList, int short_interval, int long_interval)
{
	int i;

	*snapList = (ism_monitoring_snap_list_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,14),sizeof(ism_monitoring_snap_list_t)*NUM_MONITORING_LIST);
	(*snapList)->numOfSnapList = NUM_MONITORING_LIST;
	(*snapList)->snapList = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,15),sizeof(ism_monitoring_snap_t *)*NUM_MONITORING_LIST);
	for (i = 0; i < NUM_MONITORING_LIST; i++) {
		if(i == 0)
		    createNewMonSnapList(&((*snapList)->snapList[i]), short_interval);
		else if(i == 1)
			createNewMonSnapList(&((*snapList)->snapList[i]), long_interval);
		else {
			TRACE(9, "Monitoring: monitoring list initial failed\n");
		    return ISMRC_NotImplemented;
		}
	}
	return ISMRC_OK;
}

/*Get snapshot list by interval.*/
XAPI ism_monitoring_snap_t * ism_monitoring_getSnapshotListByInterval(ism_snaptime_t snap_interval, ism_monitoring_snap_list_t * snapList  )
{
	int i;
	if (!snapList) {
	    TRACE(9, "Monitoring: monitoring list has not been initialed\n");
		return NULL;
	}
	for (i = 0; i < snapList->numOfSnapList; i++) {
	    if ((snapList->snapList[i])->snap_interval == snap_interval)
			   return snapList->snapList[i];
	}
	return NULL;
}

/*Reuse the last data when the maximum is reached.*/
static int reuseSnapshotLastDataNode(ism_snapshot_range_t *sp) {
	//TRACE(9, "reuse last node\n");
	pthread_spin_lock(&sp->snplock);
	/*
	 * The monitoring data reaches all the slots, the last slot will be reused.
	 */
	ism_snapshot_data_node_t * node = sp->tail;
	sp->tail = node->prev;
	if(sp->tail!=NULL){
		(sp->tail)->next = NULL;   //new tail
	}

	//memset(node, 0, sizeof(ism_snapshot_data_node_t));
	
	node->next = sp->data_nodes;
	if(sp->data_nodes!=NULL){
		(sp->data_nodes)->prev = node;
	}
	node->prev = NULL;
	sp->data_nodes = (void *)node;

	pthread_spin_unlock(&sp->snplock);
	return ISMRC_OK;
}

/*Free a snapshot data node*/
static int freeHistDataSRange(ism_snapshot_range_t * node) {
	ism_snapshot_data_node_t * tp, * mp;
	int count= 0;

	if (!node) {
		TRACE(9, "Monitoring: nothing to be freed\n");
		return ISMRC_NullArgument;
	}
	
	pthread_spin_lock(&node->snplock);
	tp = node->data_nodes;
	while(tp) {
		mp = tp;
		tp = tp->next;
		if(mp->data!=NULL && node->data_destroy!=NULL){
			node->data_destroy(mp->data);
		}
		ism_common_free(ism_memory_monitoring_misc,mp);
		count++;
	}
	if (count != node->node_count) {
		TRACE(9, "Monitoring: inconsistence of node_count %llu and actual count %d\n", (unsigned long long)node->node_count, count);
	}
	
	pthread_spin_unlock(&node->snplock);
	node->next = NULL;
	ism_common_free(ism_memory_monitoring_misc,node);


	return ISMRC_OK;
}

/*
 * remove expired endpoint and assoicated mondata from endpoint list
 */
static int cleanExpiredSnapshotNode(ism_snapshot_range_t * snaprange, uint64_t max_count) {
	ism_snapshot_range_t * tmp = NULL, *p, *s;

	/*
	 * First clean all unused srange_t object.
	 */
	while(snaprange && (snaprange->node_idle >= max_count)) {
		tmp = snaprange;
		snaprange=snaprange->next;
		freeHistDataSRange(tmp);
	}

	p = snaprange;
	if (p) {
	    tmp=snaprange->next;
	} else {
		tmp = NULL;
	}
	while(tmp) {
        //TRACE(9, "Monitoring: clean an expired endpoint:%s\n", tmp->name);
        if(tmp->node_idle >= max_count) {
        	s = tmp;
        	tmp = tmp->next;
        	p->next = tmp;
        	freeHistDataSRange(s);
        }
        p = tmp;
        tmp = tmp->next;
	}
	return ISMRC_OK;
}

/*New data node for a particular statistics type*/
static ism_snapshot_data_node_t * newSnapshotDataNode(int statType) {
	
	ism_snapshot_data_node_t *dataNode = ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,18),1, sizeof(ism_snapshot_data_node_t));
	if (!dataNode) {
		TRACE(9, "Monitoring: ism monitoring data memory allocation failed\n");
		return NULL;
	}
	void *nmd =NULL;
	switch(statType){
		case ismMON_STAT_Memory:
			nmd = ism_monitoring_newMemoryHistDataObject();
			break;
		case ismMON_STAT_Store:
			nmd = ism_monitoring_newStoreHistDataObject();
			break;
		default:
			nmd=NULL;
	};
	
	if (!nmd) {
		ism_common_free(ism_memory_monitoring_misc,dataNode);
		TRACE(1, "Monitoring: ism monitoring data allocation failed\n");
		return NULL;
	}
	dataNode->prev = NULL;
    dataNode->next = NULL;
    dataNode->data = nmd;
	
    return dataNode;
}

/*Create new snapshot data node*/
static int createSnapshotDataNode(ism_snapshot_range_t *sp, int statType) {
	ism_snapshot_data_node_t *md = newSnapshotDataNode(statType);
	if (!md) {
		return ISMRC_AllocateError;
	}
	pthread_spin_lock(&sp->snplock);
	if (!sp->data_nodes) {
		sp->data_nodes =(void *) md;
	} else {
	   (sp->data_nodes)->prev =md;
	   md->next = sp->data_nodes;
	   sp->data_nodes =(void *) md;
	}
	sp->node_idle++;
	sp->node_count++;
	pthread_spin_unlock(&sp->snplock);
   
	return ISMRC_OK;
}

/*
 * Initialize and create a new Monitoring data node for the snapshot. 
 * A new(reused) monitoring node needs to be initialed as an idle node in case some endpoints can not be updated during the snapshot (disabled temporary etc.).
 *  -- clean all expired nodes -- idle mode in all its nodes. no reason to keep it.
 *  -- depends on the max_count, decided if a new node should be added or reused the earliest node.
 */
static int initSnapshotDataNode(ism_monitoring_snap_t * slist, ism_monitoring_snap_t * llist, int statType) {
	ism_snapshot_range_t *p;
	int rc;

	if (!slist) {
		TRACE(9, "Monitoring: initMonDataNode short list is NULL\n");
		return ISMRC_NotFound;
	}
	p = slist->range_list;

	/*
	 * First clean all unused srange_t object.
	 */
    if (!p)   {
    	return ISMRC_NotFound;
    }
	rc = cleanExpiredSnapshotNode(p, slist->max_count);
	if (rc)
		return rc;


	/*
	 * Adding a new node to all the endpoints' mondata list.
	 */
	while(p) {
		if (p->node_count < slist->max_count) {
			rc = createSnapshotDataNode(p, statType);
		} else {
			rc = reuseSnapshotLastDataNode(p);
		}
		if (rc)
			return rc;
		p = p->next;
	}

	//init 24H list
	if (llist) {
		p = NULL;
	    p = llist->range_list;
	    if (!p)   {
	    	return ISMRC_NotFound;
	    }
	    cleanExpiredSnapshotNode(p, llist->max_count);
		if (rc)
			return rc;

		/*
		 * Adding a new node to all the endpoints' mondata list.
		 */
		while(p) {
			if (p->node_count < llist->max_count) {
				rc = createSnapshotDataNode(p, statType);
			} else {
				rc = reuseSnapshotLastDataNode(p);
			}
			if (rc)
				return rc;
			p = p->next;
		}
	}
	return ISMRC_OK;
}

/*Create new snapshot range object which contain the data based on the statistics type*/
XAPI int ism_monitoring_newSnapshotRange(ism_snapshot_range_t ** list, int statType, void	(*data_destroy)(void *data)) {
	ism_snapshot_range_t *range = (ism_snapshot_range_t *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,20),sizeof(ism_snapshot_range_t));

	range->data_nodes = NULL;   /**< A double link list holding the monitroing data */
    range->node_count = 0;           /**< Count of nodes in the mondata list             */
    range->node_idle = 0;
    range->data_destroy = data_destroy;
    pthread_spin_init(&range->snplock, 0);
    createSnapshotDataNode(range, statType);
    range->tail = range->data_nodes;              /**< A point to the last node of monitoring_data    */

    range->next = *list;
    *list = range;
    
    return ISMRC_OK;
}

/*Record the snapshot for the statictics type*/
static int recordSnapShot(ism_monitoring_snap_t * slist, ism_monitoring_snap_t * llist, void *stat, int statType) {
	
	int rc;
	
	switch(statType){
		case ismMON_STAT_Memory:
			rc= ism_monitoring_recordMemorySnapShot(slist, llist, stat);
			break;
		case ismMON_STAT_Store:
			rc= ism_monitoring_recordStoreSnapShot(slist, llist, stat);
			break;
		default:
			rc=ISMRC_OK;
	};
	
	return rc;
}

/*
 * Check if a snapshot is needed at the time.
 */
XAPI int  ism_monitoring_updateSnapshot(ism_snaptime_t curtime, void * stat, int statType, ism_monitoring_snap_list_t * snapList) {
	int rc = ISMRC_OK;

	if(snapList==NULL){
		TRACE(7, "Snapshot List is NULL.\n");
		return ISMRC_NullPointer;
	}

	/*Short Interval List*/
	ism_monitoring_snap_t * slist = snapList->snapList[0];
	
	if(slist==NULL){
		TRACE(7, "Snapshot List for short interval is NULL.\n");
		return ISMRC_NullPointer;
	}
	
	/*Long Interval List*/
	ism_monitoring_snap_t * llist = snapList->snapList[1];
	
	if(llist==NULL){
		TRACE(7, "Snapshot List for long interval is NULL.\n");
		return ISMRC_NullPointer;
	}
	

    if (curtime - llist->last_snap_shot >= llist->snap_interval) {
    	/*Update both short and long interval snapshots*/
		rc = initSnapshotDataNode(slist, llist, statType);
		if (rc && rc != ISMRC_NotFound)
			return rc;
		rc = recordSnapShot(slist, llist, stat, statType);
		if (rc)
			return rc;
	    slist->last_snap_shot = curtime;
	    llist->last_snap_shot = curtime;

	} else {
		/*Update only short interval snapshot*/
		rc = initSnapshotDataNode(slist, NULL, statType);
		if (rc && rc != ISMRC_NotFound)
			return rc;
		rc = recordSnapShot(slist, NULL, stat, statType);
		if (rc)
			return rc;
	    slist->last_snap_shot = curtime;
	}
	return rc;
}
