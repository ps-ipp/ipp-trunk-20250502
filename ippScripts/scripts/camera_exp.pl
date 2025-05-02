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
my $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $ppImage = can_run('ppImage') or (warn "Can't find ppImage" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $psastro = can_run('psastro') or (warn "Can't find psastro" and $missing_tools = 1);
my $addstar = can_run('addstar') or (warn "Can't find addstar" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

my ( $exp_tag, $cam_id, $camera, $outroot, $dbname, $reduction, $dvodb, $verbose, $no_update,
     $no_op, $redirect, $save_temps, $run_state, $skip_binned, $skip_masks, $bkg_only);
GetOptions(
    'exp_tag=s'         => \$exp_tag, # Exposure identifier
    'cam_id=s'          => \$cam_id, # Camtool identifier
    'camera|c=s'        => \$camera, # Camera
    'dbname|d=s'        => \$dbname, # Database name
    'outroot|w=s'       => \$outroot, # output file base name
    'reduction=s'       => \$reduction, # Reduction class
    'dvodb|w=s'         => \$dvodb,  # output DVO database
    'run-state=s'       => \$run_state, # 'new' or 'update'
    'skip-binned'       => \$skip_binned, # override recipe - don't create binned images
    'skip-refmask'      => \$skip_masks, # override recipe - don't create refmask
#    'bkg-only'          => \$bkg_only,  # override recipe - only do background continuity
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --exp_tag --cam_id --camera --outroot",
          -exitval => 3,
          ) unless
    defined $exp_tag and
    defined $cam_id and
    defined $outroot and
    defined $camera;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $cam_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

if (not defined $run_state) { $run_state = 'new'; }

my_die ("$run_state is an invalid value for run-state", $cam_id, $PS_EXIT_PROG_ERROR) unless ($run_state eq 'new' or $run_state eq 'update');


my $replicateOutputs = 1;

$ipprc->outroot_prepare($outroot);

my $logDest;
my $traceDest;
if ($run_state eq 'new') {
    $logDest = prepare_output("LOG.EXP", $outroot, undef, 0);
    $traceDest = prepare_output("TRACE.EXP", $outroot, undef, 0);
} else {
    $logDest = prepare_output("LOG.EXP.UPDATE", $outroot, undef, 0);
    $traceDest = prepare_output("TRACE.EXP.UPDATE", $outroot, undef, 0);
}

if ($redirect) {
    $ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $cam_id, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "FULL COMMAND: $0 @ARGS\n\n";
}

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;

my $recipe1 = $ipprc->reduction($reduction, 'JPEG_BIN1'); # Recipe to use
&my_die("Unrecognised JPEG recipe", $cam_id, $PS_EXIT_CONFIG_ERROR) unless defined $recipe1;

my $recipe2 = $ipprc->reduction($reduction, 'JPEG_BIN2'); # Recipe to use
&my_die("Unrecognised JPEG recipe", $cam_id, $PS_EXIT_CONFIG_ERROR) unless defined $recipe2;

#my $recipe_addstar = $ipprc->reduction($reduction, 'ADDSTAR'); # Recipe to use
#&my_die("Unrecognised ADDSTAR recipe", $cam_id, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_addstar;

my $recipe_psastro = $ipprc->reduction($reduction, 'PSASTRO'); # Recipe to use
&my_die("Unrecognised PSASTRO recipe", $cam_id, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_psastro;

my $bkg_recipe     = 'PPIMAGE_BKGCONT'; # Add to reduction?
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $recipeData;
{
    # Get the PSASTRO recipe
    my $command = "$ppConfigDump -camera $camera -recipe PSASTRO $recipe_psastro -dump-recipe PSASTRO -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $cam_id, $PS_EXIT_CONFIG_ERROR);
    }
    $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $cam_id, $PS_EXIT_CONFIG_ERROR);
}

## option to skip the astrometry analysis (but still do the jpegs)
## if not defined in the config file, default is to not skip
my $skip_astrom = 0;
$skip_astrom = metadataLookupBool($recipeData, 'PSASTRO.SKIP.ASTROMETRY');
print "skip astrom: $skip_astrom\n"; 
if ($skip_astrom) { print "skip the astrom\n"; }

my $cmdflags;

# Get list of component files
# get FWHM statistics from input chip cmf files using ppStatsFromMetadata
my $files;                      # Array of component files
{
    my $command = "$camtool -pendingimfile -cam_id $cam_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform camtool: $error_code", $cam_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $cam_id, $PS_EXIT_PROG_ERROR);

    # extract the metadata for the files into a hash list
    $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $cam_id, $PS_EXIT_PROG_ERROR);

    # since I can't figure out how to do input and output within PERL, I'm writing to a temp file
    my ($statFile, $statName) = tempfile( "/tmp/$exp_tag.cm.$cam_id.stats.XXXX", UNLINK => !$save_temps );
    print "saving stats to $statName\n";
    foreach my $line (@$stdout_buf) {
        print $statFile $line;
    }
    close $statFile;

    # parse the stats in the metadata file
    $command = "$ppStatsFromMetadata $statName - CAMERA_EXP_IMFILE";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform ppStatsFromMetadata: $error_code\n");
        exit($error_code);
    }

    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;

    # Determine if FWHM is too large to bother continuing.
    my $maxFWHM = metadataLookupF32($recipeData, 'PSASTRO.MAX.PROCESS.FWHM');
    if ($maxFWHM) {
        my $expFWHM;
        ($expFWHM) = $cmdflags =~ /-fwhm_major (\d+)/;	
        
	if (not defined $expFWHM) {
	    print "FWHM not measured (no photometry?), Setting quality to 4077\n";
            $cmdflags .= " -quality 4077 "; # This corresponds to PSASTRO_ERR_DATA
            $skip_astrom = 1;
	} elsif ($expFWHM > $maxFWHM) {
            print "Setting quality to 4007 due to large FWHM: exposure: $expFWHM  maximum: $maxFWHM\n";
            $cmdflags .= " -quality 4007 "; # This corresponds to PSASTRO_ERR_DATA
            # $no_op = 1;
            $skip_astrom = 1; # skip astrometry, but make the jpegs
        }
    }
}

