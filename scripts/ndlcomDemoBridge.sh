#!/bin/bash
#
# this is a demo-script on how to test and use the "ndlcomBridge" tool in real
# life. uses the hex-encoded pipe-interface to connect a number of different
# bridges. we can then observe how the messages get passed between them and see
# if the routing works as it should.
#
# is also a nice excercise on unix-tooling, signal, pipes and buffering...
#
# now even more advanced: define nodes with their pipes in associative array,
# and the connections between nodes in an array. use magical bash functions to
# resolve all this. NOTE: you can create circles (bad) and you can create two
# unconnected trees (stupid).

set -e
set -u

declare -A nodes
# which deviceId has which pipe-interfaces
nodes[1]="A B"
nodes[2]="A B"
nodes[3]="A B"
nodes[4]="A B C D E"

# which two deviceIds should be connected
declare -a conns=("1 4" "1 2" "3 4")

# which nodes have still open connections. at the beginning just a copy of the
# "nodes" from before
declare -n openConns=nodes

# the command we gonne use
BRIDGE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomBridge"
PRODUCE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomPacketProducer"
CONSUME_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomPacketConsumer"


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

# create commandline to start a bridge with the specified pipes
launch_node() {
    local deviceId="$1"
    local interfaces="$2"

    #echo "called with '$deviceId' and '$interfaces'"

    local uri=""
    for inter in ${interfaces}; do uri="$uri -u pipe://pipe_${deviceId}_${inter}"; done
    echo "would launch ndlcomBridge -i $deviceId $uri"
    eval "$BRIDGE_COMMAND -i $deviceId $uri -O &"
}

# will delete the two corresponding entries from the "openConns" array when
# connecting the pipes of two nodes
connect_nodes() {
    local firstDeviceId="$1"
    local secondDeviceId="$2"

    #echo "called with '$firstDeviceId' and '$secondDeviceId'"
    # find two entries in openConns
    #echo "have open ${openConns[$firstDeviceId]} and ${openConns[$secondDeviceId]}"

    # first port
    read -a ports <<< ${openConns[$firstDeviceId]}
    local firstConn=${ports[0]}
    openConns[$firstDeviceId]=""
    for el in "${!ports[@]}"; do
        if [ $el -gt 0 ]; then
            #echo "adding ${ports[$el]} back"
            openConns[$firstDeviceId]="${openConns[$firstDeviceId]} ${ports[$el]}"
        fi
    done

    # second port
    read -a ports <<< ${openConns[$secondDeviceId]}
    local secondConn=${ports[0]}
    openConns[$secondDeviceId]=""
    for el in "${!ports[@]}"; do
        if [ $el -gt 0 ]; then
            #echo "adding ${ports[$el]} back"
            openConns[$secondDeviceId]="${openConns[$secondDeviceId]} ${ports[$el]}"
        fi
    done

    echo "connecting 'pipe_${firstDeviceId}_$firstConn' to 'pipe_${secondDeviceId}_$secondConn'"
    connect_pipes pipe_${firstDeviceId}_$firstConn pipe_${secondDeviceId}_$secondConn
}


for id in "${!nodes[@]}"; do launch_node $id "${nodes[$id]}"; done

sleep 0.1

for c in "${conns[@]}"; do connect_nodes $c; done

sleep 0.1

$CONSUME_COMMAND < pipe_3_B_tx &

# now we should see the four prints from the four listeners. note that we have
# to know where to write to. also be cure that the port we use is not used
# between the bridges themselfes.
$PRODUCE_COMMAND -s 99 -r 255 > pipe_4_E_rx

# wait some bit for all the buffers to empty
sleep 0.1

exit
