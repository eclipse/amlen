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

#include <ismutil.h>


int main(void) {

   /*Create hash map */
   ismHashMap * hMap;
   xUNUSED int rc;
   char * key = "key1";
   char * value ="value1";


   char * value2 ="value2";



   char * value3 ="value3";

   char * result ;
   char  * previous_value=NULL;
   int failed = 0;

   int count;


   hMap = ism_common_createHashMap(10,HASH_STRING);

   rc = ism_common_putHashMapElement(hMap,key, 0, value, (void *)&previous_value);
   result = ism_common_getHashMapElement(hMap,key, 0);

   /*Check Value*/
   if(strcmp(value, result)){
    printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",value, result, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
   }
   if(previous_value!=NULL){
    printf("Test Failed: Expected Result: NULL. Got: %s. Line:%d.\n",previous_value, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: Previous Value is NULL. Line: %d.\n", __LINE__);
   }

   previous_value=NULL;

   rc = ism_common_putHashMapElementLock(hMap,key, 0, value2,(void *)&previous_value);
   result = ism_common_getHashMapElementLock(hMap,key, 0);
   /*Check Value*/
   if(strcmp(value2, result)){
    printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",value2, result, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
   }
   if(strcmp(value, previous_value)){
    printf("Test Failed: Expected Result: Previous_value=%s. Got: %s. Line:%d.\n",value, previous_value, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: Previous Value is %s. Line: %d.\n", previous_value,  __LINE__);
   }

    previous_value=NULL;
   rc = ism_common_putHashMapElementLock(hMap,key, 0, value3,(void *) &previous_value);
   result = ism_common_getHashMapElementLock(hMap,key, 0);

   /*Check Value*/
   if(strcmp(value3, result)){
    printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",value3, result, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
   }
   if(strcmp(value2, previous_value)){
    printf("Test Failed: Expected Result: Previous_value=%s. Got: %s. Line:%d.\n",value, previous_value, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: Previous Value is %s. Line: %d.\n", previous_value,  __LINE__);
   }
   previous_value=NULL;
   rc = ism_common_putHashMapElementLock(hMap,key, 0, value, (void *)&previous_value);
   result = ism_common_removeHashMapElement(hMap,key, 0);

   /*Check Value*/
   if(strcmp(value, result)){
    printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",value, result, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
   }
   if(strcmp(value3, previous_value)){
    printf("Test Failed: Expected Result: Previous_value=%s. Got: %s. Line:%d.\n",value, previous_value, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: Previous Value is %s. Line: %d.\n", previous_value,  __LINE__);
   }
   previous_value=NULL;
   rc = ism_common_putHashMapElementLock(hMap,key, 0, value2,(void *) &previous_value);
   result = ism_common_removeHashMapElement(hMap,key, 0);

   /*Check Value*/
   if(strcmp(value2, result)){
    printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",value, result, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
   }
   if(previous_value!=NULL){
    printf("Test Failed: Expected Result: Previous_value=NULL. Got: %s. Line:%d.\n",previous_value, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: Previous Value is NULL. Line: %d.\n",__LINE__);
   }
   previous_value=NULL;
   rc = ism_common_putHashMapElementLock(hMap,key, 0, value3,(void *) &previous_value);
   result = ism_common_removeHashMapElement(hMap,key, 0);

   /*Check Value*/
   if(strcmp(value3, result)){
    printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",value, result, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
   }
   if(previous_value!=NULL){
    printf("Test Failed: Expected Result: Previous_value=NULL. Got: %s. Line:%d.\n",previous_value, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: Previous Value is NULL. Line: %d.\n",   __LINE__);
   }


   /*Test if the previous value param is NULL*/
   rc = ism_common_putHashMapElementLock(hMap,key, 0, value3, NULL);
   result = ism_common_removeHashMapElement(hMap,key, 0);

   /*Check Value*/
   if(strcmp(value3, result)){
    printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",value, result, __LINE__ );
    failed = 1;
   }else{
    printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
   }



   /*Test resize function */

   for(count=1; count < 1000000; count++)
   {
        char vkey[256];

        memset(&vkey,0 ,256);
        char *vkey_ptr= (char *)&vkey;
        char *vvalue_ptr= (char *)malloc(256);
        sprintf(vkey_ptr, "key%d", count);
        sprintf(vvalue_ptr, "value%d", count);

        ism_common_putHashMapElementLock(hMap,vkey_ptr, 0, vvalue_ptr,(void *) &previous_value);


   }
   for(count=1; count < 1000000; count++)
   {
        char vkey[256];

        memset(&vkey,0 ,256);
        char *vkey_ptr= (char *)&vkey;
        char *vvalue_ptr= (char *)malloc(256);
        sprintf(vkey_ptr, "key%d", count);
        sprintf(vvalue_ptr, "valueNew%d", count);

        ism_common_putHashMapElementLock(hMap,vkey_ptr, 0, vvalue_ptr,(void *) &previous_value);



   }

   /*Test the replace of value*/
   for(count=1; count < 1000000; count++)
   {
        char vkey[256];
        char vvalue[256];
        char *vkey_ptr=(char *) &vkey;
        char *vvalue_ptr= (char *)&vvalue;
        memset(&vkey,0 ,256);
        sprintf(vkey_ptr, "key%d", count);
         sprintf(vvalue_ptr, "valueNew%d", count);


        result = ism_common_getHashMapElementLock(hMap,vkey_ptr, 0);

        if(strcmp(vvalue_ptr, result)){
        printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",vvalue_ptr, result, __LINE__ );
        failed = 1;
       }else{
       // printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
       }

   }

    for(count=1; count < 1000000; count++)
   {
        char vkey[256];
        char vvalue[256];
        char *vkey_ptr=(char *) &vkey;
        char *vvalue_ptr= (char *)&vvalue;
        memset(&vkey,0 ,256);
        sprintf(vkey_ptr, "key%d", count);
         sprintf(vvalue_ptr, "valueNew%d", count);


        result = ism_common_removeHashMapElementLock(hMap,vkey_ptr, 0);

        if(strcmp(vvalue_ptr, result)){
        printf("Test Failed: Expected Result: %s. Got: %s. Line: %d\n",vvalue_ptr, result, __LINE__ );
        failed = 1;
       }else{
        //printf("Test Passed: Result: %s. Line: %d\n", result, __LINE__);
       }

   }


   ism_common_destroyHashMap(hMap);

    if(failed==1) {
        printf("Test Failed.\n");
    } else {
        printf("Test Passed.\n");
    }
    return 0;
}
