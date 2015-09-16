#!/bin/bash
#
# this is a demo-script on how to test and use the "ndlcomBridge" tool in real
# life. uses the hex-encoded pipe-interface to connect a number of different
# bridges. we can then observe how the messages get passed between them and see
# if the routing works as it should.
#
# is also a nice excercise on unix-tooling, signal, pipes and buffering...
#
# TODO: build arbritrarily complex networks... maybe by somehow generating a
# random tree-layout?
# TODO: create unit-tests based on this by somehow checking expectancies
# against observed behaviour?

set -e

# cleanup all background-child processes on script exit
# see http://stackoverflow.com/a/22644006/4658481
trap "exit" INT TERM
# send 'ctrl-c' to every process in our process group
trap "kill -SIGINT 0" EXIT

# tooling function to launch two "tail" processes which shuffle bytes between
# the rx and tx sides of the two given pipes.
#
# also do proper argument validation
#
# we ignore "stderr" of "tail" because the bridges might unlink the pipe during
# shutdown of this script, before the tails exit. this would generate an
# annoying but meaningless error-message.
connect_pipes() {
    if [ $# -ne 2 ]; then
        echo "argcount is not 2, but $#"
        exit
    fi
    local onePipe="$1"
    local twoPipe="$2"
    if [ "${onePipe}" = "${twoPipe}" -o -z "${onePipe}" -o -z "${twoPipe}" ]; then
        echo "you kidding me?"
        exit
    fi
    if [ ! -p "${onePipe}_rx" -o ! -p "${onePipe}_tx" ]; then
        echo "given '${onePipe}' is not a proper rx/tx pipe"
        exit
    fi
    if [ ! -p "${twoPipe}_rx" -o ! -p "${twoPipe}_tx" ]; then
        echo "given '${twoPipe}' is not a proper rx/tx pipe"
        exit
    fi
    # and then do our work
    stdbuf -o0 tail -q -n +1 -f "${onePipe}_tx" > "${twoPipe}_rx" 2>/dev/null &
    stdbuf -o0 tail -q -n +1 -f "${twoPipe}_tx" > "${onePipe}_rx" 2>/dev/null &
    # note that the "tails" are in the background and still part of this
    # process-group. someone needs to tell them to exit upon script-exit.
}

# the command we gonne use
BRIDGE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomBridge"
PRODUCE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomPacketProducer"
CONSUME_COMMAND="stdbuf -i0 ./build/x86_64-linux-gnu/tools/ndlcomPacketConsumer"

# very easy:
#
# create one bridge and see if a message received on one port is echoed on the
# other
#$BRIDGE_COMMAND -i 3 -A -O -u pipe://pipe3A -u pipe://pipe3B &
## wait some bit
#sleep 0.1
#stdbuf -i0 -o0 $CONSUME_COMMAND < pipe3A_tx &
#$PRODUCE_COMMAND -s 7 -r 255   > pipe3B_rx
#sleep 1
#exit

# slightly more complex:
#
# connect two bridges and see if they exchange messages
#$BRIDGE_COMMAND -i 3 -A -O -u pipe://pipe3A -u pipe://pipe3B &
#$BRIDGE_COMMAND -i 4 -A -O -u pipe://pipe4A -u pipe://pipe4B &
## wait some bit
#sleep 0.1
#connect_pipes "pipe3A" "pipe4B"
#$CONSUME_COMMAND < pipe4A_tx &
#$PRODUCE_COMMAND -s 7 -r 3   > pipe3B_rx
#sleep 1
#exit

# really complicated:
#
# create three bridges forming a "Y" with one bridge in the middle (and has
# three interfaces) and two bridges both connected to the center (each with two
# interfaces).
#
# see if the routing table gets updated after a broadcast message passes
# through the bridge.
$BRIDGE_COMMAND -i 3 -A -O -u pipe://pipe3A -u pipe://pipe3B &
$BRIDGE_COMMAND -i 4 -A -O -u pipe://pipe4A -u pipe://pipe4B -u pipe://pipe4C &
$BRIDGE_COMMAND -i 5 -A -O -u pipe://pipe5A -u pipe://pipe5B &
# what for them to start and create their pipes
sleep 0.1

# connect two "outer" bridges to the center:
# first pair:
connect_pipes "pipe3A" "pipe4B"
# second pair:
connect_pipes "pipe5A" "pipe4C"

# listen to messages coming from the second (unconnected) port of the last
# (third) bridge
$CONSUME_COMMAND < pipe5B_tx &

# insert some messages to check that the routing table works
# first: test that if the receive is not known every interface receives the
# message. send to "7" into "3B". the consumer on "5B" should see and print the
# message.
$PRODUCE_COMMAND -s 9 -r 7   > pipe3B_rx
# next: make "7" known to everone as connected to "4B"
$PRODUCE_COMMAND -s 7 -r 255 > pipe4B_rx
# then: send to "7" again by using "3B", now there should be no more prints
# from the "consumer" listening "5B". the message got only sent to "4B"
$PRODUCE_COMMAND -s 9 -r 7   > pipe3B_rx

# wait some bit for all the buffers to empty
sleep 1

exit
