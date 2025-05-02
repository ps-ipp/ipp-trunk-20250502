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
my $dtime_merge;
my $dtime_verify;
my $dtime_script;

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
my $addstar = can_run('addstar') or (warn "Can't find addstar" and $missing_tools = 1);
my $relphot = can_run('relphot') or (warn "Can't find relphot" and $missing_tools = 1);
my $relastro = can_run('relastro') or (warn "Can't find relastro" and $missing_tools = 1);
my $dvoverify = can_run('dvoverify') or (warn "Can't find dvoverify" and $missing_tools = 1);


if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ( $mergedvodb, $minidvodb, $minidvodb_id, $minidvodb_group, $camera, $dbname,$verbose, $logfile, $no_op, $redirect, $save_temps);
GetOptions(
    'mergedvodb|w=s'         => \$mergedvodb,  # output DVO database
    'minidvodb|w=s'     => \$minidvodb, #minidvodb database
    'minidvodb_id|w=s'  => \$minidvodb_id, #minidvodb_id
 'minidvodb_group|w=s'  => \$minidvodb_group, #minidvodb_id
    'camera|c=s'        => \$camera, # Camera
    'dbname|d=s'        => \$dbname, # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-op'             => \$no_op, # Don't do any operations?
    'logfile=s'         => \$logfile,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --mergedvodb --minidvodb --minidvodb_id -- minidvodb_group --camera",
          -exitval => 3,
          ) unless
    defined $mergedvodb and
    defined $minidvodb and
    defined $minidvodb_id and
    defined $minidvodb_group and
    defined $camera;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $mergedvodb, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

if ($logfile) {
    $ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $mergedvodb, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}

# convert supplied DVO database name to UNIX filename
my $mergedvodbReal;
if (defined $mergedvodb) {
    $mergedvodbReal = $ipprc->dvo_catdir( $mergedvodb ); # catdir for DVO
    $mergedvodbReal = $ipprc->convert_filename_absolute( $mergedvodbReal );
}

my $dtime_addstar = 0;
#my $dtime_relphot = 0;

unless ($no_op) {
    if (defined $mergedvodbReal) {

	#this is chopped into several parts: firstcheck, addstar, relphot, dvoverify, merge
        
        #first check that there were no faulted merges before. If there are, fault this merge with PS_EXIT_UNKNOWN_ERROR
	{
	    my $nothing_faulted = 0;
	    my $mdcParser = PS::IPP::Metadata::Config->new;

            my $command = "$addtool -listminidvodbprocessed -faulted -minidvodb_group " . $minidvodb_group;
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            &my_die( "Unable to get list of faulted minidvodbs", $minidvodb_id, $PS_EXIT_SYS_ERROR) unless $success;
            if (scalar @$stdout_buf == 0 ) { #it lists nothing if nothign has faulted
                $nothing_faulted =1;
		print "previous merges are okay.\n";
            } 
	    &my_die( "Previous merges faulted, do not proceed until they are investigated:", $minidvodb_group, $PS_EXIT_UNKNOWN_ERROR) unless $nothing_faulted;


 
	}

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
                &my_die("Unable to perform addstar: $error_code", $mergedvodb, $error_code);
            }
            $dtime_addstar = 86400.0*(DateTime->now->mjd - $mjd_addstar_start);  $dtime_resort = $dtime_addstar;
            # MJD of starting script
	    $dtime_resort = $dtime_addstar;
            print "addstar -resort time $dtime_addstar\n";
        }

	#relphot

        {
            # relphot only takes lower case gpc1
            my $relphot_camera = lc($camera);
            my $command  = "$relphot -averages -update";
            $command .= " -D CAMERA $relphot_camera";
            $command .= " -D CATDIR $minidvodb";
	    print "$command\n";
            my $mjd_relphot_start = DateTime->now->mjd;   # MJD of starting script

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform relphot: $error_code", $mergedvodb, $error_code);
            }
            $dtime_relphot = 86400.0*(DateTime->now->mjd - $mjd_relphot_start);   # MJD of starting script
            print "relphot time $dtime_relphot\n";
        }
	#dvoverify
	{
            my $command  = "$dvoverify -s $minidvodb";
            print "$command\n";
            my $mjd_dvoverify_start = DateTime->now->mjd;   # MJD of starting script

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform dvoverify: $error_code", $mergedvodb, $error_code);
            }
            $dtime_verify = 86400.0*(DateTime->now->mjd - $mjd_dvoverify_start);   # MJD of starting script
            print "dvoverify time $dtime_verify\n";
        }


	#Merge

        my $this_is_the_first;
        {

            my $mdcParser = PS::IPP::Metadata::Config->new;

            my $command = "$addtool -listminidvodbrun -state merged -minidvodb_group " . $minidvodb_group;
            $command .= " -dbname $dbname" if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
            &my_die( "Unable to get list of minidvodbruns", $minidvodb_id, $PS_EXIT_SYS_ERROR) unless $success;
            if (scalar @$stdout_buf == 0 ) { #it lists nothing if it is the first
                $this_is_the_first =1;
		print "listing nothing\n";
            } else {
                my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
                    &my_die("Unable to parse metadata config", $minidvodb_id, $PS_EXIT_PROG_ERROR);
                #this fails if there is nothing listed. I checked.
                my $components = parse_md_list($metadata) or
                    &my_die("Unable to parse metadata list", $minidvodb_id, $PS_EXIT_PROG_ERROR);
                my $comp = $$components[0];
                my  $minidvodb_name = $comp->{minidvodb_name};

                if (!defined($minidvodb_name)) {
                    &my_die("Unable to parse minidvodb_name", $minidvodb_id, $PS_EXIT_PROG_ERROR);
                } #but just to make sure, have it grab a minidvodb_name, to make sure it's not junk.
		print "found at least 1 minidvodbrun in merged state\n";
                $this_is_the_first = 0;
            }
        }
	print "$this_is_the_first $mergedvodb/Image.dat\n";
	
	 
	if (-e "$mergedvodb/Image.dat") {
	    if ($this_is_the_first == 1) {
		&my_die("refusing to merge, this is the first, but files already exist in dir", $minidvodb_id, 4);
	    }
	    $this_is_the_first =0;
	}
	
	print "$this_is_the_first $mergedvodb/Image.dat\n";
        {
            my $merge_command;
	    my $mjd_merge_start = DateTime->now->mjd;   # MJD of starting script
            if ($this_is_the_first) {
		
		$merge_command = "rsync -rvat $minidvodb/* $mergedvodb";
	    } else {
                $merge_command = "$dvomerge $minidvodb into $mergedvodb";
	    }
	    
	    print "\n$merge_command\n";
	    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		run(command => $merge_command, verbose => $verbose);
	    unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		    &my_die("Unable to merge: $error_code", $mergedvodb, $error_code);
	    }
		
	    $dtime_merge = 86400.0*(DateTime->now->mjd - $mjd_merge_start);   # MJD of starting script
	    print "merge time $dtime_merge\n";
