#!/bin/env perl
#
# run pstamptool to queue requests older than preserve-days to be cleaned

use strict;
use warnings;

use PS::IPP::Config 1.01 qw( :standard );
use IPC::Cmd 0.36 qw( can_run run );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my $verbose;
my $save_temps;
my $preserve_days;
my $dbname;
my $dbserver;

GetOptions(
    'preserve-days=s'       => \$preserve_days,     # clean up requests that stopped this many days ago
    'verbose'               => \$verbose,
    'save-temps'            => \$save_temps,
    'dbserver=s'            => \$dbserver,
    'dbname=s'              => \$dbname,
) or pod2usage( 2 );

my $missing_tools;
my $pstamptool   = can_run('pstamptool') 
    or (warn "Can't find pstamptool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new(); # IPP Configuration

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
    $pstamptool .= " -dbserver $dbserver" if $dbserver;
}

if (!$dbname) {
    $dbname =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBNAME');
    $pstamptool .= " -dbname $dbname" if $dbname;
}



if (!$preserve_days) {
    $preserve_days = metadataLookupStr($ipprc->{_siteConfig}, 'PSTAMP_PRESERVE_DAYS');
    if (!$preserve_days) {
        $preserve_days = 14;
    }
}

if ($preserve_days < 0) {
    warn("preserve-days cannot be negative\n" );
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $ticks = time();
$ticks -= 86400 * $preserve_days;

my ($sec, $min, $hour, $mday, $month, $year) = gmtime($ticks);
$year += 1900;
$month += 1;

my $timestamp_end = sprintf "%4d-%02d-%02dT%02d:%02d:%02d", $year, $month, $mday, $hour, $min, $sec;

my $command = "$pstamptool -updatereq -set_state goto_cleaned -state stop -timestamp_end $timestamp_end";
my  ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $command, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or 1);
    warn("$command failed. exit status: $error_code");
    exit $error_code;
}


exit 0;
