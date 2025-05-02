#!/usr/bin/env perl
#
# dist_queue_runs.pl : run disttool -definebyquery for the various stages or one stage
#

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use Digest::MD5::File qw( file_md5_hex );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


# Look for programs we need
my $missing_tools;
my $disttool   = can_run('disttool') or (warn "Can't find disttool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($stage, $stage_limit, $dist_root, $workdir, $no_magic, $rerun);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);
my @labels;

GetOptions(
           'stage=s'        => \$stage,      # stage to queue
           'label=s'        => \@labels,     # labels
           'workdir=s'      => \$workdir,     # workdir
           'stage_limit=s'  => \$stage_limit,# maximum number of runs queued for each stage
           'rerun'          => \$rerun,      # queue new runs even if one exists
#           'dist_root=s'    => \$dist_root,  # root of distribution work area
           'no_magic'       => \$no_magic,   # queue runs without requiring magic (for testing only)
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database
           'pretend'        => \$no_update,  # Don't update the database
           'save-temps'     => \$save_temps, # Save temporary files?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --label",
           -exitval => 3) unless
    (scalar @labels > 0);

$ipprc->redirect_output($logfile) if $logfile;

if (!$workdir) {
    print "workdir not supplied will use DISTRIBUTION_ROOT\n";
    # old method where we set workdir based on a config file
    if (!$dist_root) {
        $dist_root = metadataLookupStr($ipprc->{_siteConfig}, "DISTRIBUTION_ROOT");
        &my_die("failed to find DISTRIBUTION_ROOT in site configuration", $PS_EXIT_CONFIG_ERROR) if !$dist_root;
    }

    my ($day, $month, $year) = (gmtime)[3,4,5];
    my $datestr = sprintf "%04d%02d%02d", $year+1900, $month + 1, $day;

    $workdir = $dist_root . "/$datestr";

    print "workdir is $workdir\n";
}

# if stage is not supplied as an argument, loop over all stages
my @stages;
if ($stage) {
    push @stages, $stage;
} else {
    @stages = qw(chip chip_bg camera fake warp warp_bg diff stack SSdiff sky skysingle ff);
}

foreach my $stage (@stages) {
    foreach my $label (@labels) {
        my $single;
        my $cmdstage;
        if ($stage eq "skysingle") {
            $cmdstage = "sky";
            $single = 1;
        } else {
            $cmdstage = $stage;
        }
        my $command = "$disttool -definebyquery -stage $cmdstage -workdir $workdir -label $label";
        $command .= " -singlefilter" if $single;
        $command .= " -no_magic" if $no_magic;
        $command .= " -rerun" if $rerun;
        $command .= " -pretend" if $no_update;
        $command .= " -limit $stage_limit" if $stage_limit;
        $command .= " -set_label $label";
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command error_code: $error_code", $error_code);
        }
        # display the output from the command
        print STDERR join "", @$stdout_buf if $verbose;
    }
}

if (0) {
# notyet
# queue rcRuns for any distRuns that have completed and have interested destinations
    my $command = "$disttool -queuercrun";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command error_code: $error_code", $error_code);
    }
    # display the output from the command
    print STDERR join "", @$stdout_buf if $verbose;
}

exit 0;

sub my_die {
    my $msg = shift;
    my $fault = shift;

    print STDERR "$msg\n";

    exit $fault;
}

__END__