my $do_masks;               # Produce masks?
if (!$skip_masks) {
    $do_masks = metadataLookupBool($recipeData, 'REFSTAR_MASK');
}

### not needed to have such an extensive temp file name.
my ($list1File, $list1Name) = tempfile( "/tmp/$exp_tag.cm.$cam_id.b1.list.XXXX", UNLINK => !$save_temps ); # For binning 1
my ($list2File, $list2Name) = tempfile( "/tmp/$exp_tag.cm.$cam_id.b2.list.XXXX", UNLINK => !$save_temps ); # For binning 2
my ($list3File, $list3Name) = tempfile( "/tmp/$exp_tag.cm.$cam_id.b3.list.XXXX", UNLINK => !$save_temps ); # For astrometry

### XXX for the moment, always generate the bright-star mask
my ($list4File, $list4Name) = tempfile( "/tmp/$exp_tag.cm.$cam_id.b4.list.XXXX", UNLINK => !$save_temps ); # For astrometry

### Create temp file for background models
my ($list5File, $list5Name) = tempfile( "/tmp/$exp_tag.cm.$cam_id.b5.list.XXXX", UNLINK => !$save_temps ); # For background models


my $do_stats;
my $do_bkg;
my $do_jpegs;
my $fpaStats; 
my $psastroInputArg;
if ($run_state eq 'new') {
    $do_stats = 1;
    $do_bkg = 1;
    $do_jpegs = !$skip_binned;
    $fpaStats = prepare_output("PSASTRO.STATS",      $outroot, undef, 1);
    $psastroInputArg = " -list $list3Name";
} else {
    # for $run_state eq 'update' we only rebuild the masks using psastro
    $do_stats = 0;
    $do_jpegs = 0;
    $do_bkg = 1;        # we could skip this step if camProcessedExp.backgroun_model is non zero

    if (!$do_masks) {
        &my_die("run_state is update but do_masks is F. I have nothing to do!!", $cam_id, $PS_EXIT_UNKNOWN_ERROR)
    }

    # the input to psastro is the original PSASTRO.OUTPUT
    
    # Check for file with rule PSASTRO.OUTPUT.ORGINAL
    # This file will exist if we attempted to update this camRun before but faulted
    # XXX: make sure cleanup deals with these files
    my $inputObjects = $ipprc->filename("PSASTRO.OUTPUT.ORIGINAL", $outroot, undef);
    if (!$ipprc->file_exists($inputObjects)) {
        # not found so original file should still be in place. 
        # Rename it.
        my $originalObjects = $ipprc->filename("PSASTRO.OUTPUT", $outroot, undef);
        if ($ipprc->file_exists($originalObjects)) {

            unless ($ipprc->file_rename($originalObjects, $inputObjects)) {
                &my_die("failed to rename $originalObjects to $inputObjects", $cam_id, $PS_EXIT_UNKNOWN_ERROR);
            }

            # ok ready to go
        } else {
            print STDERR "failed to find input objects to update\n";
            print STDERR "Original file name: $originalObjects\n";
            print STDERR "Saved file name:    $inputObjects\n";

            &my_die("Cannot proceed.", $cam_id, $PS_EXIT_DATA_ERROR);

            # XXX: actually we could use the chip stage cmfs as inputs and turn on astrometry 
            # ... except that somebody (me) got the bright idea to save space and 
            # changed chip stage cleanup to remove them. So we're kind of stuck.
        }
    }
    $psastroInputArg = " -file $inputObjects -skipastro";
}
    