#	}
	}

	{

	    $dtime_script = 86400.0*(DateTime->now->mjd - $mjd_start);


            my $command = "addtool -minidvodb_id $minidvodb_id";
            $command .= " -addminidvodbprocessed";
            $command .= " -mergedvodb_path $mergedvodbReal" if defined $mergedvodbReal;
            $command .= " -minidvodb_group $minidvodb_group";
            $command .= " -dtime_relphot $dtime_relphot"  if defined $dtime_relphot;
            $command .= " -dtime_resort $dtime_resort" if defined $dtime_resort;
            $command .= " -dtime_merge $dtime_merge" if defined $dtime_merge;
	    $command .= " -dtime_script $dtime_script" if defined $dtime_script;
	    $command .= " -dtime_verify $dtime_verify" if defined $dtime_verify;
	    $command .= " -dbname $dbname" if defined $dbname;
            #print $command;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to add to minidvodbprocessed: $error_code", $mergedvodb, $error_code);
            }
        }

    } else {
        &my_die("dvodb: $mergedvodb not found", $mergedvodb, $PS_EXIT_UNKNOWN_ERROR);
    }
} else {
    print "skipping processing for $mergedvodbReal\n";
}

exit 0;


sub my_die
{
    my $msg = shift; # Warning message on die
    my $minidbvodb_id = shift;
    my $exit_code = shift; # Exit code to add

    print STDERR "$msg $mergedvodb\n";

if (defined $minidvodb_id ) {

    my $command = "addtool -minidvodb_id $minidvodb_id";
    $command .= " -addminidvodbprocessed";
        $command .= " -fault $exit_code";
        $command .= " -mergedvodb_path $mergedvodbReal" if defined $mergedvodbReal;
        $command .= " -minidvodb_group $minidvodb_group";
        $command .= " -dtime_relphot $dtime_relphot" if defined $dtime_relphot;
        $command .= " -dtime_resort $dtime_resort" if defined $dtime_resort;
        $command .= " -dtime_merge $dtime_merge" if defined $dtime_merge;
    $command .= " -dtime_script $dtime_script" if defined $dtime_script;
    $command .= " -dtime_verify $dtime_verify" if defined $dtime_verify;
        $command .= " -dbname $dbname" if defined $dbname;



    #print $command;
    system ($command);
    }




    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    exit $exit_code;
}

__END__
