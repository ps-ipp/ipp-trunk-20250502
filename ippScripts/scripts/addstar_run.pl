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

# these are only used by addstar_run, not addstar_multi
my $fftool  = can_run('fftool') or (warn "Can't find fftool" and $missing_tools = 1);
my $loadgalphot = can_run('loadgalphot') or (warn "Can't find loadgalphot" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}
my $minidvodb_path;

my ( $add_id, $camera, $stage, $stage_id, $stage_extra1, $outroot, $stageroot, $dbname, $reduction,
     $dvodb, $minidvodb, $minidvodb_name, $minidvodb_group, $image_only, $verbose,
     $no_update, $no_op, $redirect, $save_temps, $addrun_host);
GetOptions(
    'add_id=s'          => \$add_id, # Camtool identifier
    'camera|c=s'        => \$camera, # Camera
    'stage|s=s'        => \$stage, # Camera
    'stage_id|w=s'   => \$stage_id,
    'stage_extra1|w=s'    => \$stage_extra1, # the number for a staticskymulti (for finding cmf), or stack_id
    'dbname|d=s'        => \$dbname, # Database name
    'outroot|w=s'       => \$outroot, # output file base name
    'stageroot|w=s'       => \$stageroot, # stage root name.
    'reduction=s'       => \$reduction, # Reduction class
    'dvodb|w=s'         => \$dvodb,  # output DVO database
    'minidvodb_name|w=s'=> \$minidvodb_name,  # miniDVO database name
    'minidvodb_group|w=s' => \$minidvodb_group, # miniDVO database group
    'minidvodb'         => \$minidvodb,  # use minidvodb?
    'image-only'        => \$image_only,   # Print to stdout
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    'addrun_host|w=s' => \$addrun_host, # miniDVO database host
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --add_id --camera --outroot --dvodb --stageroot --stage --dbname --addrun_host",
          -exitval => 3,
          ) unless
    defined $stage and
    defined $add_id and
    defined $outroot and
    defined $stageroot and
    defined $dvodb and
    defined $dbname and
    defined $addrun_host and
    defined $camera;
if ($stage =~ /cam/ && !defined $stage_id) {
    my_die("cam stage requires -stage_id", $add_id, 3);

}
if ($minidvodb && !defined($minidvodb_group)) {
                my_die( "missing minidvodb_group", $add_id, 3 );
            }
my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $add_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot) or &my_die("Missing entry from camera config", $add_id, $PS_EXIT_CONFIG_ERROR);