my @outMasks;                   # Names of output masks
my @bkg_models;                 # Names of output background models
foreach my $file (@$files) {
    # we perform astrometry iff photometry output exists
    next if $file->{quality} != 0;

    # use the path_base as OUTPUT root and convert the filenames with ipprc->filename:
    my $class_id = $file->{class_id};

    # If there is only one chip, we use this name for the input to addstar
    # we expect the chip analysis stage to produce psphot output (cmf file) and two binned images
    my $chipObjects = $ipprc->filename("PSPHOT.OUTPUT", $file->{path_base}, $class_id);
    my $chipMask   = $ipprc->filename("PPIMAGE.CHIP.MASK", $file->{path_base}, $class_id);
    
    print $list1File ($ipprc->filename("PPIMAGE.BIN1", $file->{path_base}, $class_id) . "\n");
    print $list2File ($ipprc->filename("PPIMAGE.BIN2", $file->{path_base}, $class_id) . "\n");
    print $list3File ($chipObjects . "\n");
    print $list4File ($chipMask . "\n");
    print $list5File ($ipprc->filename("PSPHOT.BACKMDL", $file->{path_base}, $class_id) . "\n");

    push @outMasks, prepare_output("PSASTRO.OUTPUT.MASK", $outroot, $class_id, 1) if $do_masks;
    push @bkg_models, prepare_output("PPIMAGE.BACKMDL", $outroot, $class_id, 1) if $do_bkg;
}
close $list1File;
close $list2File;
close $list3File;
close $list4File;
close $list5File;

# Prepare the Output products

# the camera configurations should define the psastro output to be a single file (MEF), regardless of the inputs
my $jpeg1      = prepare_output("PPIMAGE.JPEG1",      $outroot, undef, 1) if $do_jpegs;
my $jpeg2      = prepare_output("PPIMAGE.JPEG2",      $outroot, undef, 1) if $do_jpegs;
my $fpaObjects = prepare_output("PSASTRO.OUTPUT",     $outroot, undef, 1) if (!$bkg_only && !$skip_astrom);

my $configuration;
if ($run_state eq 'new') {
    $configuration = prepare_output("PSASTRO.CONFIG",  $outroot, undef, 1);
} else {
    # Do not use the original recipes for updates because they might contain the 
    # recipe values that caused the masks to get fouled up in the first place
    # $configuration = $ipprc->filename("PSASTRO.CONFIG",  $outroot, undef);
}


