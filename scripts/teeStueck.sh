#!/bin/bash

# tooling script to print all messages going from "/tmp/first" to "/tmp/second"
# and vice versa.
#
# start first tool like this:
#
# ./build/x86_64-linux-gnu/tools/ndlcomBridge \
#      -u serial:///dev/ttyUSB0 \
#      -u pipe:///tmp/first
#
# and second one like this:
#
# ./build/x86_64-linux-gnu/tools/ndlcomBridge \
#      -u pipe:///tmp/second
#
# and finally this script like so:
#
# ./scripts/teeStueck.sh /tmp/first /tmp/second
#
# will then use the "ndlcomPacketConsumer" to print some information about
# every passing packet.

set -e
set -u
set -o pipefail

BRIDGE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomBridge"
PRODUCE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomPacketProducer"
CONSUME_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomPacketConsumer"

FIRST_PIPE=${1:-/tmp/first}
SECOND_PIPE=${2:-/tmp/second}
FIRST_TO_SECOND="/tmp/first_to_second"
SECOND_TO_FIRST="/tmp/second_to_first"

function cleanup_handler {
    # clear the trap to prevent loops
    trap - EXIT INT TERM
    echo "cleaning up"
    if [ -p "${FIRST_TO_SECOND}" ]; then
        rm -v "${FIRST_TO_SECOND}"
    fi
    if [ -p "${SECOND_TO_FIRST}" ]; then
        rm -v "${SECOND_TO_FIRST}"
    fi
    echo "exiting"
    # pitfall: this will kill the whole process group -- so also this script!
    # every line hereafter is not executed anymore
    kill 0
}

# a trap to cleanup all background-child processes on script exit
#
# see http://stackoverflow.com/a/22644006/4658481
trap exit INT TERM
# send SIGINT (ctrl-c) to every process in our process group on exit
trap cleanup_handler EXIT

if [ ! -p "${FIRST_TO_SECOND}" ]; then
    mkfifo "${FIRST_TO_SECOND}"
fi
if [ ! -p "${SECOND_TO_FIRST}" ]; then
    mkfifo "${SECOND_TO_FIRST}"
fi

echo "connecting the two pipes"
stdbuf -o0 tail -q -n +1 -f "${FIRST_PIPE}"_tx | tee "${FIRST_TO_SECOND}"  > "${SECOND_PIPE}"_rx &
stdbuf -o0 tail -q -n +1 -f "${SECOND_PIPE}"_tx | tee "${SECOND_TO_FIRST}"  > "${FIRST_PIPE}"_rx &

echo "starting two consumers"
"$CONSUME_COMMAND" < "${FIRST_TO_SECOND}" &
"$CONSUME_COMMAND" < "${SECOND_TO_FIRST}" &

echo "pressing enter will finish"
read

echo "done!"

exit
