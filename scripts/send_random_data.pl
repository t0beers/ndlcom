#!/usr/bin/perl -w

# sudo apt-get install libdevice-serialport-perl
#
# this script helps to debug NDLCom-related stuff. It throws random bytes at
# the code.
#
# Functionality: Send random bytes to a serial port. Exit on errors.
# successful exit on 'ctrl-c'. Prints message on successful exit. Echos all
# written bytes on the terminal in the same format which hexdump uses.
#
# mzenzes 2015

use strict;
use Device::SerialPort;
use Time::HiRes qw(time usleep clock_gettime CLOCK_MONOTONIC);

# options, change as you wish
my $port = "/dev/ttyUSB1";
my $baudrate = 921600;
my $load_percent = 85;

# additional preparations
my $delay_between_byte_s = 1.0 / ($baudrate / 9.0) / ($load_percent / 100.0);
my $byte_counter = 0;
my $bytes_per_row = 16;

# create and configure the port
my $ob = Device::SerialPort->new($port);
$ob->baudrate($baudrate);
$ob->write_settings;

# need the "start time" for rate limiting
my $script_start_time = clock_gettime(CLOCK_MONOTONIC);

# print information in exit-handler, executed on "ctrl-c"
sub exit_handler{
    my $elapsed_time = clock_gettime(CLOCK_MONOTONIC) - $script_start_time;
    printf("\nwrote %.2fkB to '%s' in %.2fs (%.2fkB/s)\n",
        $byte_counter/1024.0, $port, $elapsed_time, ($byte_counter/1024.0/$elapsed_time));
    exit;
};
$SIG{INT} = \&exit_handler;

# exit with ctrl-c
while(1) {
    # print hex-counter for each line on the left, like hexdump does
    if (($byte_counter % $bytes_per_row) == 0) {
        printf("%07x ", $byte_counter);
    }

    # get and write random byte to port
    my $rand = int(rand(255));
    $ob->write(pack("C", $rand));
    # output to screen what we wrote
    printf("%02x", $rand);
    $byte_counter++;

    # after printing a number of bytes, proceed to the next row
    if (($byte_counter % $bytes_per_row) == 0) {
        printf("\n");

        # rate-limit to the given load_percent
        my $current_loop_time = clock_gettime(CLOCK_MONOTONIC);
        # to reach the given load, we would have to wait that much seconds
        my $remain_s = ($script_start_time + $delay_between_byte_s * $byte_counter)
                            - $current_loop_time;
        if ($remain_s > 0){
            # sleep as much as needed
            usleep( $remain_s * 1000.0 * 1000.0 );
        }

    } elsif (($byte_counter % 2) == 0) {
        # this will print the bytes in blocks of two, like hexdump does
        printf(" ");
    }
}
