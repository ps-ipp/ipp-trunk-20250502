#!/usr/bin/env perl

use warnings;
use strict;

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
use File::Spec;
use File::Temp qw( tempfile );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

# Look for programs we need
my $missing_tools;
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $pswarp = can_run('pswarp') or (warn "Can't find pswarp" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

my ($warp_id, $skycell_id, $warp_skyfile_id, $tess_dir, $reduction, $camera, $dbname, $outroot, $threads, $run_state, $magicked, $verbose, $no_update, $no_op, $redirect, $save_temps);

GetOptions(
    'warp_id|i=s'         => \$warp_id, # Warp identifier
    'skycell_id|s=s'      => \$skycell_id, # Skycell identifier
    'warp_skyfile_id|s=s' => \$warp_skyfile_id, # Unique file identifier
    'tess_dir|s=s'        => \$tess_dir, # Tesselation identifier
    'camera|c=s'          => \$camera, # Camera name
    'dbname|d=s'          => \$dbname, # Database name
    'reduction=s'         => \$reduction, # Reduction class
    'outroot=s'           => \$outroot, # Output root name
    'threads=s'           => \$threads,   # Number of threads to use for pswarp
    'run-state=s'         => \$run_state,  # 'new' or 'update'
    'magicked=s'          => \$magicked,  # input run has been magicked already?
    'verbose'             => \$verbose,   # Print to stdout
    'no-update'           => \$no_update, # Don't update the database?
    'no-op'               => \$no_op, # Don't do any operations?
    'redirect-output'     => \$redirect,
    'save-temps'          => \$save_temps, # Save temporary files?
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --warp_id --warp_skyfile_id --skycell_id --tess_dir --camera --outroot --run-state",
    -exitval => 3,
) unless defined $warp_id
    and defined $skycell_id
    and defined $warp_skyfile_id
    and defined $tess_dir
    and defined $camera
    and defined $outroot
    and defined $run_state;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my ($logDest, $traceDest);
my $do_stats;
if ($run_state eq 'new') {
    $logDest = $ipprc->filename("LOG.EXP", $outroot, $skycell_id);
    $traceDest = prepare_output("TRACE.EXP", $outroot, $skycell_id, 1);
    $do_stats = 1;
} elsif ($run_state eq 'update')  {
    $logDest = $ipprc->filename("LOG.EXP.UPDATE", $outroot, $skycell_id);
    $traceDest = prepare_output("TRACE.EXP.UPDATE", $outroot, $skycell_id, 1);
} else {
    &my_die( "invalid run_state: $run_state", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_CONFIG_ERROR );
}

my $neb;
my $scheme = file_scheme($outroot);
if ($scheme and $scheme eq 'neb') {
    $neb = $ipprc->nebulous();
}

$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR ) if $redirect;
print "FULL COMMAND: $0 @ARGS\n\n";

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_pswarp = $ipprc->reduction($reduction, 'WARP_PSWARP'); # Recipe to use
my $recipe_psastro = $ipprc->reduction($reduction, 'PSASTRO'); # Recipe to use
unless ($recipe_pswarp and $recipe_psastro) {
    &my_die("Couldn't find selected reduction: $reduction\n", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_CONFIG_ERROR);
}

my $source_id = $ipprc->source_id($dbname, $PS_TABLE_ID_WARP);

# Get list of component imfiles for exposure
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $imfiles;
{
    # XXX change -tess_id to -tess_dir when db schema is updated
    my $command = "$warptool -scmap";
    $command .= " -warp_id $warp_id";
    $command .= " -skycell_id $skycell_id";
    # $command .= " -tess_id $tess_dir";  XXX I don't think this is necessary or useful
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform warptool -scmap: $error_code", $warp_id, $skycell_id, $tess_dir, $error_code);
    }

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_PROG_ERROR);
    $imfiles = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_PROG_ERROR);
}

# Where do we get the astrometry source from?
my $astromSource;               # The astrometry source
my $doBackground;               # Do we want to make background models?
my $noCompression;
{
    my $command = "$ppConfigDump -camera $camera -recipe PSWARP $recipe_pswarp -dump-recipe PSWARP -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $warp_id, $skycell_id, $tess_dir, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_PROG_ERROR);
    $astromSource = metadataLookupStr($metadata, 'ASTROM.SOURCE');
    $doBackground = metadataLookupBool($metadata, 'BACKGROUND.MODEL');    
    $noCompression = metadataLookupBool($metadata, 'NO.COMPRESS');
}

