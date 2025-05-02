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

use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );
use File::Basename;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $ppImage = can_run('ppImage') or (warn "Can't find ppImage" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $nebrepair = can_run('neb-repair') or (warn "Can't find neb-repair" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

# Parse the command-line arguments
my ( $exp_id, $chip_id, $class_id, $chip_imfile_id, $uri, $camera, $outroot, $dbname, $run_state, $reduction, $threads, $verbose,
     $no_update, $save_temps, $no_op, $redirect, $magicked, $deburned, $update_mode );

GetOptions(
    'exp_id=s'          => \$exp_id,    # Exposure identifier
    'chip_id=s'         => \$chip_id,   # Chiptool identifier
    'class_id=s'        => \$class_id,  # Class identifier
    'chip_imfile_id=s'  => \$chip_imfile_id, # Unique file identifier
    'uri|u=s'           => \$uri,       # Input FITS file
    'camera|c=s'        => \$camera,    # Camera
    'outroot|w=s'       => \$outroot,   # output file base name
    'dbname|d=s'        => \$dbname,    # Database name
    'reduction=s'       => \$reduction, # Reduction class
    'run-state=s'       => \$run_state, # current state of the run (new, update)
    'update-mode=s'     => \$update_mode, # update_mode if non-zero do not use configdump
    'magicked=s'        => \$magicked,  # magicked state of input file
    'deburned=s'        => \$deburned,  # does deburned image exist?
    'threads=s'         => \$threads,   # Number of threads to use for ppImage
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op,     # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --exp_id --chip_id --chip_imfile_id --class_id --uri --camera --outroot --run-state --dbname",
           -exitval => 3) unless
    defined $exp_id and
    defined $chip_id and
    defined $class_id and
    defined $chip_imfile_id and
    defined $uri and
    defined $camera and
    defined $outroot and
    defined $dbname and
    defined $run_state;

my_die ("$run_state is an invalid value for run-state", $exp_id, $chip_id, $class_id, $PS_EXIT_PROG_ERROR) unless ($run_state eq 'new' or $run_state eq 'update');

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $exp_id, $chip_id, $class_id, $PS_EXIT_CONFIG_ERROR );

my $burntoolStateGood;
my $burntoolStateGoodUpdate;
my $neb;
my $scheme = file_scheme($outroot);
if ($scheme and $scheme eq 'neb') {
    $neb = $ipprc->nebulous();
}

my ($logRule, $traceDest);
if ($run_state eq 'new') {
    $logRule = "LOG.IMFILE";
    $traceDest = prepare_output("TRACE.IMFILE",  $outroot, $class_id, 1);
} else {
    $logRule = "LOG.IMFILE.UPDATE";
    $traceDest = prepare_output("TRACE.IMFILE.UPDATE",  $outroot, $class_id, 1);
}

if ($redirect) {
    my $logDest = $ipprc->filename($logRule, $outroot, $class_id);

    $ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR );

    print STDOUT "\n\n";
    print STDOUT "Starting script $0 on $host\n\n";
    print STDOUT "FULL COMMAND: $0 @ARGS\n\n";
}


# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_ppImage = $ipprc->reduction($reduction, 'CHIP_PPIMAGE'); # Recipe to use for ppImage
my $recipe_psphot  = $ipprc->reduction($reduction, 'CHIP_PSPHOT'); # Recipe to use for psphot
unless ($recipe_ppImage and $recipe_psphot) {
    &my_die("Couldn't find selected reduction for CHIP_PPIMAGE and CHIP_PSPHOT: $reduction\n", $exp_id, $chip_id, $class_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# &my_die("Couldn't find input file: $uri\n", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($uri);

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

my $source_id = $ipprc->source_id($dbname, $PS_TABLE_ID_CHIP);

## these names are used in ppImage, and thus may be URIs
my $outputImage   = prepare_output("PPIMAGE.CHIP",          $outroot, $class_id, 1 );
my $outputMask    = prepare_output("PPIMAGE.CHIP.MASK",     $outroot, $class_id, 1);
my $outputWeight  = prepare_output("PPIMAGE.CHIP.VARIANCE", $outroot, $class_id, 1);
my $pattern       = prepare_output("PPIMAGE.PATTERN",       $outroot, $class_id, 1);
my $backmdl       = prepare_output("PSPHOT.BACKMDL",        $outroot, $class_id, 1);

my $configuration;
my $outputSources;
my $outputPsf;
my $outputStats;
my $outputBin1;
my $outputBin2;
my $dump_config = 1;

# Note: We currently always do_stats because we need the quality flags
# If run_state eq 'update' using the same file name wipes out the stats from the original processing which
# is arguably a bug.  Since they are already in the database in that case this probably isn't a big
# deal
my $do_stats = 1;   
$outputStats   = prepare_output("PPIMAGE.STATS",         $outroot, $class_id, 1);

my $do_binned_images = 1;

if ($run_state eq 'new') {
    # prepare the files that are only created for a new run
    $configuration = prepare_output("PPIMAGE.CONFIG",        $outroot, $class_id, 1);
} else {
    # If update_mode is non-zero we are going to update with current recipes. 
    # In this case dump the new configuration.
    if ($update_mode) {
        $configuration = prepare_output("PPIMAGE.CONFIG",        $outroot, $class_id, 1);
    } else {
        $configuration = $ipprc->filename('PPIMAGE.CONFIG', $outroot, $class_id) 
            or &my_die("Missing entry from camera config: PPIMAGE.CONFIG", $exp_id, $chip_id, $class_id, $PS_EXIT_CONFIG_ERROR);
        if ($ipprc->file_exists($configuration)) {
            $dump_config = 0;
        } else {
            print STDERR "WARNING: Config dump file $configuration is missing. Using current recipes and file rules.\n";

            # XXX: should we create a new config dump file?
            # I vote yes but only if we can distingusing between temporarily unavailable and GONE.
            my $gone = 0;
            if (storage_object_exists($configuration, \$gone)) {
                if ($gone) {
                    $configuration = prepare_output('PPIMAGE.CONFIG', $outroot, $class_id, 1);
                    # if we dump the config we need to insure that the config dump represents
                    # the full processing
                } else {
                    # file is temporarily not available. Don't dump config.
                    $dump_config = 0;
                }
            }
        }
    }

    # make sure that any lingering destreak backup files are gone
    $ipprc->delete_destreak_backup_file($outputImage)
        or &my_die("failed to delete existing destreak backup image file", $exp_id, $chip_id, $class_id, $PS_EXIT_UNKNOWN_ERROR);
    $ipprc->delete_destreak_backup_file($outputMask)
        or &my_die("failed to delete existing destreak backup mask file", $exp_id, $chip_id, $class_id, $PS_EXIT_UNKNOWN_ERROR);
    $ipprc->delete_destreak_backup_file($outputWeight)
        or &my_die("failed to delete existing destreak backup weight file", $exp_id, $chip_id, $class_id, $PS_EXIT_UNKNOWN_ERROR);

    # don't do binned images when updating
    $do_binned_images = 0;
}
if ($do_binned_images) {
    $outputBin1    = prepare_output("PPIMAGE.BIN1",          $outroot, $class_id, 1);
    $outputBin2    = prepare_output("PPIMAGE.BIN2",          $outroot, $class_id, 1);
}

my $cmdflags;

# Run ppImage
unless ($no_op) {
    my $command;

    ## get the ppImage recipe for this camera and CHIP reduction
    $command = "$ppConfigDump -dump-recipe PPIMAGE -recipe PPIMAGE $recipe_ppImage -";
    if ($dump_config) {
        # use this camera's recipes
        $command .= " -camera $camera";
    } else {
        # get recipes as set in config dump file
        $command .= " -ipprc $configuration";
    }
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 0);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
    }
    my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);

    my $do_photom = metadataLookupBool($recipeData, 'PHOTOM');
    if ($do_photom and ($run_state eq 'update')) {
        # If previous psf file is ok skip photometry
        if (rerun_photometry($outroot, $class_id)) {
            carp "Will rerun photometry\n";
        } else {
            $do_photom = 0;
        }
    }
    if ($do_photom) {
        $outputSources = prepare_output("PSPHOT.OUTPUT",   $outroot, $class_id, 1);
        $outputPsf     = prepare_output("PSPHOT.PSF.SAVE", $outroot, $class_id, 1);
    }

    my $no_compress_image = metadataLookupBool($recipeData, 'NO.COMPRESS');

    my $useDeburnedImage = metadataLookupBool($recipeData, 'USE.DEBURNED.IMAGE');
    my $ppImageApplyBurntool = metadataLookupBool($recipeData, 'APPLY.BURNTOOL');
    my $burntoolArguments;

    if ($useDeburnedImage) {
        my $useBestBurntool  = metadataLookupBool($recipeData, 'USE.BEST.BURNTOOL');

        ## Check that we have required programs:
#       print STDERR "Inside burntool loop!\n";
        my $regtool  = can_run('regtool') or &my_die ("Can't find regtool", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
        my $burntool = can_run('burntool') or &my_die ("Can't find burntool", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);

        ## Check the current burntool processing version:
        my $regtool_state_cmd = "$regtool -processedimfile -exp_id $exp_id -class_id $class_id -limit 1";
        if (defined($dbname)) {
            $regtool_state_cmd .= " -dbname $dbname";
        }
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $regtool_state_cmd, verbose => $verbose);

        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform regtool: $error_code", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
        }

        my $regData = $mdcParser->parse(join "", @$stdout_buf) or
            &my_die("Unable to parse regtool metadata", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
        my $regData2 = parse_md_list($regData);

        my $burntoolState = 999;
        foreach my $regEntry (@$regData2) {
           print "$regEntry->{exp_id} $regEntry->{burntool_state}\n";

            if ($regEntry->{exp_id} == $exp_id) {
                $burntoolState = $regEntry->{burntool_state};
            }
        }
        if ($burntoolState == 999) {
            &my_die("Unable to find burntool_state in metadata", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
        }
        if ($burntoolState == 32767) {
            $burntoolState = 0;
        }

        ## Read camera config to get the current good burntool state :
        my $ppConfigDump_cmd = "$ppConfigDump -camera GPC1 -get-key BURNTOOL.STATE.GOOD";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            IPC::Cmd::run(command => $ppConfigDump_cmd, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn ("Unable to perform ppConfigDump");
            exit($error_code);
        }

        # This is ugly, but doing a full parse for one entry is a bit wasteful.
        foreach my $line (split /\n/, (join "", @$stdout_buf)) {
            if ($line =~ /BURNTOOL.STATE.GOOD/) {
                $line =~ s/^\s+//;
                $burntoolStateGood = (split /\s+/, $line)[2];
                last;
            }
        }

        $ppConfigDump_cmd = "$ppConfigDump -camera GPC1 -get-key BURNTOOL.STATE.GOOD.UPDATE";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            IPC::Cmd::run(command => $ppConfigDump_cmd, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn ("Unable to perform ppConfigDump");
            exit($error_code);
        }

        # This is ugly, but doing a full parse for one entry is a bit wasteful.
        foreach my $line (split /\n/, (join "", @$stdout_buf)) {
            if ($line =~ /BURNTOOL.STATE.GOOD.UPDATE/) {
                $line =~ s/^\s+//;
                $burntoolStateGoodUpdate = (split /\s+/, $line)[2];
                last;
            }
        }

        if ($run_state eq 'new') {
            print "burntool state vs burntoolStateGood : $burntoolState vs $burntoolStateGood\n";
            $useBestBurntool = 0 if (abs($burntoolState) >= $burntoolStateGoodUpdate);
        } else {
            # if doing update go ahead if burntoolState is at least at BURNTOOL.STATE.GOOD.UPDATE
            print "burntool state vs burntoolStateGoodUpdate : $burntoolState vs $burntoolStateGoodUpdate\n";
            $useBestBurntool = 0 if (abs($burntoolState) >= $burntoolStateGoodUpdate);
        }

        if (abs($burntoolState) != $burntoolStateGood) {
            if ($useBestBurntool) {
                &my_die("Image burntool version does not match current accepted version.", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
            }
            else {
                warn ("Image burntool version does not match current accepted version. Continuing as requested.");
                # Internally pretend that we're at a good version.
                if ($burntoolState < -10) {
                    $burntoolState = -1 * $burntoolStateGood;
                }
                elsif ($burntoolState > 10) {
                    $burntoolState = $burntoolStateGood;
                }
                elsif ($burntoolState == 0) {
                    # You've told me to use a deburned image, and that you don't care if it's the most recent. The database has told me
                    # that burntool has never been run on this image. We'll hope the database is wrong, but I don't really trust that.
                    warn ("burntool_state suggests no table will be found. This will likely crash.");
                    $burntoolState = -1 * $burntoolStateGood;
                }
                else {
                    &my_die("No valid burntool table will be found.", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
                }
            }
        }
        # resolve uri
        my $uriReal = $ipprc->file_resolve( $uri );
        # If instance is not found or if it doesn't exist try running neb-repair on it 
        # Note: file_exists returns false if the file exists but has zero size
        if (!$uriReal or !$ipprc->file_exists($uriReal)) {
            my $repair_cmd = "$nebrepair $uri";
            my ($repair_success, $repair_error_code, $repair_full_buf, $repair_stdout_buf, $repair_stderr_buf ) = run(command => $repair_cmd, verbose => $verbose);
            unless ($repair_success) {
                &my_die("Unable to attempt repair: $uri $repair_error_code", $exp_id,$chip_id, $class_id, $PS_EXIT_SYS_ERROR);
            }
            $uriReal = $ipprc->file_resolve( $uri );
        }
            
        &my_die("Unable to resolve $uri on $host", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR) if !$uriReal;

        # if recipe option PPIMAGE:APPLY.BURNTOOL is set and the burntool information is stored in
        # an external table (usual case), check the existence of that table file and set $burntoolArguments.
        # If burntool state does not match, say if the burntool data is stored in a fits extension in
        # the rawImfile, use the burntool program.
        if ($ppImageApplyBurntool && ($burntoolState == -1 * $burntoolStateGood)) {
            my $burntoolTable_uri = $uri;
            if($burntoolStateGood >= 15) {
              $burntoolTable_uri =~ s/fits$/burn.v15.tbl/;	    
            } else {
              $burntoolTable_uri =~ s/fits$/burn.tbl/;  	    
            }
            my $burntoolTable_uriReal = $ipprc->file_resolve( $burntoolTable_uri );
            if ((!$burntoolTable_uriReal)||(!($ipprc->file_exists($burntoolTable_uri)))) {
                my $repair_cmd = "$nebrepair $burntoolTable_uri";
                my ($repair_success, $repair_error_code, $repair_full_buf, $repair_stdout_buf, $repair_stderr_buf ) 
                    = run(command => $repair_cmd, verbose => $verbose);
                unless ($repair_success) {
                    &my_die("Unable to attempt repair: $uri $repair_error_code", $exp_id,$chip_id, $class_id,
                        $PS_EXIT_SYS_ERROR);
                }
                $burntoolTable_uriReal = $ipprc->file_resolve( $burntoolTable_uri );
            }
            unless ($ipprc->file_exists($burntoolTable_uri)) {
                &my_die("Couldn't find burntool table: $burntoolTable_uri",$exp_id,$chip_id,$class_id, 
                    $PS_EXIT_SYS_ERROR);
            }
            $burntoolArguments = " -burntool $burntoolTable_uri";
        } else {
            ## use external burntool program 

            ## We now know that we have an image that has been burntooled.
            my ($tempFile, $tempName) = tempfile( "/tmp/chip.$exp_id.$class_id.deburned.XXXX",
                                              UNLINK => !$save_temps, SUFFIX => '.fits' );

            # funpack into the temp file.
            my $funpack_cmd = "$funpack -S $uriReal > $tempName";
            ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $funpack_cmd, verbose => $verbose);
            unless ($success) {
                # Catch errors here
                my $repair_cmd = "$nebrepair $uri";
                my ($repair_success, $repair_error_code, $repair_full_buf, $repair_stdout_buf, $repair_stderr_buf ) = run(command => $repair_cmd, verbose => $verbose);
                unless ($repair_success) {
                    &my_die("Unable to attempt repair: $uri $repair_error_code", $exp_id,$chip_id, $class_id, $PS_EXIT_SYS_ERROR);
                }
                ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $funpack_cmd, verbose => $verbose);
            }
            unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform funpack: $error_code", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
            }

            ## Construct commands to apply the pre-calculated burntool trailfits to the pixel data.
            my ($burntoolTable_uri, $burntoolTable_uriReal);
            my $burntool_cmd = "$burntool ";

            if ($burntoolState == -1 * $burntoolStateGood) {   # Burntool information stored in an external table.
                $burntoolTable_uri = $uri;
                if($burntoolStateGood >= 15) {
                  $burntoolTable_uri =~ s/fits$/burn.v15.tbl/;	    
                } else {
                  $burntoolTable_uri =~ s/fits$/burn.tbl/;  	    
                }
                $burntoolTable_uriReal = $ipprc->file_resolve( $burntoolTable_uri );
                if ((!$burntoolTable_uriReal)||(!($ipprc->file_exists($burntoolTable_uri)))) {
                    my $repair_cmd = "$nebrepair $burntoolTable_uri";
                    my ($repair_success, $repair_error_code, $repair_full_buf, $repair_stdout_buf, $repair_stderr_buf ) = run(command => $repair_cmd, verbose => $verbose);
                    unless ($repair_success) {
                        &my_die("Unable to attempt repair: $uri $repair_error_code", $exp_id,$chip_id, $class_id, $PS_EXIT_SYS_ERROR);
                    }
                    $burntoolTable_uriReal = $ipprc->file_resolve( $burntoolTable_uri );
                }
                unless ($ipprc->file_exists($burntoolTable_uri)) {
                    # Catch errors here
                    &my_die("Couldn't find burntool table: $burntoolTable_uri",$exp_id,$chip_id,$class_id, $PS_EXIT_SYS_ERROR);
                }

                $burntool_cmd .= "$tempName in=${burntoolTable_uriReal} persist=t apply=t";
            }
            elsif ($burntoolState == $burntoolStateGood) { # Burntool information stored in a header table.
                $burntool_cmd .= "$tempName persist=t apply=t";
            }
            else {
                &my_die("Image data not properly burntooled, impossible state", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
            }

            ## Run burntool to change the pixels of tempfile, and repoint $uri to that file.
            ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) =
                run(command => $burntool_cmd, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform burntool: $error_code", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
            }

            $uri = $tempName;
            unless ($ipprc->file_exists($uri)) {
                &my_die("Couldn't find deburned input file: $uri\n", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
            }

            if ($dump_config) {
                # tell ppImage to not apply burntool since we have done it here
                $burntoolArguments = ' -Db PPIMAGE:APPLY.BURNTOOL F';
            }
    #       print STDERR "$uri $uriReal $tempName $burntoolTable_uri $burntoolTable_uriReal $burntool_cmd $save_temps\n";
    #       exit(100);

        }
    }

    my $tiltystreakApply = metadataLookupBool($recipeData, 'TILTYSTREAK.APPLY');
    if ($tiltystreakApply) {
        # NOTE: application of TILTYSTREAK.APPLY was here. Removed after r35685
        &my_die("Unable to perform tiltystreak: code has been removed", $exp_id, $chip_id, $class_id,
            $PS_EXIT_PROG_ERROR);
    }

    $command  = "$ppImage -file $uri $outroot";
    if ($dump_config) {
        $command .= " -recipe PPIMAGE $recipe_ppImage";
        $command .= " -dumpconfig $configuration";
    } else {
        $command .= " -ipprc $configuration";
    }
    if ($do_photom) {
        $command .= " -recipe PSPHOT $recipe_psphot";
    } else {
        $command .= " -Db PPIMAGE:PHOTOM FALSE";
    }
    if ($run_state eq "new" or $do_stats) {
        $command .= " -recipe PPSTATS CHIPSTATS";
        $command .= " -stats $outputStats";
        $do_stats = 1;
    }
    if (!$do_binned_images) {
        $command .= " -Db PPIMAGE:BIN1.FITS FALSE -Db PPIMAGE:BIN2.FITS FALSE";
    }
    if ($no_compress_image) {
        $command .= " -R PPIMAGE.CHIP FITS.TYPE NONE";
    }
    if ($run_state eq 'update' and metadataLookupBool($recipeData, 'MASK.STATS')) {
	$command .= " -Db PPIMAGE:MASK.STATS FALSE ";
    }
    $command .= $burntoolArguments if $burntoolArguments;
    $command .= " -threads $threads" if defined $threads;
    $command .= " -image_id $chip_imfile_id" if defined $chip_imfile_id;
    $command .= " -source_id $source_id" if defined $source_id;
    $command .= " -tracedest $traceDest";
    $command .= " -dbname $dbname" if defined $dbname;

    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppImage: $error_code", $exp_id, $chip_id, $class_id, $error_code);
    }

    ## allow the output images to be optional, depending on the recipe / reduction class
    my $outputImageExpect = metadataLookupBool($recipeData, 'CHIP.FITS');
    my $outputMaskExpect = metadataLookupBool($recipeData, 'CHIP.MASK.FITS');
    my $outputWeightExpect = metadataLookupBool($recipeData, 'CHIP.VARIANCE.FITS');
    my $outputBackmdlExpect = metadataLookupBool($recipeData, 'BACKGROUND');
    my $outputPatternExpect = (metadataLookupBool($recipeData, 'PATTERN.ROW') or metadataLookupBool($recipeData, 'PATTERN.CELL')) ;

    my $quality;                # Quality flag
    if ($do_stats) {
        my $outputStatsReal = $ipprc->file_resolve($outputStats);
        &my_die("Couldn't find expected output file: $outputStats", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

        # measure chip stats
        $command = "$ppStatsFromMetadata $outputStatsReal - CHIP_IMFILE";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppStatsFromMetadata: $error_code", $exp_id, $chip_id, $class_id, $error_code);
        }
        foreach my $line (@$stdout_buf) {
            $cmdflags .= " $line";
        }
        chomp $cmdflags;
        ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag
    }

    if (!$quality) {
        my $replicateImages = 0;
        check_output($outputImage, $replicateImages) if $outputImageExpect;
        check_output($outputMask, $replicateImages) if $outputMaskExpect;
        check_output($outputWeight, $replicateImages) if $outputWeightExpect;
        if ($do_binned_images) {
            check_output($outputBin1, 1);
            check_output($outputBin2, 1);
        }
        check_output($configuration, 1) if $dump_config;
        check_output($backmdl, 1) if $outputBackmdlExpect;
	# allow the pattern file to be missing if run state is update older data doesn't have one
	# I should parse the config dump file to calculate the 'Expect' variables
        check_output($pattern, 1, $run_state eq 'update') if $outputPatternExpect;
        if ($do_photom) {
            check_output($outputSources, 1);
            check_output($outputPsf, 1);
        }
        # XXX: Do we want to replicate the stats, logs, trace file?
    }
}

