#!/usr/bin/env perl

use warnings;
use strict;
use Carp;
 
## report the program and machine
use Sys::Hostname;
my $host = hostname();
print "\n\n";
print "Starting script $0 on $host\n\n";

use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script
my $dtime_resort;
my $dtime_relphot;
my $dtime_script;
my $dtime_delstar;

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
my $dvomerge = can_run('dvomerge') or (warn "Can't find dvomerge" and $missing_tools = 1);
my $addtool = can_run('addtool') or (warn "Can't find addtool" and $missing_tools = 1);
my $delstar = can_run('delstar') or (warn "Can't find delstar" and $missing_tools = 1);
my $addstar = can_run('addstar') or (warn "Can't find addstar" and $missing_tools = 1);
my $relphot = can_run('relphot') or (warn "Can't find relphot" and $missing_tools = 1);
my $relastro = can_run('relastro') or (warn "Can't find relastro" and $missing_tools = 1);

my $dvoverify = can_run('dvoverify') or (warn "Can't find dvoverify" and $missing_tools = 1);


if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ( $minidvodb, $minidvodb_id, $minidvodb_group, $minidvodb_host, $camera, $dbname,$verbose, $logfile, $no_op, $redirect, $save_temps);
GetOptions(
    'minidvodb|w=s'        => \$minidvodb, #minidvodb database
    'minidvodb_id|w=s'     => \$minidvodb_id, #minidvodb_id
    'minidvodb_group|w=s'  => \$minidvodb_group, #minidvodb_group
    'minidvodb_host|w=s'   => \$minidvodb_host, #minidvodb_host
    'camera|c=s'           => \$camera, # Camera
    'dbname|d=s'           => \$dbname, # Database name
    'verbose'              => \$verbose,   # Print to stdout
    'no-op'                => \$no_op, # Don't do any operations?
    'logfile=s'            => \$logfile,
    'save-temps'           => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --minidvodb --minidvodb_id --minidvodb_group --minidvodb_host --camera ",
          -exitval => 3,
          ) unless
    defined $minidvodb and
    defined $minidvodb_id and
    defined $minidvodb_group and
    defined $minidvodb_host and
    defined $camera;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $minidvodb_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

if ($logfile) {
    $ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $minidvodb_id, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}

my $dtime_addstar = 0;

unless ($no_op) {
    
	#this is chopped into several parts: delstar,addstar, relphot, relastro, dvoverify
        #delstar - first step: are there duplicates, if so remove them
#	{
#            my $command  = "$delstar -update -dup-images  -skip-diff-pairs";
#            $command .= " -D CATDIR $minidvodb";
#                        my $mjd_delstar_start = DateTime->now->mjd;   # MJD of starting script
#	    print "\n$command\n";
#            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
#                run(command => $command, verbose => $verbose);
#            unless ($success) {
#                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
#                &my_die("Unable to perform delstar: $error_code", $minidvodb_id, $error_code);
#            }
#	    print $full_buf;
#            $dtime_delstar = 86400.0*(DateTime->now->mjd - $mjd_delstar_start);  
	    # MJD of starting script
#	    print "delstar time $dtime_delstar\n";
#        }



	#addstar
	{
            my $command  = "$addstar -resort";
            $command .= " -D CAMERA $camera";
            $command .= " -D CATDIR $minidvodb";
            my $mjd_addstar_start = DateTime->now->mjd;   # MJD of starting script
	    print "\n$command\n";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform addstar: $error_code", $minidvodb_id, $error_code);
            }
            $dtime_addstar = 86400.0*(DateTime->now->mjd - $mjd_addstar_start);  
	    # MJD of starting script
	    $dtime_resort = $dtime_addstar;
            print "addstar -resort time $dtime_addstar\n";
        }

	#relphot

        {
            # relphot only takes lower case gpc1
            my $relphot_camera = lc($camera);
            #my $command  = "$relphot -averages -update";
            #$command .= " -D CAMERA $relphot_camera";
            #$command .= " -D CATDIR $minidvodb";
	    #this is a friday hack
	    my $command = "echo skipping relphot";
	    print "$command\n";
            my $mjd_relphot_start = DateTime->now->mjd;   # MJD of starting script
	    
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform relphot: $error_code", $minidvodb_id, $error_code);
            }
            $dtime_relphot = 86400.0*(DateTime->now->mjd - $mjd_relphot_start);   # MJD of starting script
            print "relphot time $dtime_relphot\n";
        }

	{
	    
	    $dtime_script = 86400.0*(DateTime->now->mjd - $mjd_start);
            my $command = "addtool -minidvodb_id $minidvodb_id";
            $command .= " -addminidvodbprocessed";
            $command .= " -minidvodb_group $minidvodb_group";
            $command .= " -dtime_relphot $dtime_relphot"  if defined $dtime_relphot;
            $command .= " -dtime_resort $dtime_resort" if defined $dtime_resort;
	    $command .= " -dtime_script $dtime_script" if defined $dtime_script;
	    $command .= " -dbname $dbname" if defined $dbname;
            #print $command;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to add to minidvodbprocessed: $error_code", $minidvodb_id, $error_code);
            }
        }
#} else {
#    &my_die("dvodb: minidvodb_id = $minidvodb_id not found", $minidvodb_id, $PS_EXIT_UNKNOWN_ERROR);
#}
} else {
    print "skipping processing for minidvodb_id = $minidvodb_id\n";
}

exit 0;


sub my_die
{
    my $msg = shift; # Warning message on die
    my $minidbvodb_id = shift;
    my $exit_code = shift; # Exit code to add
    print STDERR "$msg $minidvodb_id\n";

if (defined $minidvodb_id ) {

    my $command = "addtool -minidvodb_id $minidvodb_id";
    $command .= " -addminidvodbprocessed";
    $command .= " -fault $exit_code";
    $command .= " -minidvodb_group $minidvodb_group";
    $command .= " -dtime_relphot $dtime_relphot" if defined $dtime_relphot;
    $command .= " -dtime_resort $dtime_resort" if defined $dtime_resort;
    $command .= " -dtime_script $dtime_script" if defined $dtime_script;
    $command .= " -dbname $dbname" if defined $dbname;
    system ($command);
    }
    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;
    exit $exit_code;
}

__END__
