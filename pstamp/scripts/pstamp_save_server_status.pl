#!/bin/env perl
#
# run pstamp_server_status and save the results in a date stamped file
# optionally change the symlink in the web directory to point to the new file

use strict;
use warnings;

use PS::IPP::Config 1.01 qw( :standard );
use IPC::Cmd 0.36 qw( can_run run );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my $updatelink;
my $verbose;
my $save_temps;
my $pstamp_workdir;

GetOptions(
    'workdir=s'      => \$pstamp_workdir,
    'update-link'    => \$updatelink,
    'verbose'       => \$verbose,
    'save-temps'    => \$save_temps,
) or pod2usage( 2 );

my $missing_tools;
my $pstamp_server_status   = can_run('pstamp_server_status') 
    or (warn "Can't find pstamp_server_status" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

if (!$pstamp_workdir) {
    my $ipprc = PS::IPP::Config->new(); # IPP Configuration
    $pstamp_workdir = metadataLookupStr($ipprc->{_siteConfig}, 'PSTAMP_WORKDIR');
}

if (!$pstamp_workdir) {
    warn("Failed to find PSTAMP_WORKDIR in the config\n");
    exit($PS_EXIT_CONFIG_ERROR);
}
my $status_file = "$pstamp_workdir/server_status/status.html";

my ($sec, $min, $hour, $mday, $month, $year) = gmtime;
$year += 1900;
$month += 1;

my $dir = sprintf "$pstamp_workdir/server_status/%4d/%02d/%02d", $year, $month, $mday;

if (!-e $dir ) {
    my $rc = system "mkdir -p $dir";
    if ($rc) {
        my $status = $rc >> 8;
        print STDERR "mkdir failed error: $status\n";
        exit $status;
    }
}

my $file = sprintf "$dir/pstamp.status.%4d-%02d-%02dT%02d:%02d:%02d.html",
    $year, $month, $mday, $hour, $min, $sec;

my $command = "pstamp_server_status --workdir $pstamp_workdir > $file";
#my $command = "pstamp_server_status > $file";
my  ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $command, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or 1);
    warn("$command failed. exit status: $error_code");
    exit $error_code;
}

if ($updatelink) {
    if (-e $status_file) {
        unlink $status_file or die "failed to unlink existing status file $status_file\n";
    }

    symlink $file, $status_file or die "failed to update staus file link $status_file";
}

exit 0;
