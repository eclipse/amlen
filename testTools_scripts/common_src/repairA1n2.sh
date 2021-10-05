#!/bin/bash

ssh A1_USER@A1_HOST imaserver start  2>&1 >repairA1n2.log
sleep 5
ssh A1_USER@A1_HOST imaserver update HighAvailability EnableHA=False  2>&1 >>repairA1n2.log
ssh A1_USER@A1_HOST imaserver stop force 2>&1 >>repairA1n2.log
ssh A1_USER@A1_HOST imaserver start 2>&1 >>repairA1n2.log
sleep 5
ssh A1_USER@A1_HOST status imaserver 2>&1 >>repairA1n2.log
echo beginning clean_store 1 2>&1 >>repairA1n2.log
../common/clean_store.sh clean 2>&1 >>repairA1n2.log
echo done clean_store 1 2>&1 >>repairA1n2.log
ssh A1_USER@A1_HOST status imaserver 2>&1 >>repairA1n2.log
ssh A2_USER@A2_HOST imaserver start 2>&1 >>repairA1n2.log
sleep 5
ssh A2_USER@A2_HOST imaserver update HighAvailability EnableHA=False 2>&1 >>repairA1n2.log
ssh A2_USER@A2_HOST imaserver stop force 2>&1 >>repairA1n2.log
ssh A2_USER@A2_HOST imaserver start 2>&1 >>repairA1n2.log
sleep 5
ssh A1_USER@A1_HOST status imaserver 2>&1 >>repairA1n2.log
echo beginning clean_store 2 2>&1 >>repairA1n2.log
../common/clean_store2.sh clean 2>&1 >>repairA1n2.log
echo done clean_store 2 2>&1 >>repairA1n2.log
ssh A2_USER@A2_HOST status imaserver 2>&1 >>repairA1n2.log