my $command;
if ($run_state eq 'new') {
    # command to update database
    $command = "$chiptool -addprocessedimfile";
    $command .= " -exp_id $exp_id";
    $command .= " -chip_id $chip_id";
    $command .= " -class_id $class_id";
    $command .= " -uri $outputImage";
    $command .= " -path_base $outroot";
    $command .= " -magicked $magicked" if $magicked;
    $command .= " -hostname $host" if defined $host;
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " $cmdflags";
    $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
} else {
    $command = "$chiptool -tofullimfile";
    $command .= " -chip_id $chip_id";
    $command .= " -class_id $class_id";
    $command .= " -dbname $dbname" if defined $dbname;
    # XXX: if we had a quality problem on this re-run we are not saving that fact in the DB...
    # to change this we need to modify chiptool -tofullimfile to supprot -set_quality and then add
    #$command .= " -set_quality $quality" if $quality;
}

# Add the processed file to the database
my $tries = 0;
while (1) {
    $tries++;
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn("Unable to perform chiptool -addprocessedimfile: $error_code\n");
            exit($error_code) if $tries >= 3;
            warn("Waiting 10 seconds to try again\n");
            sleep 10;
        } else {
            last;
        }
    } else {
        print "skipping command: $command\n";
        last;
    }
}

exit 0;

