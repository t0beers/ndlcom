#!/bin/bash
set -e

# this script grew rather large, but this is how it is.
#
# it helps to debug ndlcom-related stuff by throwing random bytes at the code
#
# functionality: send random bytes to a serial port. exits on errors.
# successfull exit on 'ctrl-c'. prints message on successfull exit.
#
# mzenzes 2015

# cmd-line options
SERIAL_PORT=${1:-/dev/ttyUSB1}
BAUDATE=${2:-921600}
# how much to saturate the serial port
LOAD_PERCENT=${3:-50}
# this "delay" des not work... somehow all the other calls to programs take way
# much more time than the actual delay calculated to match a baudrate...
DELAY_s=`echo " 1.0 / ("$BAUDATE" / 9.0) / ("$LOAD_PERCENT" / 100.0) "|bc -l`
DELAY_us=`echo "$DELAY_s * 1000.0 * 1000.0"|bc -l`
RATE_KBs=`echo "1.0 / $DELAY_s / 1024"|bc -l`

# a bash-builtin
START_TIME=$SECONDS

# safe old settings into variable. configure the serial port. this will fail
# the script via the 'set -e' if the port is not ok.
PREVIOUS_TTY_SETTINGS=`stty -g -F "$SERIAL_PORT"`
stty -F "$SERIAL_PORT" "$BAUDATE" -raw

# tell the world what is going on
printf "Using serialport '%s' with %ibaud.\n" "$SERIAL_PORT" "$BAUDATE"
#printf "Delay between bytes is %.2fus, which _should_ result in %.2fkB/s\n\n" $DELAY_us $RATE_KBs
printf "Press 'ctrl-c' to exit\n"

# we support printing of a nice message on exit!
exit_handler()
{
    trap - INT TERM EXIT
    # restoring previous settings
    stty -F "$SERIAL_PORT" "$PREVIOUS_TTY_SETTINGS"
    ELAPSED_TIME=$(($SECONDS - $START_TIME))
    printf "\n...sent %i random bytes in %is\n" $BYTE_COUNTER $ELAPSED_TIME
    exit
}
# now the trap is armed
trap exit_handler INT TERM EXIT

# the output should look like the one from "hexdump"
BYTE_COUNTER=0
while :
do
    # hexdump-like counter
    if [ $(( $BYTE_COUNTER % 16)) -eq 0 ]; then
        printf "%07x " $BYTE_COUNTER
    fi
    # getting a random number of the uint8_t range
    RAND_BYTE=$(($RANDOM % 256))
    # outputting it to the serial device. this call is overly slow!
    echo -ne "$(printf '\\x%x' $RAND_BYTE)" > "$SERIAL_PORT"
    # and printing of the same byte to the screen, just for reference
    printf "%02x" "$RAND_BYTE"
    # additionally we insert linebreaks and spaces once in a while
    BYTE_COUNTER=$(($BYTE_COUNTER+1))
    if [ $(( $BYTE_COUNTER % 16)) -eq 0 ]; then
        printf "\n"
    elif [ $(( $BYTE_COUNTER % 2)) -eq 0 ]; then
        printf ' ';
    fi
    # and delay, which also works with fractional numbers!
    #sleep $DELAY_s
    # exit-path of this loop is "ctrl-c"...
done

# there is actually no need to disarm the exit-handler, but this is added in
# case someone later edits this script
#trap - INT TERM EXIT
