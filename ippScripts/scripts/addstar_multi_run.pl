#!/usr/bin/env perl

use warnings;
use strict;
use Carp;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use DateTime;
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
my $addtool = can_run('addtool') or (warn "Can't find addtool" and $missing_tools = 1);
my $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $addstar = can_run('addstar') or (warn "Can't find addstar" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}
my $minidvodb_path;

my ( $label,$camera, $stage, $stage_id, $use_diff_inv, $outroot, $stageroot, $dbname, $reduction, $dvodb, $minidvodb, $minidvodb_name, $minidvodb_group, $image_only, $verbose, $no_update,
     $no_op, $redirect, $save_temps, $limit_number);
GetOptions(
    'label=s'             => \$label, # addstar label
    'stage=s'             => \$stage, # stage (camera, fullforce, etc)
    'stage_id=s'          => \$stage_id, 
    'camera=s'            => \$camera, # Camera
    'outroot=s'           => \$outroot, # output file base name
    'dvodb=s'             => \$dvodb,  # output DVO database
    'stageroot=s'         => \$stageroot, # stage root name.
    'limit=i'             => \$limit_number, 
    'use_diff_inv'        => \$use_diff_inv, # use inv images for diff cmf? 
    'dbname=s'            => \$dbname, # Database name
    'reduction=s'         => \$reduction, # Reduction class
    'minidvodb_name=s'    => \$minidvodb_name,  # miniDVO database name
    'minidvodb_group=s'   => \$minidvodb_group, # miniDVO database group
    'minidvodb'           => \$minidvodb,  # use minidvodb?
    'image-only'          => \$image_only,   # Print to stdout
    'verbose'             => \$verbose,   # Print to stdout
    'no-update'           => \$no_update, # Update the database?
    'no-op'               => \$no_op, # Don't do any operations?
    'redirect-output'     => \$redirect,
    'save-temps'          => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --label --stage --stage_id --camera --outroot --dvodb --stageroot",
          -exitval => 3,
          ) unless
    defined $stage and
    defined $stage_id and
    defined $label and
    defined $outroot and
    defined $stageroot and
    defined $dvodb and
    defined $camera;

if ($minidvodb && !defined($minidvodb_group)) {
                my_die( "missing minidvodb_group", $stage_id, $stage, $label, 3 );
            }
my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $stage_id, $stage, $label, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot) or &my_die("Missing entry from camera config", $stage_id, $stage, $label, $PS_EXIT_CONFIG_ERROR);

