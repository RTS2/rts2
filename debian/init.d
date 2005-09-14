#!/bin/bash
#
# Start/stops rts2 deamons (rts2-centrald, rts2-teld-xxx etc..)
#
# Writtent by Petr Kubanek <pkubanek@asu.cas.cz>, based on Samba init.d
#

CENTRALD_FILE=/etc/rts2/centrald
DEVICE_CONFIG=/etc/rts2/devices
SERVICE_CONFIG=/etc/rts2/services
RTS2_BIN_PREFIX=/usr/local/bin
RTS2_PID_PREFIX=/var/run/rts2_

function rts2-start
{
	if grep '^centrald' $CENTRALD_FILE 2>&1 > /dev/null; then
		grep '^centrald' $CENTRALD_FILE | read centrald centrald_options
		echo -n "Starting RTS2 centrald daemon:"
		start-stop-daemon --start --exec $RTS2_BIN_PREFIX/rts2-centrald -- $centrald_options
		echo "rts2-centrald."
	fi
	CENTRALD_OPTIONS=`grep '^-' $CENTRALD_FILE 2>/dev/null`
	echo -n "Starting RTS2 deamons:"
	cat $DEVICE_CONFIG | sed -e 's/#.*$//' | egrep -v '^\s*$' | while read device type device_name options; do
		echo -n " rts2-$device-$type($device_name)"
		start-stop-daemon --start --pidfile ${RTS2_PID_PREFIX}${device_name} --exec $RTS2_BIN_PREFIX/rts2-$device-$type -- $options $CENTRALD_OPTIONS -d $device_name
	done
	echo "."
	echo -n "Starting RTS2 services:"
	cat $SERVICE_CONFIG | sed -e 's/#.*$//' | egrep -v '^\s*$' | while read service service_name options; do
		echo -n " rts2-$service($service_name)"
		start-stop-daemon --start --pidfile ${RTS2_PID_PREFIX}${service_name} --exec $RTS2_BIN_PREFIX/rts2-$service -- $options $CENTRALD_OPTIONS -d $service_name
	done
	echo "."
}

function rts2-stop
{
	echo -n "Stopping RTS2 daemons:"
	cat $DEVICE_CONFIG | sed -e 's/#.*$//' | egrep -v '^\s*$' | while read device type device_name options; do
		echo -n " rts2-$device-$type($device_name)"
		start-stop-daemon --stop --pidfile ${RTS2_PID_PREFIX}${device_name}
	done
	echo "."
	echo -n "Stopping RTS2 services:"
	cat $SERVICE_CONFIG | sed -e 's/#.*$//' | egrep -v '^\s*$' | while read service service_name options; do
		echo -n " rts2-$service($service_name)"
		start-stop-daemon --stop --pidfile ${RTS2_PID_PREFIX}${service_name}
	done
	echo "."
}

case "$1" in
	start)
		rts2-start
		;;
	stop)
		rts2-stop
		;;
	restart)
		rts2-stop
		rts2-start
		;;
	stop-centrald)
		rts2-stop
		echo -n "Stopping RTS2 centrald daemon:"
		start-stop-daemon --stop --exec $RTS2_BIN_PREFIX/rts2-centrald
		echo "rts2-centrald."
		;;
	*)
		echo "Usage /etc/init.d/rts2 {start|stop|stop-centrald|restart}"
		exit 1
		;;
esac

exit 0
