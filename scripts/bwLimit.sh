#!/bin/bash

# tooling script to limit the bandwidth of data. needs "pv":
#
#     sudo apt-get install pv
#
# first start your normal "ndlcomBridge" with a special pipe-interface where
# the throughput will be rate limited by this script:
#
#     ./build/x86_64-linux-gnu/tools/ndlcomBridge \
#          -u serial:///dev/ttyUSB0 \
#          -u pipe:///tmp/first
#
# and then start this script. it will start a second ndlcomBridge and connect
# it to the "/tmp/first" pipe output of the normal ndlcomBridge. three possible
# arguments: the connection string to be used by the second bridge, the rate
# limit and the pipe-output of the normal ndlcomBridge:
#
#     ./scripts/bwLimit.sh udp://localhost:5000:5001 512K /tmp/first
#
# shown here are the defaults, so...
#
# also, feel free to adopt to your needs ;-)
#

set -e
set -u
set -o pipefail

BRIDGE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomBridge"

# string given to the second bridge as binary rate-limited output channel
ACTUAL_OUTPUT=${1:-udp://localhost:5000:5001}
# the user shall give a limit in "bytes per second", with optional "iec" suffixes
BW_LIMIT=${2:-512K}
# this is converted to a "bytes" value without a suffix
RAW_BW_LIMIT=$(echo ${BW_LIMIT} | numfmt --from=iec)
# as we limit on a "pipe" interface, where each byte is formatted as
# ascii-string like "0x93", the actual number of bytes per second is threefold.
# calculate this:
ACTUAL_BW_LIMIT=$(echo "$RAW_BW_LIMIT * 3"|bc -l)
# where the unlimited ascii-formatted data is read from
FIRST_PIPE=${3:-/tmp/first}
# this is the internal name of a pipe-interface used by the second bridge to
# read the ascii-formatted data after rate-limiting:
SECOND_PIPE=/tmp/second

# thats it, the rest is just script:

function cleanup_handler {
    # clear the trap to prevent loops
    trap - EXIT INT TERM
    echo "cleaning up"

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

${BRIDGE_COMMAND} -u pipe://${SECOND_PIPE} -u "${ACTUAL_OUTPUT}" -m pipe:///tmp/out &
sleep 1

echo "connecting the two pipes ratelimited for ${BW_LIMIT} (actually ${ACTUAL_BW_LIMIT} characters/s)"
stdbuf -o0 tail -q -n +1 -f "${FIRST_PIPE}"_tx | pv --quiet --rate-limit ${ACTUAL_BW_LIMIT}  > "${SECOND_PIPE}"_rx &
stdbuf -o0 tail -q -n +1 -f "${SECOND_PIPE}"_tx | pv --quiet --rate-limit ${ACTUAL_BW_LIMIT}  > "${FIRST_PIPE}"_rx &

echo "pressing enter will finish"
read

echo "done!"

exit