my $dynamicMasks;               # Use dynamic masks?
{
    # Get the PSASTRO recipe
    my $command = "$ppConfigDump -camera $camera -recipe PSASTRO $recipe_psastro -dump-recipe PSASTRO -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $warp_id, $PS_EXIT_CONFIG_ERROR);
    }
    my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $warp_id, $PS_EXIT_CONFIG_ERROR);

    $dynamicMasks = metadataLookupBool($recipeData, 'REFSTAR_MASK');
}
print "Dynamic Mask Status: $dynamicMasks\n";

my $outputImage = prepare_output ("PSWARP.OUTPUT", $outroot, $skycell_id, 1);
my $outputMask = prepare_output ("PSWARP.OUTPUT.MASK", $outroot, $skycell_id, 1);
my $outputWeight = prepare_output ("PSWARP.OUTPUT.VARIANCE", $outroot, $skycell_id, 1);
my $outputSources = prepare_output ("PSWARP.OUTPUT.SOURCES", $outroot, $skycell_id, 1);
my $outputPSF = prepare_output ("PSPHOT.PSF.SKY.SAVE", $outroot, $skycell_id, 1);
my $outputBin1 = prepare_output ("PSWARP.BIN1", $outroot, $skycell_id, 1);
my $outputBin2 = prepare_output ("PSWARP.BIN2", $outroot, $skycell_id, 1);
my $outputStats;
if ($do_stats) {
    $outputStats = prepare_output ("SKYCELL.STATS", $outroot, $skycell_id, 1) if $do_stats;
}
my $outputBKGs;
if ($doBackground) {
    $outputBKGs = prepare_output ("PSWARP.OUTPUT.BKGMODEL", $outroot, $skycell_id, 1);
}
my $configuration;

my $dump_config = 1;
if ($run_state eq 'new') {
    $configuration =  prepare_output ("PSWARP.CONFIG", $outroot, $skycell_id, 1);
} else {
    $configuration =  $ipprc->filename("PSWARP.CONFIG", $outroot, $skycell_id) or
        &my_die("Missing entry from camera config PSWARP.CONFIG", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_CONFIG_ERROR);
    if ($ipprc->file_exists($configuration)) {
        $dump_config = 0;
    } else {
        print STDERR "WARNING: Config dump file $configuration is missing. Using current recipes and file rules.\n";

        # XXX: should we create a new config dump file?
        # I vote yes but only if we can distingusing between temporarily unavailable and GONE.
        my $gone = 0;
        if (storage_object_exists($configuration, \$gone)) {
            if ($gone) {
                $configuration = prepare_output('PSWARP.CONFIG', $outroot, $skycell_id, 1);
                # if we dump the config we need to insure that the config dump represents
                # the full processing
            } else {
                # file is temporarily not available. Don't dump config.
                $dump_config = 0;
            }
        }
    }
}


my $skyFile = prepare_output ("SKYCELL.TEMPLATE", $outroot, $skycell_id, 1);
$ipprc->skycell_file( $tess_dir, $skycell_id, $skyFile, $verbose ) or &my_die("Unable to generate template skycell", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR);
## XXX this seems to have insufficient error checking: dvoImageExtract can fail to write and still return a valid exit status

