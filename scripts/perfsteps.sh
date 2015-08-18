#!/bin/bash
#
# very brief steps for obtained a flamegraph of a running process via its
# PID... not perfect, but nice enough
#

set -o errexit
set -o nounset

FLAMEGRAPH=$HOME/FlameGraph.git
TEMP_FILE=/tmp/out.perf-folded
SVG_FILE=/tmp/perf-flamegraph.svg
SAMPLE_FREQUENCY_HZ=1001
DURATION_S=20

if [ $# != 1 ]; then
    echo "give PID to watch as single argument"
    exit
fi

PID=${1}
if ! `kill -0 $PID > /dev/null 2>&1`; then
    echo "pid $PID not found"
    exit
fi

if [ ! -d $FLAMEGRAPH ]; then
    git clone https://github.com/brendangregg/FlameGraph $FLAMEGRAPH
else
    echo "using '$FLAMEGRAPH'"
fi

perf record -F $SAMPLE_FREQUENCY_HZ -p $PID -g -- sleep $DURATION_S
perf script | $FLAMEGRAPH/stackcollapse-perf.pl > $TEMP_FILE
$FLAMEGRAPH/flamegraph.pl $TEMP_FILE > $SVG_FILE

echo "displaying '$SVG_FILE'"
iceweasel $SVG_FILE
