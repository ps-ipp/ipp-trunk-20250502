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
my $very_verbose = 0;
my $save_temps;
my $preserve_days;
my $dbname;
my $dbserver;
my $imagedb;
my $label;

GetOptions(
    'preserve-days=s'       => \$preserve_days,     # clean up requests that stopped this many days ago
    'verbose'               => \$verbose,
    'save-temps'            => \$save_temps,
    'imagedb=s'             => \$imagedb,
    'label=s'               => \$label,
    'dbserver=s'            => \$dbserver,
    'dbname=s'              => \$dbname,
) or pod2usage( 2 );

my $missing_tools;
my $pstamptool   = can_run('pstamptool') 
    or (warn "Can't find pstamptool" and $missing_tools = 1);
my $chiptool   = can_run('chiptool') 
    or (warn "Can't find chiptool" and $missing_tools = 1);
my $warptool   = can_run('warptool') 
    or (warn "Can't find warptool" and $missing_tools = 1);
my $difftool   = can_run('difftool') 
    or (warn "Can't find difftool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

if (!$imagedb)  {
    $imagedb = 'gpc1';
}

if (!$label)  {
    $label = 'ps_ud%';
}

my $ipprc = PS::IPP::Config->new(); # IPP Configuration

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}
$pstamptool .= " -dbserver $dbserver" if $dbserver;

if (!$dbname) {
    $dbname =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBNAME');
}
$pstamptool .= " -dbname $dbname" if $dbname;


my $command = "$pstamptool -pendingdependent -includefaulted -simple";
my  ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $command, verbose => $very_verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or 1);
    warn("$command failed. exit status: $error_code");
    exit $error_code;
}

my @lines = split "\n", (join "", @$stdout_buf);
my $nlines = scalar @lines;

if ($nlines) {
    # printing to stderr so that it shows up in the task's log file. stdout is set to null
    print STDERR "Skipping cleanup of update labels because there are currently $nlines dependents outstanding.\n";
} else  {
    print STDERR "Queuing cleanup of update labels because there are no dependents outstanding.\n";
    foreach my $tool ($chiptool, $warptool, $difftool) {
        my $command = "$tool -updaterun -set_state goto_cleaned -set_label goto_cleaned -label $label";
        $command .= " -dbname $imagedb";

        my  ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $very_verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or 1);
            warn("$command failed. exit status: $error_code");
            exit $error_code;
        }
    }
}

exit 0;
