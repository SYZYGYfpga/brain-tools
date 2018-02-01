#!/bin/bash
# description: Enables SYZYGY SmartVIO at startup for the SYZYGY Brain-1

. /etc/init.d/functions

start() {
	# At this point the VIO supplies are off, adjust their VREF
	smartvio -r /dev/i2c-1

	# Set the enable lines high
	echo 906 > /sys/class/gpio/export
	echo 913 > /sys/class/gpio/export

	echo out > /sys/class/gpio/gpio906/direction
	echo out > /sys/class/gpio/gpio913/direction

	echo 1 > /sys/class/gpio/gpio906/value
	echo 1 > /sys/class/gpio/gpio913/value
}

stop() {
	# Turn off the VIO supplies, leave 3.3V as-is since it should always be on
	echo 0 > /sys/class/gpio/gpio906/value
	echo 0 > /sys/class/gpio/gpio913/value
}

case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		stop
		start
		;;
	status)
		
		;;
	*)
	echo "Usage: $0 {start|stop|status|restart}"
esac
