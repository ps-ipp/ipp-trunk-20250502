#!/usr/bin/env perl

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
use File::Basename qw( basename dirname );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );
use Nebulous::Client;
use DBI;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $magictool   = can_run('magictool') or (warn "Can't find magictool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($magic_id, $exp_id, $outroot, $camera);
my ($dbname, $verbose, $no_update, $no_op, $logfile);

GetOptions(
           'magic_id=s'     => \$magic_id,   # Magic run identifier
           'outroot=s'      => \$outroot,    # directory to clean up
           'camera=s'       => \$camera,    # directory to clean up
           'logfile=s'      => \$logfile,

           'exp_id=s'       => \$exp_id,     # exposure id # XXX: do we need this?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'no-op'          => \$no_op,      # Don't do any operations?
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --magic_id --exp_id --outroot --camera",
           -exitval => 3) unless
    defined $magic_id and
    defined $exp_id and
    defined $outroot and 
    defined $camera;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $magic_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$ipprc->outroot_prepare($outroot);
$ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $magic_id, $PS_EXIT_SYS_ERROR ) if $logfile;

my $parentdir = dirname($outroot);
if (-e $parentdir ) {
    my @extensions = ('.list', '.streakMap', '.streaks', '.clusters', '_hough.fits');
    foreach my $ext (@extensions) {
        my $n = 0;
        my $path = $outroot . '*' . $ext;
        my @files = glob($path);
        foreach my $file (@files) {
            if (-e $file) {
                $n++;
                unless ($no_op) {
                    # remove this print
                    # print "unlinking $file\n";
                    unlink $file or my_die ("unable to delete $file", $magic_id, $PS_EXIT_SYS_ERROR);
                } else {
                    print "Skipping unlink $file\n";
                }
            }
        }
        print "Deleted $n $ext files\n" unless $no_op;
    }
    # remove the verify directory
    my $verify_dir = $parentdir . "/$exp_id.mgc.$magic_id.verify";
    if (-e $verify_dir ) {
        my $rc = system "rm -r $verify_dir";
        my_die("failed to rm $verify_dir",  $magic_id, $rc >> 8) if $rc;
        print "deleted $verify_dir\n";
    }
} else {
    # parent directory doesn't "exist" either, check that mountpoint exists
    # if it doesn't assume nfs error and fault
    # we expect at least /data/volume
    my @dirs = split "/", $parentdir;
    # so the following should be true
    # $dirs[0] eq ""
    # $dirs[1] eq 'data'
    # $dirs[2] eq the volume: for example ipp053.0
    if ((scalar(@dirs) < 3) or ($dirs[0] ne "") or ($dirs[1] ne 'data')) {
        my_die("unexpected parent of outroot found: '$parentdir'", $magic_id, $PS_EXIT_UNKNOWN_ERROR);
    } else { 
        # re-form mountpoint
        my $mountpoint = "/$dirs[1]/$dirs[2]";
        # check if it is readable
        if (! -e $mountpoint or ! -r $mountpoint ) {
            my_die("mount point for outroot not found or not readable: '$mountpoint'", $magic_id, $PS_EXIT_SYS_ERROR);
        }
    }
    print "Workdir $outroot not found. Setting magicRun.workdir_state to cleaned.\n";
    # fall through to update workdir_state
}
            
my $command = "$magictool -setworkdirstate -magic_id $magic_id -set_workdir_state cleaned";
$command   .= " -dbname $dbname" if defined $dbname;
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        carp("failed to update database for $magic_id");
    }
} else {
    print "Skipping command: $command\n";
}
exit 0;
    
sub my_die
{
    my $msg = shift;            # Warning message on die
    my $magic_id = shift;    # Magic DS identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless $exit_code;

    my $command = "$magictool -setworkdirstate -set_workdir_state error_cleaned";
    $command   .= " -magic_id $magic_id";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            carp("failed to update database for $magic_id");
        }
    } else {
        print "Skipping command: $command\n";
    }

    carp($msg);
    exit $exit_code;
}

__END__
