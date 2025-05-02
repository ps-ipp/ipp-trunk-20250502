#!/usr/bin/env perl
#
# dist_defineruns.pl : run magicdstool -definebyquery for the various stages
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
my $magicdstool   = can_run('magicdstool') or (warn "Can't find magicdstool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($stage, $stage_limit, $workdir, $recoveryroot);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);
my @labels;

GetOptions(
           'stage=s'        => \$stage,      # stage to queue
           'label=s'        => \@labels,     # labels
           'stage_limit=s'  => \$stage_limit,# maximum number of runs queued for each stage
           'workdir=s'      => \$workdir,    # output destination
           'recoveryroot=s' => \$recoveryroot, # recovery pixels destination
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database
           'save-temps'     => \$save_temps, # Save temporary files?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --label --workdir",
           -exitval => 3) unless
    (scalar @labels > 0) and
    defined $workdir;

$ipprc->redirect_output($logfile) if $logfile;

# if stage is not supplied as an argument, loop over all stages
my @stages;
if ($stage) {
    push @stages, $stage;
} else {
    # raw is omitted for now
    @stages = qw( chip camera warp diff );
}

foreach my $stage (@stages) {
    foreach my $label (@labels) {
        my $command = "$magicdstool -definebyquery -stage $stage -workdir $workdir -label $label";
        $command .= " -recoveryroot $recoveryroot" if $recoveryroot;
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

exit 0;

sub my_die {
    my $msg = shift;
    my $fault = shift;

    print STDERR "$msg\n";

    exit $fault;
}

__END__
