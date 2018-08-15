#!/bin/bash
#
# USAGE: $0 {[ON_TIME] [OFF_TIME]}
#
GPIO=18

ON_TIME=${1:-0.001}
OFF_TIME=${2:-0.1}

echo "USAGE: $0 {[ON_TIME] [OFF_TIME]}"

die() { echo "ERROR:$0:$@ !"; exit 1; }
cd /sys/class/gpio/
GPIO_dir="gpio${GPIO}"
[ ! -d $GPIO_dir ] && sudo echo $GPIO > export
sleep 0.5
[ ! -d $GPIO_dir ] && die "Cannot export GPIO $GPIO"
cd $GPIO_dir
sudo echo out > direction

trap "echo 0 > /sys/class/gpio/gpio${GPIO}/value; exit" INT

while :; do
	echo 1 > value
	sleep $ON_TIME
	echo 0 > value
       	sleep $OFF_TIME
done

