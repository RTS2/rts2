#!/bin/bash

# SCript to execute random pointing from bash through rts2-json
# Please use with care, change username and password.
#
# (C) 2015 Petr Kubánek <petr@kubanek.net>

while true; do
	rts2-json -s T0.ORI="`rts2-telmodeltest -r`" --user <username> --password <password> --verbose
	sleep 10
done
