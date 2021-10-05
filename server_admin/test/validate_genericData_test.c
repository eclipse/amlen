/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
 * Add CUNIT tests for generic configuration data validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

int testno = 0;

/*
 * Test setup or initialization
 * - Add any initialization code if required
 */
void test_validate_genericData_setup(void)
{

}

/*
 * Test cleanup
 * - Add cleanup code if required
 */
void test_validate_genericData_cleanup(void)
{

}

/*************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "NUMBER"   ---------------
 *
 *************************************************************************************************/

/* wrapper to call ism_config_validateDataType_number() API */
static int32_t validate_number(char *name, char *value, char *min, char *max, char *qualif, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_number(name, value, min, max, qualif);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s min=%s max=%s qualif=%s exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", min?min:"NULL",
            max?max:"NULL", qualif?qualif:"NULL", exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_genericData_number(void)
{
	/* Test name cannot be NULL or empty */
	CU_ASSERT( validate_number( NULL, "30G", "1", "10", NULL, ISMRC_NullPointer ) == ISMRC_NullPointer );
    CU_ASSERT( validate_number( "", "30G", "1", "10", NULL, ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value cannot be NULL or empty */
    CU_ASSERT( validate_number( "TestConfig", NULL, "1", "10", NULL, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_number( "TestConfig", "", "1", "10", NULL, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test out of range number fail the checking without qualifier */
    CU_ASSERT( validate_number( "TestConfig", "20", "1", "10", NULL, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_number( "TestConfig", "-1", "0", "10", NULL, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_number( "TestConfig", "1", "2", "10", NULL, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test out of range number fail the checking with different qualifiers */
    CU_ASSERT( validate_number( "TestConfig", "20M", "1", "10", ",M", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_number( "TestConfig", "2K", "1024", "4048", "M", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_number( "TestConfig", "2b", "1024", "4048", "M", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_number( "TestConfig", "20M", "1", "10", ", ,M", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_number( "TestConfig", "2M", "1", "10", "M,", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test number with a based fail the checking because it is not part of the qualifier */
    CU_ASSERT( validate_number( "TestConfig", "1G", "1024", "1073741825", "M", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test number with a unsupported based fail the checking without qualifiers */
    CU_ASSERT( validate_number( "TestConfig", "1P", "1", "10", NULL, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );

    /* Test in-range number pass the checking without qualifier */
    CU_ASSERT( validate_number( "TestConfig", "5", "1", "10", "", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "5", "1", "10", NULL, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1", "-1", "10", NULL, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "100", NULL, NULL, NULL, ISMRC_OK ) == ISMRC_OK );
    /* Test in-range number with base(k/K) pass the checking without qualifier */
    CU_ASSERT( validate_number( "TestConfig", "2K", "1024", "4048", NULL, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "2k", "1024", "4048", NULL, ISMRC_OK ) == ISMRC_OK );
    /* Test in-range number with base(k/K) pass the checking with qualifier  "k/K"*/
    CU_ASSERT( validate_number( "TestConfig", "2K", "1024", "4048", "K", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "2K", "1024", "4048", "k", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "2k", "1024", "4048", "K", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "2k", "1024", "4048", "k", ISMRC_OK ) == ISMRC_OK );

    /* Test in-range number with base(m/M) pass the checking without qualifier */
    CU_ASSERT( validate_number( "TestConfig", "1M", "1", "1048577", NULL, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1m", "1", "1048577", NULL, ISMRC_OK ) == ISMRC_OK );
    /* Test in-range number with base(m/M) pass the checking with qualifier  "M/m"*/
    CU_ASSERT( validate_number( "TestConfig", "1M", "1", "1048577", "M", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1M", "1", "1048577", "m", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1m", "1", "1048577", "M", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1m", "1", "1048577", "m", ISMRC_OK ) == ISMRC_OK );

    /* Test in-range number with base(g/G) pass the checking without qualifier */
    CU_ASSERT( validate_number( "TestConfig", "1G", "1024", "1073741825", NULL, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1g", "1024", "1073741825", NULL, ISMRC_OK ) == ISMRC_OK );
    /* Test in-range number with base(g/G) pass the checking with qualifier  "G/g"*/
    CU_ASSERT( validate_number( "TestConfig", "1G", "1024", "1073741825", "G", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1G", "1024", "1073741825", "g", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1g", "1024", "1073741825", "G", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_number( "TestConfig", "1g", "1024", "1073741825", "g", ISMRC_OK ) == ISMRC_OK );

    /* Test a number with base(k) failed the checking if multiple qualifier does not have "K" */
    CU_ASSERT( validate_number( "TestConfig", "2K", "1024", "4048", "M,G", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test a number with base(k) pass the checking if multiple qualifier has "K" and the number is in-range*/
    CU_ASSERT( validate_number( "TestConfig", "2K", "1024", "4048", "M,G,K", ISMRC_OK ) == ISMRC_OK );
    /* Test a in-range number with base(k) failed the checking if qualifier is not valid */
    CU_ASSERT( validate_number( "TestConfig", "2K", "1024", "4048", "P", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );

    return;
}

/*************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "ENUM"   -----------------
 *
 *************************************************************************************************/

/* wrapper to call ism_config_validateDataType_enum() API */
static int32_t validate_enum(char *name, char *value, char *options, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_enum(name, value, options, ISM_CONFIG_PROP_ENUM);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s options=%s exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", options?options:"NULL", exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type enum using ism_config_validateDataType_enum() API */
void test_validate_genericData_enum(void)
{
	/* Test name cannot be NULL or empty */
	CU_ASSERT( validate_enum( NULL, "20", "10,20,30,40,50,100", ISMRC_NullPointer ) == ISMRC_NullPointer );
    CU_ASSERT( validate_enum( "",   "20", "10,20,30,40,50,100", ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value cannot be NULL or empty */
    CU_ASSERT( validate_enum( "TestConfig", NULL, "10,20,30,40,50,100", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_enum( "TestConfig", "", "10,20,30,40,50,100", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test enum list cannot be NULL or empty */
    CU_ASSERT( validate_enum( "TestConfig", "20", NULL, ISMRC_BadOptionValue ) == ISMRC_BadOptionValue );
    CU_ASSERT( validate_enum( "TestConfig", "20", "", ISMRC_BadOptionValue ) == ISMRC_BadOptionValue );
    /* Test unsupported enum value fail the checking */
    CU_ASSERT( validate_enum( "TestConfig", "1", "MIN,NORMAL,MAX", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_enum( "TestConfig", "normal", "MIN,NORMAL,MAX", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_enum( "TestConfig", "Non-Developers", "Developers,Non-Production,Production", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_enum( "TestConfig", "20.1", "10,20,30,40,50,100", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_enum( "TestConfig", "TLSv1.5", "SSLv3,TLSv1,TLSv1.1,TLSv1.2", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );

    /* Test supported enum value pass the checking */
    CU_ASSERT( validate_enum( "TestConfig", "NORMAL", "NORMAL", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_enum( "TestConfig", "40", "10,20,30,40,50,100", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_enum( "TestConfig", "Production", "Developers,Non-Production,Production", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_enum( "TestConfig", "TLSv1", "SSLv3,TLSv1,TLSv1.1,TLSv1.2", ISMRC_OK ) == ISMRC_OK );

    return;
}

/*************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "BOOLEAN"   --------------
 *
 *************************************************************************************************/

/* wrapper to call ism_config_validateDataType_boolean() API */
static int32_t validate_boolean(char *name, char *value, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_boolean(name, value);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type boolean using ism_config_validateDataType_boolean() API */
void test_validate_genericData_boolean(void)
{
	/* Test name cannot be NULL or empty */
	CU_ASSERT( validate_boolean( NULL, "20", ISMRC_NullPointer ) == ISMRC_NullPointer );
    CU_ASSERT( validate_boolean( "",   "true", ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value cannot be NULL or empty */
    CU_ASSERT( validate_boolean( "TestConfig", NULL, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test unsupported boolean value fail the checking */
    CU_ASSERT( validate_boolean( "TestConfig", "1", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "Truea", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "falsal", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "true false", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "true,false", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "truefalse", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "true_false", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_boolean( "TestConfig", "true/false", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );

    /* Test supported boolean value pass the checking */
    CU_ASSERT( validate_boolean( "TestConfig", "True", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_boolean( "TestConfig", "False", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_boolean( "TestConfig", "TRUE", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_boolean( "TestConfig", "FALSE", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_boolean( "TestConfig", "tRUe", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_boolean( "TestConfig", "true", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_boolean( "TestConfig", "fALSe", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_boolean( "TestConfig", "false", ISMRC_OK ) == ISMRC_OK );

    return;
}

/*************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "STRING"   --------------
 *
 *************************************************************************************************/

/* wrapper to call ism_config_validateDataType_string() API */
static int32_t validate_string(char *name, char *value, int type, char *maxlen, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_string(name, value, type, maxlen, NULL);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s type=%d maxlen=%s exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", type, maxlen, exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_genericData_string(void)
{
	/* Test name cannot be NULL */
    CU_ASSERT( validate_string( NULL, "Test Description", 0, "100", ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value NULL with type==1 should pass the checking */
    CU_ASSERT( validate_string( "Destination", NULL, 1, "100", ISMRC_OK ) == ISMRC_OK );
    /* Test empty value with type==0 pass the checking */
    CU_ASSERT( validate_string( "Description", "", 0, "100", ISMRC_OK ) == ISMRC_OK );
    /* Test NULL value with type==0 pass the checking */
    CU_ASSERT( validate_string( "Description", NULL, 0, "100", ISMRC_OK ) == ISMRC_OK );
    /* Test a value with type==0 pass the checking */
    CU_ASSERT( validate_string( "Description", "Test Description", 0, "100", ISMRC_OK ) == ISMRC_OK );
    /* Test a white space value with type==0 pass the checking */
    CU_ASSERT( validate_string( "Description", " ", 0, "100", ISMRC_OK ) == ISMRC_OK );
    /* Test special no-meaningful unicode value with type==0 failed the checking */
    CU_ASSERT( validate_string( "Description", "\xed\xa1\x80", 0, "100", ISMRC_ObjectNotValid ) == ISMRC_ObjectNotValid );
    CU_ASSERT( validate_string( "Description", "\xf1\x80\x80\xc4", 0, "100", ISMRC_ObjectNotValid ) == ISMRC_ObjectNotValid );
    CU_ASSERT( validate_string( "Description", "\xc1\x80", 0, "100", ISMRC_ObjectNotValid ) == ISMRC_ObjectNotValid );
    CU_ASSERT( validate_string( "Description", "\xf0\x80\x80\x80", 0, "100", ISMRC_ObjectNotValid ) == ISMRC_ObjectNotValid );
    /* Test Description with starting/trailing white space pass the checking */
    CU_ASSERT( validate_string( "Description", " Test Description ", 0, "100", ISMRC_OK ) == ISMRC_OK );
    /* Test string pass the checking */
    CU_ASSERT( validate_string( "Description", "Test Description", 0, "12", ISMRC_LenthLimitSingleton ) == ISMRC_LenthLimitSingleton );
    /* Test unicode string with type==0 pass the checking */
    CU_ASSERT( validate_string( "Description", "\x61\x62\x63", 0, "100", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_string( "Description", "a\xf0\xa0\xa0\x80", 0, "4", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_string( "Description", "a\xf1\x80\x80\x80", 0, "2", ISMRC_OK ) == ISMRC_OK );

}

/*************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "NAME"   ---------------
 *
 *************************************************************************************************/
/* wrapper to call ism_config_validateDataType_name() API */
static int32_t validate_name(char *name, char *value, char *maxlen, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_name(name, value, maxlen, NULL);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s maxlen=%s exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", maxlen, exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type name using ism_config_validateDataType_name() API */
void test_validate_genericData_name(void)
{
	/* Test name cannot be NULL or empty */
	CU_ASSERT( validate_name( NULL, "20", "256", ISMRC_NullPointer ) == ISMRC_NullPointer );
    CU_ASSERT( validate_name( "",   "true", "256", ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value cannot be NULL or empty */
    CU_ASSERT( validate_name( "TestConfig", NULL, "256", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_name( "TestConfig", "", "256", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );

    /* Test the name value cannot be exceed the maxlen */
    CU_ASSERT( validate_name( "TestConfig", "ABCDFDEFFKJSDKLSDJKSDJSLDKSJDSIDJSLDJSKDJASIJDSLAFNJHJDSJDSDJSDJSKDJSKDJKSJDKJDSDJKSDJKSJDKSJDKSJDKJSDKJSKDJKSJDJDKSJDKJSDKJSKDJKSJDKSJDKSJ185jfdkjfljeij0fdafdsfdsafjldafjdsljfldsafjlsdfjkldfjfdsafjdklsjfklsdjfklsdjfklsdajfklsdajfklsdjfkldsjfklsdajfklsdjfklsdjfkljsdaklfjasdklfjsdkla329", "256", ISMRC_NameLimitExceed ) == ISMRC_NameLimitExceed );
    CU_ASSERT( validate_name( "ChannelName", "channelNameCanNotBeLongerThan20", "20", ISMRC_NameLimitExceed ) == ISMRC_NameLimitExceed );
    CU_ASSERT( validate_name( "QueueManagerName", "QueueManagerNameCanBe48ButItIsOverflowFDSD48overflow", "48", ISMRC_NameLimitExceed ) == ISMRC_NameLimitExceed );
    /* Test the name value cannot be have trailed white space */
    CU_ASSERT( validate_name( "TestConfig", "withlastwhitespace  ", "256", ISMRC_ArgNotValid ) == ISMRC_ArgNotValid );
    /* Test the name value cannot be started with a number or special character */
    CU_ASSERT( validate_name( "TestConfig", "123Name", "256", ISMRC_UnicodeNotValid ) == ISMRC_UnicodeNotValid );
    CU_ASSERT( validate_name( "TestConfig", "98765", "256", ISMRC_UnicodeNotValid ) == ISMRC_UnicodeNotValid );
    CU_ASSERT( validate_name( "TestConfig", "/987/tyui", "256", ISMRC_UnicodeNotValid ) == ISMRC_UnicodeNotValid );
    CU_ASSERT( validate_name( "TestConfig", "/topic/1", "256", ISMRC_UnicodeNotValid ) == ISMRC_UnicodeNotValid );

    /* Test the name value in correct format with maxlen */
    CU_ASSERT( validate_name( "TestConfig", "ABCDESdflkfjdkfjaklwek", "256", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_name( "TestConfig", "ABC_Config.name", "256", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_name( "TestConfig", "Subscription%llm", "256", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_name( "TestConfig", "Name1With23Num", "256", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_name( "TestConfig", "withnonsupportedcharacters$$$*^~", "256", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_name( "TestConfig", "The space is allowed", "256", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_name( "ChannelName", "channelNameCanBe20", "20", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_name( "QueueManagerName", "QueueManagerNameCanBe22queuemanger35AVCNDFDSD48", "48", ISMRC_OK ) == ISMRC_OK );

    return;
}

/*************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "IPADDRESS"   ---------------
 *
 *************************************************************************************************/
/* wrapper to call ism_config_validateDataType_IPAddresses() API */
static int32_t validate_IPAddress(char *name, char *value, int mode, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_IPAddresses(name, value, mode);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s mode=%d exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", mode, exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type IPAddress using ism_config_validateDataType_IPAddresses() API */
void test_validate_genericData_IPAddress(void)
{
    /* Test name cannot be NULL or empty*/
	CU_ASSERT( validate_IPAddress( NULL, "20", 0, ISMRC_NullPointer ) == ISMRC_NullPointer );
    CU_ASSERT( validate_IPAddress( "",   "true", 1, ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value cannot be NULL */
    CU_ASSERT( validate_IPAddress( "TestConfig", NULL, 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test mode cannot be a number other than 0,1 */
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85", 3, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85", -1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );

    /* Test IPv4 address must have right format */
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179 85", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179 85", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179,85", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179,85", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9 3 179 85", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9 3 179 85", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9,3,179,85", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9,3,179,85", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179,85", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179,85", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85.3", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85.3", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[9.3.179.85.3]", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[9.3.179.85.3]", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test IP address cannot be "*" when mode is 1 or contains "*"  */
    CU_ASSERT( validate_IPAddress( "TestConfig", "*", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "*fds", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "*:fe80::214:5eff:fefb", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9f*:fe80:214:5eff:fefb", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.*4.17.23", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "all", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test IPv6 address must have right format */
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80:179:214:5eff:fefb:77c8:0a23", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80:179:214:5eff:fefb:77c8:0a23", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80:179:214:5eff:fefb:77c8:0a23]", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80:179:214:5eff:fefb:77c8:0a23]", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "(fe80::214:5eff:fefb:77c8:0a23)", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "{fe80::214:5eff:fefb:77c8:0a23}", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test IP address range only support '-' as the delimiter */
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.83#9.3.179.85", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.83&9.3.179.85", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::214:5eff:fefb:77c8-fe80::214:5eff:fefb:735a", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::214:5eff:fefb:77c8%fe80::214:5eff:fefb:79db", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]-fe80::214:5eff:fefb:735a", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]^fe80::214:5eff:fefb:79db", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]-[fe80::214:5eff:fefb:735a]", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]@[fe80::214:5eff:fefb:79db]", 1, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /* Test IP address valiation doesn't support subnet mask */
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::214:5eff:fefb:77c8/128", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "2001:db8::/32", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::207:30ee:edcb:d05d/64", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85/24", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_IPAddress( "TestConfig", "10.10.0.85/64", 0, ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );

    /* Test IPv4 addresses list, range in correct format*/
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85", 1, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85,9.3.179.83", 1, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.85,9.3.179.83", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.83-9.3.179.85", 1, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "9.3.179.83-9.3.179.85", 0, ISMRC_OK ) == ISMRC_OK );
    /* Test "all", "*", and empty in correct format */
    CU_ASSERT( validate_IPAddress( "TestConfig", "*", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "all", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "", 0, ISMRC_OK ) == ISMRC_OK );
    /* Test IPv6 addresses list, range in correct format*/
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::214:5eff:fefb:77c8", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::214:5eff:fefb:77c8", 1, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]", 1, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::214:5eff:fefb:77c8-fe80::214:5eff:fefb:79db", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "fe80::214:5eff:fefb:77c8-fe80::214:5eff:fefb:79db", 1, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]-fe80::214:5eff:fefb:79db", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]-fe80::214:5eff:fefb:79db", 1, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]-[fe80::214:5eff:fefb:79db]", 0, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_IPAddress( "TestConfig", "[fe80::214:5eff:fefb:77c8]-[fe80::214:5eff:fefb:79db]", 1, ISMRC_OK ) == ISMRC_OK );
    /* Test localhost ip */
    CU_ASSERT( validate_IPAddress( "TestConfig", "127.0.0.1", 0, ISMRC_OK ) == ISMRC_OK );

    return;
}

/*************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "URL"   --------------
 *
 *************************************************************************************************/

/* wrapper to call ism_config_validateDataType_URL() API */
static int32_t validate_URL(char *name, char *value, char *maxlen, char *options, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_URL(name, value, maxlen, options, NULL);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s maxlen=%s options=%s exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", maxlen, options, exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type number using ism_config_validateDataType_URL() API */
void test_validate_genericData_URL(void)
{
	/* Test name cannot be NULL */
    CU_ASSERT( validate_URL( NULL, "https://9.3.179.85", "100", "scp://,ftp://", ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value cannot be NULL */
    // CU_ASSERT( validate_URL( "Destination", NULL, "100", "scp://,ftp://", ISMRC_OK ) == ISMRC_OK );

    /* Test support cases */
    /* LDAP URL */
    CU_ASSERT( validate_URL( "URL", "ldap://10.10.4.86", "2048", "ldap://,ldaps://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "URL", "ldaps://10.10.4.86", "2048", "ldap://,ldaps://", ISMRC_OK ) == ISMRC_OK );
    /*ResourceURL*/
    CU_ASSERT( validate_URL( "ResourceURL", "http://10.10.4.86", "2048", "http://,https://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "ResourceURL", "https://10.10.4.86", "2048", "http://,https://", ISMRC_OK ) == ISMRC_OK );
    /*UserInfoURL*/
    CU_ASSERT( validate_URL( "UserInfoURL", "http://10.10.4.86", "2048", "http://,https://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "UserInfoURL", "https://10.10.4.86", "2048", "http://,https://", ISMRC_OK ) == ISMRC_OK );
    /*TraceBackupDestination*/
    CU_ASSERT( validate_URL( "TraceBackupDestination", "ftp://10.10.4.86", "2048", "ftp://,scp://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "TraceBackupDestination", "scp://10.10.4.86", "2048", "ftp://,scp://", ISMRC_OK ) == ISMRC_OK );
    /*Destination, URI*/
    CU_ASSERT( validate_URL( "Destination", "ftp://10.10.4.86", "2048", "scp://,ftp://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "Destination", "scp://10.10.4.86", "2048", "scp://,ftp://", ISMRC_OK ) == ISMRC_OK );
    /*URI*/
    CU_ASSERT( validate_URL( "URI", "http://10.10.4.86", "2048", "http://,https://,ftp://,ftps://,ldap://,ldaps://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "URI", "https://10.10.4.86", "2048", "http://,https://,ftp://,ftps://,ldap://,ldaps://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "URI", "ftp://10.10.4.86", "2048", "http://,https://,ftp://,ftps://,ldap://,ldaps://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "URI", "ftps://10.10.4.86", "2048", "http://,https://,ftp://,ftps://,ldap://,ldaps://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "URI", "ldap://10.10.4.86", "2048", "ldap://,ldaps://,http://,https://,ftp://,ftps://", ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_URL( "URI", "ldaps://10.10.4.86", "2048", "ldap://,ldaps://,http://,https://,ftp://,ftps://", ISMRC_OK ) == ISMRC_OK );

    /*Negative tests*/
    /*LDAP URL*/
    CU_ASSERT( validate_URL( "URL", "http://10.10.4.86", "2048", "ldap://,ldaps://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_URL( "URL", "ftp://10.10.4.86", "2048", "ldap://,ldaps://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /*ResourceURL*/
    CU_ASSERT( validate_URL( "ResourceURL", "ftp://10.10.4.86", "2048", "http://,https://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_URL( "ResourceURL", "scp://10.10.4.86", "2048", "http://,https://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /*UserInfoURL*/
    CU_ASSERT( validate_URL( "UserInfoURL", "ldap://10.10.4.86", "2048", "http://,https://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_URL( "UserInfoURL", "ftp://10.10.4.86", "2048", "http://,https://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /*TraceBackupDestination*/
    CU_ASSERT( validate_URL( "TraceBackupDestination", "http://10.10.4.86", "2048", "ftp://,scp://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_URL( "TraceBackupDestination", "https://10.10.4.86", "2048", "ftp://,scp://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /*Destination, URI*/
    CU_ASSERT( validate_URL( "Destination", "ldap://10.10.4.86", "2048", "ftp://,scp://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    CU_ASSERT( validate_URL( "Destination", "http://10.10.4.86", "2048", "ftp://,scp://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /*URI*/
    CU_ASSERT( validate_URL( "URI", "scp://10.10.4.86", "2048", "ldap://,ldaps://,http://,https://,ftp://,ftps://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /*too short*/
    CU_ASSERT( validate_URL( "WhateverName", "htps", "2048", "http://,https://", ISMRC_BadPropertyValue ) == ISMRC_BadPropertyValue );
    /*Characters not allow -- skip this test since unicode will change this file to UTF-8 and mess up with the project .settings*/
    CU_ASSERT( validate_URL( "TraceBackupDestination", "scp://10.10.4.&#00986", "2048", "ftp://,scp://", ISMRC_OK ) == ISMRC_OK );

}

/*************************************************************************************************
 *
 * -------------   CUNIT TEST FUNCTIONS FOR VALIDATING GENERIC DATA TYPE "Selector"   --------------
 *
 *************************************************************************************************/

/* wrapper to call ism_config_validateDataType_Selector() API */
static int32_t validate_Selector(char *name, char *value, char *maxlen, char *item, int32_t exprc)
{
    int32_t rc = ism_config_validateDataType_Selector(name, value, maxlen, item);
    testno += 1;
    if ( exprc != rc ) {
        printf("Test %2d Failed: name=%s value=%s maxlen=%s item=%s exprc=%d rc=%d\n",
            testno, name?name:"NULL", value?value:"NULL", maxlen, item?item:"NULL", exprc, rc);
    }
    return rc;
}

/* Cunit test for validating data type using ism_config_validateDataType_Selector() API */
void test_validate_genericData_Selector(void)
{
    /* Test name cannot be NULL */
    CU_ASSERT( validate_Selector( NULL, "JMS_Topic like 'test/topic/%'", "100", NULL, ISMRC_NullPointer ) == ISMRC_NullPointer );
    /* Test value can be NULL & blank */
    CU_ASSERT( validate_Selector( "TestPolicyName", NULL, "100", NULL, ISMRC_OK ) == ISMRC_OK );
    CU_ASSERT( validate_Selector( "TestPolicyName", "", "100", NULL, ISMRC_OK ) == ISMRC_OK );

    /* Test support cases */
    /* LIKE case */
    CU_ASSERT( validate_Selector( "TestPolicyName", "JMS_Topic like 'test/topic/%'", "4096", "DefaultSelectionRule", ISMRC_OK ) == ISMRC_OK );
    /* NOTE: Most cases covered by selector testing elsewhere */

    /*Negative tests*/
    /* Invalid string specification */
    CU_ASSERT( validate_Selector( "TestPolicyName", "JMS_Topic like NotAValidString", "0", "DefaultSelectionRule", ISMRC_LikeSyntax ) == ISMRC_LikeSyntax );
    /* String too long */
    CU_ASSERT( validate_Selector( "TestPolicyName", "JMS_Topic like 'test/topic/%'", "10", NULL, ISMRC_LenthLimitSingleton ) == ISMRC_LenthLimitSingleton );
    CU_ASSERT( validate_Selector( "TestPolicyName", "JMS_Topic like 'test/topic/%'", "10", "DefaultSelectionRule", ISMRC_LenthLimitExceed ) == ISMRC_LenthLimitExceed );
    /* Invalid UTF8 */
    CU_ASSERT( validate_Selector( "TestPolicyName", "\xf0\x80\x81\x82", "100", "DefaultSelectionRule", ISMRC_ObjectNotValid ) == ISMRC_ObjectNotValid );
}
