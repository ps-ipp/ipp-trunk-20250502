#! /usr/bin/env perl

use warnings;
use strict;
use Sys::Hostname;
use DateTime;

my $sleep_period = 30;


my $host = hostname();
my $logdir = "/tmp/load_monitor";

my $current_date = 999;

while (1) {
    my $dt = DateTime->now;
    if ($dt->day != $current_date) {
	my $del_dt = DateTime->now;
	$del_dt->subtract( days => 4 );
	my $del_stamp = sprintf("%04d%02d%02d.\*",$del_dt->year,$del_dt->month,$del_dt->day);
	my_system("rm ${logdir}/load.${host}.$del_stamp");
    }
    
    my $stamp = sprintf("%04d%02d%02d.%02d%02d%02d",$dt->year,$dt->month,$dt->day,$dt->hour,$dt->minute,$dt->second);
    my $outfile = "${logdir}/load.${host}.$stamp";

    my_system("uptime > $outfile");
    my_system("ps aux >> $outfile");
 
    $current_date = $dt->day;
    sleep($sleep_period);
}
    
sub my_system {
    my $cmd = shift;
#    print ">>$cmd<<\n";
    system("$cmd");
}

