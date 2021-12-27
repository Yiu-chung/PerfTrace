#!/bin/bash
current=`date "+%Y-%m-%d %H:%M:%S"`
timeStamp=`date -d "$current" +%s`
currentTimeStamp=$((timeStamp*1000+10#`date "+%N"`/1000000)) #将current转换为时
间戳，精确到毫秒
echo $currentTimeStamp
./perftrace_cli -s 49.234.78.51 -c 100 -i 10ms
current=`date "+%Y-%m-%d %H:%M:%S"`
timeStamp=`date -d "$current" +%s`
currentTimeStamp=$((timeStamp*1000+10#`date "+%N"`/1000000)) #将current转换为时
间戳，精确到毫秒
echo $currentTimeStamp