# Get list of filenames
my $tempOutRoot = "/tmp/warp.$camera.$warp_id.$skycell_id";
my ($imageFile,  $imageName)  = tempfile( "$tempOutRoot.image.list.XXXX",  UNLINK => !$save_temps);
my ($maskFile,   $maskName)   = tempfile( "$tempOutRoot.mask.list.XXXX",   UNLINK => !$save_temps);
my ($weightFile, $weightName) = tempfile( "$tempOutRoot.weight.list.XXXX", UNLINK => !$save_temps);
my ($astromFile, $astromName) = tempfile( "$tempOutRoot.astrom.list.XXXX", UNLINK => !$save_temps);
my ($bkgFile, $bkgName);
if ($doBackground) {
    ($bkgFile, $bkgName) = tempfile( "$tempOutRoot.bkg.list.XXXX", UNLINK => !$save_temps);
}
my $wrote_astrom = 0;
foreach my $imfile (@$imfiles) {
    my $image = $ipprc->filename ("PPIMAGE.CHIP", $imfile->{chip_path_base}, $imfile->{class_id}); # Image name
    my $weight = $ipprc->filename ("PPIMAGE.CHIP.VARIANCE", $imfile->{chip_path_base}, $imfile->{class_id}); # Mask name

    my $mask;                   # Mask name
    if ($dynamicMasks) {
        $mask = $ipprc->filename ("PSASTRO.OUTPUT.MASK", $imfile->{cam_path_base}, $imfile->{class_id});
    } else {
        $mask = $ipprc->filename ("PPIMAGE.CHIP.MASK", $imfile->{chip_path_base}, $imfile->{class_id});
    }

    &my_die("Couldn't find input file: $image", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($image);
    &my_die("Couldn't find input file: $weight", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($weight);
    &my_die("Couldn't find input file: $mask", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($mask);

    # Astrometry file: astrometry is done at the camera stage, and always results in a MEF file
    # XXX allow an option to use the image header astrometry?
    my $astrom = $ipprc->filename ($astromSource, $imfile->{cam_path_base});

    &my_die("Couldn't find input file: $astrom", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR) unless defined $astrom and $ipprc->file_exists($astrom);

    print $imageFile  "$image\n";
    print $maskFile   "$mask\n";
    print $weightFile "$weight\n";
    my $bkg;
    if (($doBackground)) {
	# &&($imfile->{background_model} == 1)
	if (($imfile->{cam_background_model})&&($imfile->{cam_background_model} != 32767)) {
	    $bkg    = $ipprc->filename("PPIMAGE.BACKMDL", $imfile->{cam_path_base}, $imfile->{class_id});
	}
	else {
	    $bkg    = $ipprc->filename("PPIMAGE.BACKMDL", $imfile->{chip_path_base}, $imfile->{class_id});
	}
	print $bkgFile "$bkg\n";
    }

    if (!$wrote_astrom) {
        print $astromFile "$astrom\n";
        $wrote_astrom = 1;
    }
}
close $imageFile;
close $maskFile;
close $weightFile;
close $astromFile;
if ($doBackground) {
    close($bkgFile);
}
# We need the recipe to determine if we care whether the PSF is generated or not
my $recipe;
{
    my $command = "$ppConfigDump -camera $camera -dump-recipe PSWARP -recipe PSWARP $recipe_pswarp -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR);
    }
    $recipe = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR);
}


# Run pswarp
my $cmdflags;
unless ($no_op) {
    my $command = "$pswarp";
    $command .= " -list $imageName";
    $command .= " -masklist $maskName";
    $command .= " -variancelist $weightName";
    $command .= " -astromlist $astromName";
    $command .= " -bkglist $bkgName" if ($doBackground);
    $command .= " $outroot $skyFile";
    $command .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE";
    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    $command .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF";
    $command .= " -F SOURCE.PLOT.MOMENTS SOURCE.PLOT.SKY.MOMENTS";
    $command .= " -F SOURCE.PLOT.PSFMODEL SOURCE.PLOT.SKY.PSFMODEL";
    $command .= " -F SOURCE.PLOT.APRESID SOURCE.PLOT.SKY.APRESID";
    $command .= " -R PSWARP.OUTPUT FITS.TYPE NONE" if $noCompression;
    $command .= " -recipe PSWARP $recipe_pswarp";
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -threads $threads" if defined $threads;
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -image_id $warp_skyfile_id" if defined $warp_skyfile_id;
    $command .= " -source_id $source_id" if defined $source_id;
    if ($run_state eq 'new') {
        $command .= " -dumpconfig $configuration";
    } else {
        $command .= " -ipprc $configuration";
    }
    if ($do_stats) {
        $command .= " -recipe PPSTATS WARPSTATS";
        $command .= " -stats $outputStats";
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform pswarp: $error_code", $warp_id, $skycell_id, $tess_dir, $error_code);
    }

    if ($do_stats) {
        # Check first for the stats file
        check_output($outputStats, 0);
        my $outputStatsReal = $ipprc->file_resolve($outputStats);
#        &my_die("Couldn't find expected output file: $outputStats", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);
        &my_die("Stats file has zero size: $outputStats", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR) unless -s $outputStatsReal;

        # measure skycell stats
        $command = "$ppStatsFromMetadata $outputStatsReal - WARP_SKYCELL";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppStatsFromMetadata: $error_code", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR);
        }
        foreach my $line (@$stdout_buf) {
            $cmdflags .= " $line";
        }
        chomp $cmdflags;

        my ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag

        if (!$quality) {
            check_output($outputImage, 0);
            check_output($outputMask, 0);
            check_output($outputWeight, 0);
            check_output($outputSources, 1);
            check_output($outputPSF, 1) if metadataLookupBool($recipe, 'PSF')  ;
            if ($dump_config)  {
                check_output($configuration, 1);
            }
        }
	print "Quality: $quality\n";
        unless ($no_update) {
            # XXX change -tess_id to -tess_dir when db is updated
            my $command = "$warptool -addwarped";
            $command .= " -warp_id $warp_id";
            $command .= " -skycell_id $skycell_id";
            $command .= " -tess_id $tess_dir";
            $command .= " -path_base $outroot"; # needed for logfile lookups
            $command .= " -set_magicked $magicked" if $magicked;

            $command .= " -uri $outputImage" if !$quality;
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
            $command .= " $cmdflags";
            $command .= " -hostname $host"   if defined $host;
            $command .= " -dbname $dbname"   if defined $dbname;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                warn("Unable to perform warptool -addwarped: $error_code\n");
                exit($error_code);
            }
        }
    } else {

	my $quality = 0;

	unless ($ipprc->file_exists($outputPSF)) {
	    $quality = 8006; # bad data.
	}
	print "Quality: $quality\n";
        if (!$quality) {
            check_output($outputImage, 0);
            check_output($outputMask, 0);
            check_output($outputWeight, 0);
            check_output($outputSources, 0);
            check_output($outputPSF, 0) if metadataLookupBool($recipe, 'PSF')  ;
        }

        # $run_state eq 'update'
        unless ($no_update) {
	    my $try = 0;
	    while (1) {
                $try++;
                my $command = "$warptool -tofullskyfile";
                $command .= " -warp_id $warp_id";
                $command .= " -skycell_id $skycell_id";
                $command .= " -set_magicked $magicked" if $magicked;
                $command .= " -set_quality $quality" if $quality;
                $command .= " -dbname $dbname"   if defined $dbname;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    warn("Unable to perform warptool -tofullskyfile: $error_code\n");
                    exit($error_code) if $try >= 2;
                    sleep 10;
                    warn("Trying again\n");
                } else {
                    last;
                }
            }
        }
    }
}