if ($redirect) {
    $ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $add_id, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Output products
$ipprc->outroot_prepare($outroot);


#
# Step 1: sort out reductions / recipes
#


# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
#if ($stage =~/diff/) {
#    $reduction = 'ADDSTAR';
#} 
#if ($stage =~/fullforce/) {
#    $reduction = 'ADDSTAR';
#} 

if ($stage eq 'fullforce') {
    $reduction='DEFAULT';
    #hardwired because why not
}
elsif ($stage eq 'fullforce_summary') {
    $reduction = 'DEFAULT';
}
elsif ($stage eq 'diff') {
    $reduction='DEFAULT';
    #hardwired because why not
}

my $recipe_addstar = $ipprc->reduction($reduction, 'ADDSTAR');
# XXX This is now not used: do we still need it?
if ($stage eq 'cam') {
  $recipe_addstar = $ipprc->reduction($reduction, 'ADDSTAR'); # Recipe to use
}
elsif ($stage eq 'stack') {
  $recipe_addstar = $ipprc->reduction($reduction, 'ADDSTAR_STACK'); # Recipe to use
}
elsif ($stage eq 'skycal') {
    $recipe_addstar = $ipprc->reduction($reduction, 'ADDSTAR'); # Recipe to use                                                              
}
#if ($stage =~/staticsky/) {
#  $recipe_addstar = $ipprc->reduction($reduction, 'ADDSTAR_STATICSKY'); # Recipe to use
#}



&my_die("Unrecognised ADDSTAR recipe", $add_id, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_addstar;


# the camera configurations should define the psastro output to be a single file (MEF), regardless of the inputs

#
# Step 2: Determine where sources should come from, modify them as needed.
#


# it was PSASTRO.OUTPUT

my $fpaObjects = $ipprc->filename("PSASTRO.OUTPUT", $stageroot) or &my_die("Missing entry from camera config", $add_id, $PS_EXIT_CONFIG_ERROR);
my $fpaObjectsAlt = $fpaObjects;

if ($stage eq 'skycal') {
    #should be ok for skycal?
    print "using $fpaObjects for $stage\n";
}
elsif ($stage eq 'diff') {
    print "using $fpaObjects for $stage\n";
}
elsif ($stage eq 'fullforce')  {
    print "using $fpaObjects for $stage\n";
} 

if ($stage eq 'cam') {
    # if it is cam stage we need to be careful when grabbing the filename. 
    # This breaks down into a few steps: 
    
    #get info about the cam_id 
    my $magicked;
    {
	my $mdcParser = PS::IPP::Metadata::Config->new;
        my $command = "$camtool -processedexp -cam_id " . $stage_id;
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die( "Unable to get info on camRun", $add_id, $PS_EXIT_SYS_ERROR) unless $success;
        if (scalar @$stdout_buf == 0 ) { #it lists nothing if it is the first
	    &my_die( "empty camRun", $add_id, $PS_EXIT_SYS_ERROR);
	    print "listing nothing\n";
	} else {
	    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
		&my_die("Unable to parse metadata config", $add_id, $PS_EXIT_PROG_ERROR);
	    #this fails if there is nothing listed. I checked.
	    my $components = parse_md_list($metadata) or
		&my_die("Unable to parse metadata list", $add_id, $PS_EXIT_PROG_ERROR);
	    my $comp = $$components[0];
	    my  $mparsed = $comp->{magicked};
	    
	    if (!defined($mparsed)) {
		&my_die("Unable to parse magicked", $add_id, $PS_EXIT_PROG_ERROR);
	    } #but just to make sure, have it grab a minidvodb_name, to make sure it's not junk.
	    print "found a value for magicked:$mparsed\n";
	    $magicked = $mparsed;
	}
	
	#is this cam_id magicked or not?
	if ($magicked) {
	    $stage_extra1 = $magicked;
	    $fpaObjects = $ipprc->destreaked_filename("$fpaObjects") or &my_die("Missing entry from camera config", $add_id, $PS_EXIT_CONFIG_ERROR);
	    print "cam_id is magicked, using $fpaObjects for the cam smf\n";
	} else {
	    print "cam_id is NOT magicked, using $fpaObjects for the cam smf\n";
	}
    }
}

if (($stage eq 'stack') || 
    ($stage eq 'skycal') || 
    ($stage eq 'diff') || 
    ($stage eq 'fullforce')||
    ($stage eq 'fullforce_summary')) {
    $fpaObjects =~ s/smf$/cmf/;
    $fpaObjectsAlt =~ s/smf$/cmf/;
}

my $fpaObjects1;
my $fpaObjects2;
my $checkalt = 0;
if ($stage eq 'staticsky') {
    $checkalt = 1;
    my $sources   = $ipprc->filename("PSPHOT.OUT.CMF.MEF", $stageroot); #this is mostly rigtht except the .cmf needs either
    # .000.cmf or .stk.xxxxx.cmf
    print "$sources\n\n\n";
    &my_die( "can't find the filter_num for staticsky_multi, giving up.", $add_id, $PS_EXIT_SYS_ERROR) unless (defined $stage_extra1);

    
    $fpaObjects1 = $sources;
    $fpaObjects2 = $sources;
    $fpaObjects = $sources;

    my $nice_num = sprintf ("%03d", $stage_extra1);

    $fpaObjects1 =~ s/cmf$/stk.$stage_extra1.cmf/;
    $fpaObjects2 =~ s/cmf$/$nice_num.cmf/;  #this make it look for .001.cmf, etc
 # we have 3 of them to try
    my $realFile = $ipprc->file_resolve($fpaObjects);
    my $realFile1 = $ipprc->file_resolve($fpaObjects1);
    my $realFile2 = $ipprc->file_resolve($fpaObjects2);
    if (!defined($realFile1)) {
	if (!defined($realFile2)) {
	    print "using $fpaObjects\n";
	    
	} else {
	    print "using $fpaObjects2\n";
	    $fpaObjects = $fpaObjects2;
	}
	
    } else {
	print "using $fpaObjects1\n";
	$fpaObjects = $fpaObjects1;
    }
}


#
# Step 3 Set up addtool/loadgalphot commands.
#

my $traceDest  = $ipprc->filename("TRACE.EXP",          $outroot) or &my_die("Missing entry from camera config", $add_id, $PS_EXIT_CONFIG_ERROR);
	
# convert supplied DVO database name to UNIX filename
my $dvodbReal;
if (defined $dvodb) {
    $dvodbReal = $ipprc->dvo_catdir( $dvodb ); # catdir for DVO
    $dvodbReal = $ipprc->convert_filename_absolute( $dvodbReal ) or &my_die("can't get path for dvodb", $add_id, $PS_EXIT_CONFIG_ERROR);
}

my $dtime_addstar = 0;
if (defined $dvodbReal) {
    if ($minidvodb) {
	my $command = "$addtool -listminidvodbrun ";
	$command .= " -minidvodb_group $minidvodb_group" if defined $minidvodb_group;
	$command .= " -minidvodb_name $minidvodb_name" if defined $minidvodb_name;
	$command .= " -state 'active' -limit 1";
	$command .= " -dbname $dbname" if defined $dbname;
	print $command;
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	&my_die( "Unable to get active minidvodb_name", $add_id, $PS_EXIT_SYS_ERROR) unless $success;
	my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
	    &my_die("Unable to parse metadata config", $add_id, $PS_EXIT_PROG_ERROR);

	my $components = parse_md_list($metadata) or
	    &my_die("Unable to parse metadata list", $add_id, $PS_EXIT_PROG_ERROR);
	my $comp = $$components[0];
	$minidvodb_path = $comp->{minidvodb_path};
	$minidvodb_name = $comp->{minidvodb_name};
        $addrun_host = $comp->{addrun_host};

	if (!defined($minidvodb_path)) {
	    &my_die("Unable to parse minidvodb_path", $add_id, $PS_EXIT_PROG_ERROR);
	}
	if (!defined($minidvodb_name)) {
	    &my_die("Unable to parse minidvodb_name", $add_id, $PS_EXIT_PROG_ERROR);
	}
    }
    else {
	$minidvodb_path = $dvodbReal;
    }
    
    unless ($no_op) {
            print "$dvodbReal\n";
	    ## addstar can either save the full set of detections, or just
	    ## the image metadata, in the dvodb.  this is set in the
	    ## database table addRun

	    # addstar requires the user to have a valid .ptolemyrc which
	    # in turn points at ippconfig/dvo.site

	    # get the names for the camera and the real input file
	    my $camdir = $ipprc->dvo_cameradir(); # Camera directory for addstar
	   
	    my $realFile = $ipprc->file_resolve($fpaObjects) or  &my_die("Unable to resolve $fpaObjects", $add_id, $PS_EXIT_SYS_ERROR);

	    # most cameras use CHIP_HEADER.  other exceptions:
	    # gpc1 @ camera : PHU_HEADER
	    # gpc1 @ staticsky : NOMINAL (is uncalibrated)

	    # temporary hard-wired info
	    my $zeroPointOption = "CHIP_HEADER";
	    my $zeroPointKeyword = "ZPT_OBS";
	    if (($camdir =~ /gpc1/) && ($stage =~ /staticsky/)) {
		$zeroPointOption = "NOMINAL";
	    }		
	    if (($camdir =~ /gpc1/) && ($stage =~ /cam/)) {
		$zeroPointOption = "PHU_HEADER";
	    }		
	    if ($stage eq "diff") {
		$zeroPointKeyword = "FPA.ZP";
	    }		

	    my $command;

	    if ($stage ne 'fullforce_summary') {
#		print "IN ADDSTAR VERSION\n";
		# require a defined output dvo database to run addstar (ie, refuse to use the .ptolemyrc default)
		$command  = "$addstar -update"; # XXX optionally set -update?
		$command .= " -force-single-time"; # IPP-676 time difference for chips causes dvo mosaic issue
		$command .= " -D CAMERA $camdir";
		$command .= " -D CATDIR $minidvodb_path";
		$command .= " -D ZERO_POINT_OPTION $zeroPointOption";
		$command .= " -D ZERO_POINT_KEYWORD $zeroPointKeyword";
		$command .= " $realFile";
		$command .= " -use-name $fpaObjects"; # DVO wants the neb-name as a file reference
		$command .= " -image" if $image_only;
		if ($stage =~ /staticsky/) {
		    $command .= " -accept-astrom -quick-airmass";
		}  #careful here - this matches staticsky and staticsky_multi
		if ($stage =~ /skycal/) {
		    $command .= " -quick-airmass";
		}  #careful here - this matches staticsky and staticsky_multi
		if ($stage =~ /diff/) {
		    $command .= " -accept-astrom";
		}
		if ($stage =~ /fullforce/) {
		    $command .= " -accept-astrom -xrad"; 
		}
	    }
	    else { # Full force summary case
#		print "IN LOADGALPHOT VERSION\n";
		# We need to know the filter to set up the photcode, as the summaries do not include that in the header.
		# We /could/ set up the addtool stuff to pass that in to the script when needed, but that would require adding 
		# a lot of extra hooks and option handlers.  Therefore, call out to fftool, and get the filter that way, and
		# construct the correct photcode.

		my $photcode = '';
		{
		    my $ff_command = "$fftool -dbname ${dbname} -summary -ff_id ${stage_id}";
		    print "$ff_command\n";
		    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
			run(command => $ff_command, verbose => $verbose);
		    unless ($success) {
			$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
			&my_die("Unable to fetch filter via fftool: $error_code", $add_id, $error_code);
		    }
		    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
			&my_die("Unable to determine ff summary information.", $add_id, $error_code);
		    my $metadata = parse_md_list($MDlist);
		    my $ffSummary = $metadata->[0];
		    my $filter = $ffSummary->{filter};
		    my $stack_id = $ffSummary->{stack_id};
		    
		    $filter =~ s/\.00000//;
		    $photcode = "GPC1.${filter}.ForcedWarp";
		}

		$command =  "$loadgalphot -v ";
		$command .= " -D CAMERA $camdir ";
		$command .= " -D CATDIR $minidvodb_path ";
		$command .= " $realFile ";
		$command .= " -p $photcode ";
		$command .= " -image-id $stage_id ";
	    }
		    
	    my $mjd_addstar_start = DateTime->now->mjd;   # MJD of starting script
	    
	    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		run(command => $command, verbose => $verbose);
	    unless ($success) {
		$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		&my_die("Unable to perform addstar: $error_code", $add_id, $error_code);
	    }
	    $dtime_addstar = 86400.0*(DateTime->now->mjd - $mjd_addstar_start);   # MJD of starting script
    }
}

# This needs to be updated when addtool is written. BROKEN
my $fpaCommand = "$addtool -addprocessedexp";
$fpaCommand .= " -add_id $add_id";
$fpaCommand .= " -dtime_addstar $dtime_addstar";

$fpaCommand .= " -path_base $outroot";
$fpaCommand .= " -dvodb_path $minidvodb_path" if defined $minidvodb_path;
$fpaCommand .= " -minidvodb_name $minidvodb_name" if defined $minidvodb_name;
$fpaCommand .= " -addrun_host $addrun_host" if defined $addrun_host;
$fpaCommand .= " -dbname $dbname" if defined $dbname;

$fpaCommand .= " -stage_extra1 $stage_extra1" if defined $stage_extra1;

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
    my $add_id = shift; # Camtool identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $add_id and not $no_update) {
        my $command = "$addtool -add_id $add_id";
        $command .= " -addprocessedexp";
        $command .= " -fault $exit_code";
        $command .= " -dvodb_path $minidvodb_path" if defined $minidvodb_path;
        $command .= " -path_base $outroot" if defined $outroot;
        $command .= (" -dtime_addstar " . ((DateTime->now->mjd - $mjd_start) * 86400));
        $command .= " -minidvodb_name $minidvodb_name" if defined $minidvodb_name; # dont think we want it recorded (not sure)
        $command .= " -addrun_host $addrun_host" if defined $addrun_host; 
	$command .= " -stage_extra1 $stage_extra1" if defined $stage_extra1;
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