unless ($no_op) {

    ## build the output JPEG images first so we get them even if the astrometry fails

    if ($do_jpegs) {
        # Make the jpeg for binning 1
        my $command = "$ppImage -list $list1Name $outroot"; # Command to run
        $command .= " -recipe PPIMAGE $recipe1";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppImage: $error_code", $cam_id, $error_code);
        }
        check_output($jpeg1, $replicateOutputs);

        # Make the jpeg for binning 2

        $command = "$ppImage -list $list2Name $outroot"; # Command to run
        $command .= " -recipe PPIMAGE $recipe2";
        $command .= " -dbname $dbname" if defined $dbname;

        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppImage: $error_code", $cam_id, $error_code);
        }
        check_output($jpeg2, $replicateOutputs);
    }

    if (!$bkg_only && !$skip_astrom) {
        # run psastro on the chipObjects, producing fpaObjects
        my $command;
        $command  = $psastro;
        $command .= " $psastroInputArg";
        $command .= " -masklist $list4Name" if $do_masks;
        $command .= " $outroot";
        $command .= " -recipe PSASTRO $recipe_psastro";
        $command .= " -Db PSASTRO:REFSTAR_MASK F" if !$do_masks;
        $command .= " -tracedest $traceDest -log $logDest";
        $command .= " -dbname $dbname" if defined $dbname;

        if ($run_state eq 'new') {
            $command .= " -dumpconfig $configuration";
        } elsif ($run_state eq 'update') {
            $command .= " -ipprc $configuration" if $configuration;
        } else {
            &my_die("invalid value for run-state: $run_state", $cam_id, $PS_EXIT_CONFIG_ERROR);
        }
        $command .= " -stats $fpaStats -recipe PPSTATS CAMSTATS" if $do_stats;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    print STDERR (join "\n", @$stderr_buf);
            &my_die("Unable to perform psastro: $error_code", $cam_id, $error_code);
        }

        my $quality;            # Quality flag
        if ($do_stats) {
            check_output($fpaStats, $replicateOutputs);

            my $fpaStatsReal = $ipprc->file_resolve($fpaStats);

            # parse stats from metadata
            $command = "$ppStatsFromMetadata $fpaStatsReal - CAMERA_EXP_FPA";
            ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform ppStatsFromMetadata: $error_code", $cam_id, $error_code);
            }
            foreach my $line (@$stdout_buf) {
                $cmdflags .= " $line";
            }
            chomp $cmdflags;

            ($quality) = $cmdflags =~ /-quality (\d+)/;
        }

        if (!$quality) {
            check_output($fpaObjects, $replicateOutputs);

            foreach my $outMask (@outMasks) {
                check_output($outMask, $replicateOutputs);
            }

            if ($run_state eq 'new') {
                check_output($configuration, $replicateOutputs);
            }
        }
    }

    # Construct FPA continuity corrected background images
    # if (($camera =~ /ISP/)||($camera =~ /HSC/)||($camera =~ /gpc2/i)) {
    if ($camera =~ /gpc1/i) {
	print "Generating FPA continuity corrected background images for GPC1 only\n";
	my $command;
	$command = "$ppImage";
	$command .= " -list $list5Name $outroot";
	$command .= " -recipe PPIMAGE $bkg_recipe";
	$command .= " -dbname $dbname" if defined $dbname;

	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform ppImage to fix background: $error_code", $cam_id, $error_code);
	}
	foreach my $bkgModel (@bkg_models) {
	    check_output($bkgModel, $replicateOutputs);
	}
        if ($run_state eq 'new') {
            $cmdflags .= " -background_model 1 ";
        } else {
            my $command = "camtool -updateprocessedexp -set_background_model 1 -fault 0 -cam_id $cam_id";
            $command .= " -dbname $dbname" if $dbname;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to updateprocessedexp to set background_model: $error_code", $cam_id, $error_code);
            }
        }
    }
}

my $dtime_script = (DateTime->now->mjd - $mjd_start) * 86400;

my $fpaCommand = "$camtool -cam_id $cam_id";
if ($run_state eq 'new') {
    $fpaCommand .= " -addprocessedexp";
    $fpaCommand .= " -uri UNKNOWN";
    $fpaCommand .= " -path_base $outroot";
    $fpaCommand .= " $cmdflags";
    $fpaCommand .= " -hostname $host" if defined $host;
    $fpaCommand .= " -dtime_script $dtime_script";
} else {
    $fpaCommand .= " -updaterun -set_state full";
}
$fpaCommand .= " -dbname $dbname" if defined $dbname;

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

exit 0;

sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $class_id = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, $class_id, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $cam_id, $error);

    return $output;
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;

    if (!defined $file) {
        return;
    }

    &my_die("Couldn't find expected output file: $file",  $cam_id, $PS_EXIT_SYS_ERROR) unless
        $ipprc->file_exists($file);

    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack",  $cam_id, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file",  $cam_id, $PS_EXIT_SYS_ERROR);
	}
    }
    #####


    my $scheme = file_scheme($file);
    if ($replicate and $scheme and (file_scheme($file) eq 'neb')) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $cam_id, $PS_EXIT_SYS_ERROR);
    }
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $cam_id = shift; # Camtool identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $cam_id and not $no_update) {
        my $command = "$camtool -cam_id $cam_id";
        if ($run_state eq 'new') {
            $command .= " -addprocessedexp";
            $command .= " -uri UNKNOWN";
            $command .= " -fault $exit_code";
            $command .= " -path_base $outroot";
            $command .= " -path_base $outroot" if defined $outroot;
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
            $command .= " -hostname $host" if defined $host;
        } else {
            $command .= " -updateprocessedexp";
            $command .= " -fault $exit_code";
        }
        $command .= " -dbname $dbname" if defined $dbname;
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