exit 0;

# Prepare to write to an output file
#   Lookup the filename in the rules.
#   Make sure that if file exists and is a nebulous file that there is only one instance
#   Deal with files that have been lost.
sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $skycell_id = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, $skycell_id, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $warp_id, $skycell_id, $tess_dir, $error);
    return $output;
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;

    if (!defined $file) {
        return;
    }

    &my_die("Couldn't find expected output file: $file",  $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($file);

    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR);
	}
    }
    #####

    if ($replicate and $neb) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $warp_id, $skycell_id, $tess_dir, $PS_EXIT_SYS_ERROR);
    }
}

# subroutine to check the status of a nebulous file. Used to distinguish between a storage object that
# does not exist and one that all of the instances have been lost.
# XXXX This should be implemented properly in Nebulous
# For now uses Bill's script 'whichnode' which queries the nebulous database directly

my $whichnode;
sub storage_object_exists
{
    return 0 if !$neb;

    my $file = shift;
    my $ref_all_gone = shift;

    my $exists = $neb->storage_object_exists($file);
    if (!$exists) {
        return 0;
    }

    if (!$whichnode) {
        $whichnode = can_run('whichnode') or
            &my_die("Can't find whichnode",  $warp_id, $skycell_id, $tess_dir, $PS_EXIT_CONFIG_ERROR);
    }

    my $command = "$whichnode $file";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform whichnode: $error_code", $warp_id, $skycell_id, $tess_dir, $PS_EXIT_CONFIG_ERROR);
    }

    my @lines = split "\n", (join "", @$stdout_buf);

    if (scalar @lines == 0) {
        # no output the file is really and truely gone
        # XXX: this is now caught above
        print STDERR "storage object for $file does not exist\n";
        return 0;
    }

    my $numGone = 0;
    my $numNotGone = 0;
    foreach my $line (@lines) {
        chomp $line;

        # output lines are either
        #   "volume available"
        # or 
        #   "volume not available"

        my ($volume, $answer, undef) = split " ", $line;
        # our hack is if the volume has an X in the name it's gone
        if ($volume =~ /X/) {
            print STDERR "$file is on $volume which is gone\n";
            $numGone++;
        } elsif ($answer eq 'available') {
            $numNotGone++;
        } elsif ($answer eq 'not') {
            print STDERR "$file is on $volume which is not available\n";
            $numNotGone++;
        } else {
            print STDERR "unexpected output from whichnode: $line\n";
        }
    }
    # if there are any instances that are not on a gone volume set all_gone to 0
    if ($numNotGone == 0 and $numGone > 0) {
        $$ref_all_gone = 1;
    } else {
        $$ref_all_gone = 0;
    }

    # storage object exists so return true
    return 1;
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $warp_id = shift;        # Warp identifier
    my $skycell_id = shift;     # Skycell identifier
    my $tess_dir = shift;        # Tesselation identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $warp_id and defined $skycell_id and defined $tess_dir and not $no_update) {
        # XXX change -tess_id to -tess_dir when db is updated
        my $command = "$warptool";
        if ($run_state eq 'new') {
            $command .= " -addwarped";
            $command .= " -tess_id $tess_dir";
            $command .= " -path_base $outroot";
            $command .= " -hostname $host" if defined $host;
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        } else {
            $command .= " -updateskyfile";
        }
        $command .= " -warp_id $warp_id";
        $command .= " -skycell_id $skycell_id";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        run(command => $command, verbose => $verbose);
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
