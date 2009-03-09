#!/bin/bash

ifconfig bond0 down
ifconfig bond0 hw ether 00:1c:f0:ed:98:5a
ifconfig bond0 up
ifconfig bond0
cat /proc/net/bonding/bond0

# Initialize association and active slave
./test-hoolock-init bond0 0

iwconfig ath0

for i in `seq 1 10`; do
	./test-hoolock-once bond0 1
	sleep(10)
	./test-hoolock-once bond0 0
	sleep(10)
done