#my $logDest = "stuff.txt";
if ($redirect) {
    $ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $stage_id,$stage, $label, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;

if ($stage =~/fullforce/) {
    $reduction='DEFAULT';
    #hardwired because why not
}
if ($stage =~/diff/) {
    $reduction='DEFAULT';
    #hardwired because why not
}

my $recipe_addstar = $ipprc->reduction($reduction, 'ADDSTAR');

&my_die("Unrecognised ADDSTAR recipe", $stage_id, $stage, $label, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_addstar;

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

#my $temp_file = $outroot.".log.".$label.".".$stage.".".$stage_id.".list";
my $temp_file = "/tmp/addstar.list";
# XXX this temp_file is dangerous: multiple instances of this program will overwrite

print "using $temp_file for list\n";

open( TEMPLIST, ">$temp_file") or &my_die( "Can't open $temp_file\n", $stage_id,$stage,$label, $PS_EXIT_UNKNOWN_ERROR);

my @stage_extra1 = ();

# query database for list of exposures: we need to generate the input list for addstar
{
    my $mdcParser = PS::IPP::Metadata::Config->new;

    my $command = "$addtool -pendingexp";
    $command .= " -stage_id $stage_id";
    $command .= " -stage $stage";
    $command .= " -label $label";
    $command .= " -dbname $dbname"      if defined $dbname;
    $command .= " -limit $limit_number" if defined $limit_number;
    
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    &my_die( "Unable to get info on addRun", $stage_id,$stage, $label, $PS_EXIT_SYS_ERROR) unless $success;

    if (scalar @$stdout_buf == 0 ) { 
	# if there are no entries, just exit (do not update database with a fault)
	# &my_die( "empty addRun", $stage_id,$stage,$label, $PS_EXIT_SYS_ERROR);
	print "empty addRun, exiting... $stage,  $stage_id, $label\n";
	exit 0;
    } 

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata config", $stage_id,$stage,$label, $PS_EXIT_PROG_ERROR);
    # this fails if there is nothing listed. I checked.
    my $components = parse_md_list($metadata) or
	&my_die("Unable to parse metadata list", $stage_id,$stage,$label, $PS_EXIT_PROG_ERROR);
    my $num_components = scalar @$components;
    print "there are $num_components\n";

    # go through the list of components to create addstar input list
    foreach my $comp  (@$components) {
	my $mparsed = $comp->{stageroot};
	
	if (!defined($mparsed)) {
	    &my_die("Unable to parse stageroot", $stage_id,$stage,$label, $PS_EXIT_PROG_ERROR);
	} #but just to make sure, have it grab a minidvodb_name, to make sure it's not junk.
	print "found a value for stageroot:$mparsed\n";

	my $fpaObjects = $mparsed . '.cmf';
	my $realFile = $ipprc->file_resolve($fpaObjects);
	if (!defined($realFile)) {
	    &my_die("unable to resolve real file from $mparsed",$stage_id, $stage, $label, 7);
	}

	printf TEMPLIST "$realFile = $fpaObjects\n";
	if ($stage =~ /diff/) { # hardwired for now: $use_diff_inv) {
	    print "finding the inv.cmf files\n";
	    my $fpaObjectsInv = $mparsed . '.inv.cmf';
	    my $realFileInv = $ipprc->file_resolve($fpaObjectsInv);
	    if (!defined($realFileInv)) {
		&my_die("unable to resolve real file from $mparsed", $stage_id, $stage, $label, 8);
	    }
	    printf TEMPLIST "$realFileInv = $fpaObjectsInv\n";
	}

	# save the stage_extra1 values so we can update the database correctly at the end
	my $stage_extra1_value = $comp->{stage_extra1};
	if (!defined($stage_extra1_value)) {
	    &my_die("unable to find stage_extra1 value for $mparsed", $stage_id, $stage, $label, 9);
	}
	push @stage_extra1, $stage_extra1_value;
    }
}

close(TEMPLIST);
print "saved $temp_file here\n";

# convert supplied DVO database name to UNIX filename
my $dvodbReal;
if (defined $dvodb) {
    $dvodbReal = $ipprc->dvo_catdir( $dvodb ); # catdir for DVO
    $dvodbReal = $ipprc->convert_filename_absolute( $dvodbReal ) or &my_die("can't get path for dvodb", $stage_id,$stage,$label, $PS_EXIT_CONFIG_ERROR);
}

my $dtime_addstar = 0;
if (defined $dvodbReal) {
    if ($minidvodb) {
	my $command = "addtool -listminidvodbrun ";
	$command .= " -minidvodb_group $minidvodb_group" if defined $minidvodb_group;
	$command .= " -minidvodb_name $minidvodb_name" if defined $minidvodb_name;
	$command .= " -state 'active' -limit 1";
	$command .= " -dbname $dbname" if defined $dbname;
	print $command;
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	&my_die( "Unable to get active minidvodb_name", $stage_id,$stage,$label, $PS_EXIT_SYS_ERROR) unless $success;
	my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
	    &my_die("Unable to parse metadata config", $stage_id,$stage,$label, $PS_EXIT_PROG_ERROR);

	my $components = parse_md_list($metadata) or
	    &my_die("Unable to parse metadata list", $stage_id,$stage,$label, $PS_EXIT_PROG_ERROR);
	my $comp = $$components[0];
	$minidvodb_path = $comp->{minidvodb_path};
	$minidvodb_name = $comp->{minidvodb_name};
	
	if (!defined($minidvodb_path)) {
	    &my_die("Unable to parse minidvodb_path", $stage_id,$stage,$label, $PS_EXIT_PROG_ERROR);
	}
	if (!defined($minidvodb_name)) {
	    &my_die("Unable to parse minidvodb_name", $stage_id,$stage,$label, $PS_EXIT_PROG_ERROR);
	}
    } else {
	$minidvodb_path = $dvodbReal;
    }
    
    unless ($no_op) {
	print $dvodbReal;
	
	## addstar can either save the full set of detections, or just
	## the image metadata, in the dvodb.  this is set in the
	## database table addRun

	# addstar requires the user to have a valid .ptolemyrc which
	# in turn points at ippconfig/dvo.site

	# get the names for the camera and the real input file
	my $camdir = $ipprc->dvo_cameradir(); # Camera directory for addstar
	
	# most cameras use CHIP_HEADER.  other exceptions:
	# gpc1 @ camera : PHU_HEADER
	# gpc1 @ staticsky : NOMINAL (is uncalibrated)

	# temporary hard-wired info
	my $zeroPointOption = "CHIP_HEADER";
	if (($camdir =~ /gpc1/) && ($stage =~ /staticsky/)) {
	    $zeroPointOption = "NOMINAL";
	}		
	if (($camdir =~ /gpc1/) && ($stage =~ /cam/)) {
	    $zeroPointOption = "PHU_HEADER";
	}		

	# require a defined output dvo database to run addstar (ie, refuse to use the .ptolemyrc default)
	my $command  = "$addstar -update"; # XXX optionally set -update?
	$command .= " -D CAMERA $camdir";
	$command .= " -D CATDIR $minidvodb_path";
	$command .= " -D ZERO_POINT_OPTION $zeroPointOption";
	$command .= " -list $temp_file";
	$command .= " -image" if $image_only;
	if ($stage =~ /staticsky/) {
	    $command .= " -accept-astrom -quick-airmass";
	}  #careful here - this matches staticsky and staticsky_multi
	if ($stage =~ /skycal/) {
	    $command .= " -quick-airmass";
	}  #careful here - this matches staticsky and staticsky_multi
	if ($stage =~ /diff/) {
	    $command .= " -accept-astrom";
	    if ($use_diff_inv) {
		$command .= " -diff-inv";
	    }
	}
	
	if ($stage =~ /fullforce/) {
	    $command .= " -accept-astrom -xrad"; 
	}

	my $mjd_addstar_start = DateTime->now->mjd;   # MJD of starting script

	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform addstar: $error_code", $stage_id,$stage,$label, $error_code);
	}
	$dtime_addstar = 86400.0*(DateTime->now->mjd - $mjd_addstar_start);   # MJD of starting script
    }
}

my $stage_extra1_list = "";
my $Nstage_extra1_list = @stage_extra1;

for (my $i = 0; $i < $Nstage_extra1_list; $i ++) {
    $stage_extra1_list .= "$stage_extra1[$i]";
    if ($i < $Nstage_extra1_list - 1) {
	$stage_extra1_list .= ",";
    }
    print "STAGE EXTRA1 : $stage_extra1[$i]\n";
}

# at the end, we need to update the database with the entries we actually processed
# these are identified by stage_extra1, listed in the array @stage_extra1
my $fpaCommand = "$addtool -addprocessedexp";
$fpaCommand .= " -multiadd";
$fpaCommand .= " -stage $stage";
$fpaCommand .= " -stage_id $stage_id";
$fpaCommand .= " -multiaddlabel $label";
$fpaCommand .= " -dtime_addstar $dtime_addstar";
if ($Nstage_extra1_list > 0) {
    $fpaCommand .= " -stage_extra1_list $stage_extra1_list";
}
$fpaCommand .= " -path_base $outroot";
$fpaCommand .= " -dvodb_path $minidvodb_path" if defined $minidvodb_path;
$fpaCommand .= " -minidvodb_name $minidvodb_name" if defined $minidvodb_name;
$fpaCommand .= " -dbname $dbname" if defined $dbname;

print $fpaCommand;

# Add the result into the database
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


sub my_die
{
    my $msg = shift; # Warning message on die
    my $stage_id = shift; # Camtool identifier
    my $stage = shift;
    my $label = shift;
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $stage_id and not $no_update) {
        my $command = "$addtool -multiadd -stage_id $stage_id";
        $command .= " -addprocessedexp";
        $command .= " -fault $exit_code";
        $command .= " -stage $stage";
        $command .= " -multiaddlabel $label";
        $command .= " -dvodb_path $minidvodb_path" if defined $minidvodb_path;
        $command .= " -path_base $outroot" if defined $outroot;
        $command .= (" -dtime_addstar " . ((DateTime->now->mjd - $mjd_start) * 86400));
	# $command .= " -minidvodb_name $minidvodb_name" if defined $minidvodb_name; don't think we want it recorded (not sure)
        $command .= " -dbname $dbname" if defined $dbname;
	print $command;
        system ($command);
    }
    exit $exit_code;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
