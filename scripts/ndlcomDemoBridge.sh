#!/bin/bash
#
# this is a demo-script on how to test and use the "ndlcomBridge" tool in real
# life. uses the hex-encoded pipe-interface to connect a number of different
# bridges. we can then observe how the messages get passed between them and see
# if the routing works as it should.
#
# is also a nice excercise on unix-tooling, signal, pipes and buffering...
#
# now even more advanced: define nodes with their ports in an associative
# bash-array, and the connections between nodes in an bash array. use magical
# bash trickery to resolve all this.
#
# NOTE: it is possible to create circles (bad) and also to  create two
# completely unconnected trees (stupid).
#
# TODO: how to code the expected outcome? script the test?
# - create hex-string encoded files, containing packets, piping them into the
#   ports and piping all unconnected ports into files. then compare them...?
# - having more intelligent nodes, which can send and receive pings...?
# - maybe go full-scale: add all tooling functions in "library script", code
#   the tests in a yaml-file, render tests with template-engine...?

set -e
set -u

# this is an bash array
declare -A nodes
# which deviceId has which pipe-interfaces, named by strings separated by
# spaces
nodes[1]="A B C"
nodes[2]="A B"
nodes[3]="A B"
nodes[4]="A B C"
nodes[5]="A B"
nodes[6]="A B C"
nodes[7]="A B"
nodes[8]="A B C"
nodes[9]="A"
nodes[10]="A B"

# which two deviceIds should be connected. the scripting later will sort out
# which interfaces to use for each connection and bail out if something does
# not work as specified here.
declare -a conns=("1 2" "2 3" "3 4" "4 5" "5 6" "4 7" "7 8" "8 9" "8 10")

# an array holding all nodes which have still open connections to choose from
# while setting up the network. at the beginning just a copy of the "nodes"
# array created before.
declare -n openConns=nodes

# the commands we gonna use
#
# TODO: make this file cmake-processed and put the actual binary directory in
BRIDGE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomBridge"
PRODUCE_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomPacketProducer"
CONSUME_COMMAND="./build/x86_64-linux-gnu/tools/ndlcomPacketConsumer"

function cleanup_handler {
    # remove trap to prevent recursion
    trap - INT TERM
    # the final kill will stop this script as well
    kill 0
}
# a trap to cleanup all background-child processes on script exit
#
# see http://stackoverflow.com/a/22644006/4658481
trap exit INT TERM
# send SIGINT (ctrl-c) to every process in our process group on exit
trap cleanup_handler EXIT

GRAPHVIZ_FILE=tree.dot
graphviz_cluster_counter=0

cat << EOF > "$GRAPHVIZ_FILE"
digraph {
EOF

# tooling function to launch two "tail" processes which shuffle bytes between
# the rx and tx sides of the two given named pipes.
#
# also does some argument validation
#
# we ignore "stderr" of "tail" because the bridges might unlink the pipe during
# shutdown of this script, before the tails exit. this would generate an
# annoying but meaningless error-message.
#
# NOTE: see the "socat" tool, which does a lot of what we need here
connect_two_named_pipes() {
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
    # process-group. someone needs to tell them to close upon script-exit.
    # using "stdbuf" to prevent stale bytes in some of the kernel-level buffers
    # from not beeing processed

    # additionally, append this information to the graphviz file
cat << EOF >> "$GRAPHVIZ_FILE"
    "${onePipe}_tx" -> "${twoPipe}_rx";
    "${twoPipe}_tx" -> "${onePipe}_rx";
EOF
}

# launch ndlcomBridge in the background with given deviceId and
# interface-names. the correct commandline to crate the given named pipes is
# crafted and given to the bridge.
launch_bridge() {
    local deviceId="$1"
    local interfaces="$2"

    #echo "called with '$deviceId' and '$interfaces'"

    local uri=""
    for inter in ${interfaces};
        do uri="$uri -u pipe://pipe_${deviceId}_${inter}"
    done
    echo "launching ndlcomBridge -i $deviceId $uri"
    # given "-O" to print all messages directed at this bridge
    eval "$BRIDGE_COMMAND -i $deviceId $uri -O &"
    
    # additionally, append this information to the graphviz file
cat << EOF >> "$GRAPHVIZ_FILE"
    subgraph cluster_${graphviz_cluster_counter} {
        label="ndlcomBridge\nID: $deviceId";
EOF
    graphviz_cluster_counter=$((graphviz_cluster_counter+1))
    for inter in ${interfaces}; do
cat << EOF >> "$GRAPHVIZ_FILE"
        subgraph cluster_${graphviz_cluster_counter} {
        label="pipe_${deviceId}_${inter}"
            style=filled;color=lightgrey;
            "pipe_${deviceId}_${inter}_rx" [label="rx"];
            "pipe_${deviceId}_${inter}_tx" [label="tx"];
        }
EOF
    graphviz_cluster_counter=$((graphviz_cluster_counter+1))
    done
cat << EOF >> "$GRAPHVIZ_FILE"
    }
EOF
}

# this function is the real bash-magic...
#
# will delete the two corresponding entries from the "openConns" array when
# connecting the pipes of two already existing bridges by looking them up via
# their deviceId.
connected_bridges() {
    local firstDeviceId="$1"
    local secondDeviceId="$2"

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
    connect_two_named_pipes pipe_${firstDeviceId}_$firstConn pipe_${secondDeviceId}_$secondConn
}


for id in "${!nodes[@]}"; do launch_bridge $id "${nodes[$id]}"; done

sleep 0.1

for c in "${conns[@]}"; do connected_bridges $c; done

sleep 0.1

# correct printing of messages
$CONSUME_COMMAND < pipe_6_C_tx &
$CONSUME_COMMAND < pipe_1_C_tx &
$CONSUME_COMMAND < pipe_10_B_tx &

# now we should see the four prints from the four bridges.
#
# NOTE: we have to know where to write to
#
# by having one consumer on "1_C" and one on "10_B", both sould receive the
# broadcast. the third consumer is on "6_C" itself, so it should stay silent.
$PRODUCE_COMMAND -s 99 -r 255 > pipe_6_C_rx

sleep 0.1

# after sending a message with senderId 99 on "6_C", filling the routing
# tables, this consumer (and only this one) should receive messages to 99
$PRODUCE_COMMAND -s 190 -r 99 > pipe_1_C_rx

# wait some bit for all the buffers to empty
sleep 0.1

# end finish the graphviz picture
cat << EOF >> "$GRAPHVIZ_FILE"
}
EOF

dot < "$GRAPHVIZ_FILE" -Tpng > $(basename "$GRAPHVIZ_FILE" .dot).png

exit
