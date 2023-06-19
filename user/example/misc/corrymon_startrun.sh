#!/usr/bin/env sh
BINPATH=../../../bin
#$BINPATH/euRun &
$BINPATH/euRun -n Ex1RunControl &
sleep 1
$BINPATH/euLog &
sleep 1
$BINPATH/euCliMonitor -n CorryMonitor -t my_mon &
#$BINPATH/euCliMonitor -n Ex0Monitor -t my_mon &
$BINPATH/euCliCollector -n Ex0TgDataCollector -t my_dc0 &
#$BINPATH/euCliCollector -n Ex1TgDataCollector -t my_dc1 &
$BINPATH/euCliProducer -n Ex0Producer -t my_pd0 &
#$BINPATH/euCliProducer -n Ex1Producer -t my_pd1 &