# check whether psphot outputs should be regenerated.
# Whether we need to or not was a somewhat complicated question. Since we clean cmfs now we only rerun
# photometry if the psf file has gone missing since it is needed to run warps

sub rerun_photometry
{
    my $outroot = shift;
    my $class_id = shift;
    my $outputSources = $ipprc->filename('PSPHOT.OUTPUT', $outroot, $class_id) 
                    or &my_die("Missing entry from camera config: PSPHOT.OUTPUT", $exp_id, $chip_id, $class_id, $PS_EXIT_CONFIG_ERROR);

    my $update_sources_if_gone = 0; # set this to regenerate sources if gone. 
                                    # We no longer do this as of 2012-12 since cleanup deletes them
    my $make_sources = 0;
    my $sources_available = 0;
    if ($ipprc->file_exists($outputSources)) {
        $sources_available = 1;
    } else {
        if ($update_sources_if_gone) {
            carp "WARNING: photometry sources file $outputSources is not available";
            my $gone;
            if (storage_object_exists($outputSources, \$gone)) {
                # check whether the file is permanantely or temporarily gone
                if ($gone) {
                    carp "WARNING: photometry sources storage object exists but all instances are permanently gone";
		    $ipprc->file_rename($outputSources,$outputSources . ".gone");
                    $make_sources = 1;
                }
            } else {
                # storage object must have been deleted
                $make_sources = 1;
            }
        }
    }

    my $make_psf = 0;
    my $psf_available = 0;
    my $outputPsf = $ipprc->filename("PSPHOT.PSF.SAVE",       $outroot, $class_id)
                    or &my_die("Missing entry from camera config: PSPHOT.PSF.SAVE",
                                $exp_id, $chip_id, $class_id, $PS_EXIT_CONFIG_ERROR);

    if ($ipprc->file_exists($outputPsf)) {
        $psf_available = 1;
    } else {
        carp "PSF file $outputPsf is missing";
        my $gone = 0;
        if (storage_object_exists($outputPsf, \$gone)) {
            # object exists, but no instances are available. If they are permanently gone
            # remake it
            if ($gone) {
                carp "WARNING: PSF storage object exists but all instances are permanently gone";
		$ipprc->file_rename($outputPsf,$outputPsf . ".gone");
                $make_psf = 1;
            }
        } else {
            # object completely missing remake it
            $make_psf = 1;
        }
    }

    if ($sources_available && $psf_available) {
        return 0;
    }

    if ($update_sources_if_gone) {
        if (!$sources_available && !$make_sources) {
            # destreak will die if the sources is not available
            # but magic is dead....
            &my_die("PSPHOT.SOURCES is missing but we cannot regenerate it", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
        }
    }

    if (!$psf_available && !$make_psf) {
        # warp updates need the psf file
        &my_die("PSPHOT.PSF.SAVE is missing but we cannot regenerate it", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
    }

    return $make_psf || $make_sources;
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
            &my_die("Can't find whichnode",  $exp_id, $chip_id, $class_id, $PS_EXIT_CONFIG_ERROR);
    }

    my $command = "$whichnode $file";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform whichnode: $error_code", $exp_id, $chip_id, $class_id, $PS_EXIT_CONFIG_ERROR);
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
	} elsif ($answer eq 'permanently') {
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

# Prepare to write to an output file
#   Lookup the filename in the rules.
#   Make sure that if file exists and is a nebulous file that there is only one instance
#   Deal with files that have been lost.
sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $class_id = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, $class_id, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $exp_id, $chip_id, $class_id, $error);
    return $output;
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;
    my $allow_missing = shift;

    if (!defined $file) {
        return;
    }

    my $exists = $ipprc->file_exists($file);

    if (!$exists) {
        if ($allow_missing) {
            carp("Couldn't find expected output_file: $file but continuing anyways\n");
            return 1;
        }
        &my_die("Couldn't find expected output file: $file",  $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
    }
    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file", $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
	}
    }
    #####

    if ($replicate and $neb) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $exp_id, $chip_id, $class_id, $PS_EXIT_SYS_ERROR);
    }
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $exp_id = shift; # Chiptool identifier
    my $chip_id = shift; # Chiptool identifier
    my $class_id = shift; # Class identifier
    my $exit_code = shift; # Exit code to add
    # run_state, outputImage, and outroot are globals

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $chip_id and defined $class_id and not $no_update) {
        my $command;
        if ($run_state eq 'new') {
            $command = "$chiptool -addprocessedimfile";
            $command .= " -exp_id $exp_id";
            $command .= " -uri $outputImage" if defined $outputImage;
            $command .= " -path_base $outroot";
            $command .= " -hostname $host" if defined $host;
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        } else {
            $command .= "$chiptool -updateprocessedimfile";
        }
        $command .= " -chip_id $chip_id";
        $command .= " -class_id $class_id";
        $command .= " -fault $exit_code";
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
