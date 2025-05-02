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
my $mergetool = can_run('mergetool') or (warn "Can't find mergetool" and $missing_tools = 1);
my $dvoverify = can_run('dvoverify') or (warn "Can't find dvoverify" and $missing_tools = 1);


if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ( $mergedvodb_path, $merge_id, $mergedvodb, $minidvodb_path, $dbname,$camera,$verbose, $logfile, $no_op, $redirect, $save_temps);
GetOptions(
    'mergedvodb_path|w=s'         => \$mergedvodb_path,  # output DVO database
    'minidvodb_path|w=s'     => \$minidvodb_path, #minidvodb database
    'mergedvodb|w=s'    => \$mergedvodb, #mergedvodb
    'merge_id|w=s'  => \$merge_id, #minidvodb_id
    'dbname|d=s'        => \$dbname, # Database name
    'camera|w=s'        => \$camera, #camera name
    'verbose'           => \$verbose,   # Print to stdout
    'no-op'             => \$no_op, # Don't do any operations?
    'logfile=s'         => \$logfile,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --merge_id -- minidvodb_path --mergedvodb_path --mergedvodb --camera",
          -exitval => 3,
          ) unless
    defined $mergedvodb_path and
    defined $minidvodb_path and
    defined $mergedvodb and
    defined $merge_id and
    defined $camera;


my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $merge_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

if ($logfile) {
    $ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $merge_id, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}

# convert supplied DVO database name to UNIX filename

unless ($no_op) {

    #this is chopped into several parts: firstcheck, addstar, relphot, dvoverify, merge
    
    #first check that there were no faulted merges before. If there are, fault this merge with PS_EXIT_UNKNOWN_ERROR
    {
	my $nothing_faulted = 0;
	my $mdcParser = PS::IPP::Metadata::Config->new;
	my $command = "$mergetool -listmerged -faulted -mergedvodb " . $mergedvodb;
	$command .= " -dbname $dbname" if defined $dbname;
	print "looking for faulted:\n\n$command\n\n";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	&my_die( "Unable to get list of faulted mergedvodbs", $merge_id, $PS_EXIT_SYS_ERROR) unless $success;
	if (scalar @$stdout_buf == 0 ) { #it lists nothing if nothign has faulted
	    $nothing_faulted =1;
	    print "previous merges are okay.\n";
	} 
	&my_die( "Previous merges faulted, do not proceed until they are investigated:", $merge_id, $PS_EXIT_UNKNOWN_ERROR) unless $nothing_faulted;
    }
    {
	my $command  = "$dvoverify -s $minidvodb_path";
	print "$command\n";
	my $mjd_dvoverify_start = DateTime->now->mjd;   # MJD of starting script
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform dvoverify: $error_code", $merge_id, $error_code);
	}
	$dtime_verify = 86400.0*(DateTime->now->mjd - $mjd_dvoverify_start);   # MJD of starting script
	print "dvoverify time $dtime_verify\n";
    }
    #Merge
    
    my $this_is_the_first;
    {
	
	my $mdcParser = PS::IPP::Metadata::Config->new;
	my $command = "$mergetool -listmerged -mergedvodb " . $mergedvodb;
	$command .= " -dbname $dbname" if defined $dbname;
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	&my_die( "Unable to get list of mergedvodbruns", $merge_id, $PS_EXIT_SYS_ERROR) unless $success;
	if (scalar @$stdout_buf == 0 ) { #it lists nothing if it is the first
	    $this_is_the_first =1;
	    print "listing nothing\n";
	} else {
	    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
		&my_die("Unable to parse metadata config", $merge_id, $PS_EXIT_PROG_ERROR);
	    #this fails if there is nothing listed. I checked.
	    my $components = parse_md_list($metadata) or
		&my_die("Unable to parse metadata list", $merge_id, $PS_EXIT_PROG_ERROR);
	    my $comp = $$components[0];
	    my  $mergedvodb_path_tmp = $comp->{mergedvodb_path};
	    
	    if (!defined($mergedvodb_path_tmp)) {
		&my_die("Unable to parse mergedvodb_path", $merge_id, $PS_EXIT_PROG_ERROR);
	    } #but just to make sure, have it grab a minidvodb_name, to make sure it's not junk.
	    print "found at least 1 mergedvodbrun in merged state\n";
	    $this_is_the_first = 0;
	}
    }
    print "$this_is_the_first $mergedvodb_path/Image.dat\n";
    if (-e "$mergedvodb_path/Image.dat") {
	if ($this_is_the_first == 1) {
	    &my_die("refusing to merge, this is the first, but files already exist in dir", $merge_id, 4);
	}
	$this_is_the_first =0;
    }
	
    print "$this_is_the_first $mergedvodb_path/Image.dat\n";
    {
	my $merge_command;
	my $mjd_merge_start = DateTime->now->mjd;   # MJD of starting script
	if ($this_is_the_first) {
	    $merge_command = "rsync -rvat $minidvodb_path/* $mergedvodb_path";
	} else {
#	    $merge_command = "$dvomerge -parallel $minidvodb_path into $mergedvodb_path";
	    $merge_command = "$dvomerge -matched-tables $minidvodb_path into $mergedvodb_path";
	}
	print "\n$merge_command\n";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $merge_command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to merge: $error_code", $merge_id, $error_code);
	}
	$dtime_merge = 86400.0*(DateTime->now->mjd - $mjd_merge_start);   # MJD of starting script
	print "merge time $dtime_merge\n";
#	}
    }

    {

	$dtime_script = 86400.0*(DateTime->now->mjd - $mjd_start);
	my $command = "mergetool -merge_id $merge_id";
	$command .= " -addmerged";
	$command .= " -mergedvodb $mergedvodb" if defined $mergedvodb;
#	$command .= " -mergedvodb_path $mergedvodb_path" if defined $mergedvodb_path;
	$command .= " -dtime_merge $dtime_merge" if defined $dtime_merge;
	$command .= " -dtime_script $dtime_script" if defined $dtime_script;
	$command .= " -dtime_verify $dtime_verify" if defined $dtime_verify;
	$command .= " -dbname $dbname" if defined $dbname;
	#print $command;
	
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to add to mergedvodbprocessed: $error_code", $merge_id, $error_code);
	}
    }
    
#} else {
#    &my_die("dvodb: $mergedvodb_path not found", $merge_id, $PS_EXIT_UNKNOWN_ERROR);
}
else {
    print "skipping processing for $mergedvodb_path\n";
}
    
exit 0;


sub my_die
{
    my $msg = shift; # Warning message on die
    my $merge_id = shift;
    my $exit_code = shift; # Exit code to add

    print STDERR "$msg $mergedvodb_path\n";

if (defined $merge_id ) {

    my $command = "mergetool -merge_id $merge_id";
    $command .= " -addmerged";
    $command .= " -mergedvodb $mergedvodb" if defined $mergedvodb;
        $command .= " -fault $exit_code";
        #$command .= " -mergedvodb_path $mergedvodb_path" if defined $mergedvodb_path;
        $command .= " -dtime_merge $dtime_merge" if defined $dtime_merge;
    $command .= " -dtime_script $dtime_script" if defined $dtime_script;
    $command .= " -dtime_verify $dtime_verify" if defined $dtime_verify;
        $command .= " -dbname $dbname" if defined $dbname;



    print $command;
    system ($command);
    }




    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    exit $exit_code;
}

__END__
