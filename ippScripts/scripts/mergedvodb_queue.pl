#!/usr/bin/env perl

use warnings;
use strict;
use Carp;
use DateTime;

# Ideally, we want to queue new runs with mergetool -definebyquery.  We can't for the survey task, because we use the value in 'site.config' to find the mergedvodb_path. I do not know how to tell pantasks how to do this, but I know how to do it in perl.


use DateTime::Format::Strptime;
use DateTime::Duration;
## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";


my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $mergetool = can_run('mergetool') or (warn "Can't find mergetool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my (  $outroot, $dbname, $minidvodb_group, $mergedvodb , $camera,  $verbose, $no_update,
     $no_op, $redirect, $save_temps);

# we just need any old camera to be able to read site.config. I'm using SIMPLE, unless it is overridden on the commandline (it makes no difference for this case)

$camera = 'SIMPLE';


GetOptions(
    'camera|c=s'        => \$camera, # Camera
    'dbname|d=s'        => \$dbname, # Database name
    'minidvodb_group|w=s'       => \$minidvodb_group, # minidvodb_group.
    'outroot|w=s'       => \$outroot, # output file base name
    'mergedvodb|w=s'     => \$mergedvodb, # output miniDVODB
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --mergedvodb --minidvodb_group  --outroot",
          -exitval => 3,
          ) unless
    defined $minidvodb_group and
   # defined $camera and
    defined $outroot and
    defined $mergedvodb;

print "camera is $camera\n";


my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $mergedvodb   , $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot) or &my_die("Missing entry from camera config", $minidvodb_group, $PS_EXIT_CONFIG_ERROR);
print "log file is $logDest\n";
if ($redirect) {
    $ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $minidvodb_group, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}


# Recipes to use based on reduction class

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Output products
$ipprc->outroot_prepare($outroot);

# the camera configurations should define the psastro output to be a single file (MEF), regardless of the inputs
 my $create_new = 0;
# convert supplied DVO database name to UNIX filename
my $mergedvodbReal;
if (defined $mergedvodb) {
    $mergedvodbReal = $ipprc->dvo_catdir( $mergedvodb ); # catdir for DVO
    $mergedvodbReal = $ipprc->convert_filename_absolute( $mergedvodbReal );
} else {
    warn("mergedvodb undefined:\n");
    exit(4);
}
if (!defined $mergedvodbReal) {
    warn("can't parse out mergedvodb_path\n");
    exit(4);
}



print "mergedvodb_path = $mergedvodbReal\n";

unless ($no_op) {

  #create the mergedvodb entry (well, the command for it)
    my $fpaCommand = "$mergetool -definebyquery";
    $fpaCommand .= " -mergedvodb $mergedvodb";
    $fpaCommand .= " -minidvodb_group $minidvodb_group";
    $fpaCommand .= " -set_mergedvodb_path  $mergedvodbReal" ;
    $fpaCommand .= " -dbname $dbname" if defined $dbname;

    print "running the following: \n\n";
    print "$fpaCommand\n\n";

    unless ($no_update) {


        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $fpaCommand, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn("Unable to add result to database: $error_code\n");
            exit($error_code);
        }
    } else {
        print "skipping command: $fpaCommand\n";
    }
}

sub my_die
{#complain if it doesn't work
    my $msg = shift; # Warning message on die
    my $mergedvodb = shift; # Camtool identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    exit $exit_code;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
