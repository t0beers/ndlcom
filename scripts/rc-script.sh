#!/bin/bash

### BEGIN INIT INFO
# Provides:          ndlcomBridge
# Required-Start:    networking
# Required-Stop:     networking
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: This is an ndlcomBridge
# Description:       This is an ndlcomBridge. This particular description is a wee bit longer.
### END INIT INFO

###
#
#                                    ndlcomBridge
#
#
# This is a vorschlag for a "System-V" like  init-script for the "ndlcomBridge"
# to route ndlcom-messages between interfaces by default after system boot.
#
# To install and activate on every systemboot copy an adjusted version of this
# script (see below) to "/etc/init.d" and copy the binary to "/usr/local/bin".
# Like so:
#
#     cp build/*/tools/ndlcomBridge /usr/local/bin
#     cp ./scripts/rc-script.sh /etc/init.d/ndlcomBridge
#
# To manually handle the ndlcomBridge do
#
#     /etc/init.d/ndlcomBridge [start|stop|status]
#
# To install permanently:
#
#     update-rc.d ndlcomBridge defaults
#
# To alter the state temporarely:
#     
#     update-rc.d ndlcomBridge [disable|enable]
#
# To deinstall run
#
#     rm /etc/init.d/ndlcomBridge
#     update-rc.d ndlcomBridge remove
#
# This assumes that you know what you do!
#
###

###
# edit
###

# what to tell the bridge
BRIDGE_ARGUMENTS="-u udp://localhost&1,2 -u serial:///dev/ttyUSB0"

# the actual location of executable. edit this!
DIRECTORY_OF_THIS_SCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DAEMON="$DIRECTORY_OF_THIS_SCRIPT"/../build/x86_64-linux-gnu/tools/ndlcomBridge
#DAEMON=/usr/bin/ndlcomBridge

###
# TODO:
# - native systemd, anymone?
###

# a name, for beautiness
NAME=$(basename $DAEMON)

# the location for the pid-file
PIDFILE=/var/run/${NAME}.pid

# this will source the tooling function used down below
. /lib/lsb/init-functions

# test if the daemon is there
if [ ! -x "$DAEMON" ]; then
    log_daemon_msg "no deamon: '$DAEMON'"
    log_end_msg 1
    exit 5
fi

# check that we are allowed to do what we are asked for
if [ "$EUID" -ne 0 ]; then
    log_daemon_msg "this script needs root!"
    log_end_msg 1
    exit 1
fi


# and the main-case
case $1 in
start)
    status_of_proc -p $PIDFILE $DAEMON $NAME && exit $?
    # startit
    log_daemon_msg "starting" "$NAME"
    start-stop-daemon --background --make-pidfile --pidfile $PIDFILE \
        --exec $DAEMON --start -- $BRIDGE_ARGUMENTS
    log_end_msg $?
    ;;
stop)
    # stopit
    status_of_proc -p $PIDFILE $DAEMON $NAME || exit $?
    log_daemon_msg "stopping" "$NAME"
    start-stop-daemon --stop --remove-pidfile --pidfile $PIDFILE
    log_end_msg $?
    ;;
restart)
    # restarting is killing and reviving
    $0 stop && sleep 1 && $0 start
    ;;
status)
    # checkit
    status_of_proc -p $PIDFILE $DAEMON $NAME
    ;;
**)
    log_daemon_msg "unknown argument '$1'"
    log_end_msg 1
esac
