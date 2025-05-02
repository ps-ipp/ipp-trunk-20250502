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
use File::Temp qw( tempfile tempdir );
use File::Basename qw( basename dirname );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );
use Nebulous::Client;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $magicdstool   = can_run('magicdstool') or (warn "Can't find magicdstool" and $missing_tools = 1);
my $streaksremove = can_run('streaksremove') or (warn "Can't find streaksremove" and $missing_tools = 1);
my $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $censorObjects = can_run('censorObjects') or (warn "Can't find censorObjects" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $dvoImageOverlaps = can_run('dvoImageOverlaps') or (warn "Can't find dvoImageOverlaps" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($magic_ds_id, $camera, $streaks, $inv_streaks, $exp_id, $stage, $stage_id, $component, $uri, $path_base, $cam_path_base, $cam_reduction);
my ($streaks_path_base, $inv_streaks_path_base, $run_state);
my ($outroot, $recoveryroot, $magicked);
my ($replace, $release);
my ($diff_tess_id, $mismatched_tess);
my ($dbname, $save_temps, $verbose, $no_update, $no_op, $logfile);

GetOptions(
           'magic_ds_id=s'  => \$magic_ds_id,# Magic destreak run identifier
           'camera=s'       => \$camera,     # camera for evaluating file rules
           'run-state=s'    => \$run_state,   # state of run (new or update)
           'streaks_path_base=s'      => \$streaks_path_base,    # path_base for streaks data
           'inv_streaks_path_base=s'  => \$inv_streaks_path_base, #path_base for streaks from inverse diff
           'inv_streaks=s'  => \$inv_streaks,# file containing the list of streaks from the inverse diff
           'streaks=s'      => \$streaks,    # file containing the list of streaks
           'inv_streaks=s'  => \$inv_streaks,# file containing the list of streaks from the inverse diff
           'exp_id=s'       => \$exp_id,     # exp_id, chip_id, warp_id, or diff_id
           'stage=s'        => \$stage,      # raw, chip, warp, or diff
           'stage_id=s'     => \$stage_id,   # exp_id, chip_id, warp_id, or diff_id
           'component=s'    => \$component,  # the class_id or skycell_id
           'mismatched_tess'=> \$mismatched_tess, # true if tess_id of input does not match tess_id of the diff used for magic
           'diff_tess_id=s' => \$diff_tess_id, # tess_id of diffRun used to compute the streaks
           'uri=s'          => \$uri,        # uri of the input image
           'path_base=s'    => \$path_base,  # path_base of the input
           'cam_path_base=s'=> \$cam_path_base,  # path_base from camera stage (for chip and raw)
           'cam_reduction=s'=> \$cam_reduction,  # reduction class from camera stage (for chip and raw)
           'outroot=s'      => \$outroot,     # "directory" for temporary images (may be nebulous)
           'recoveryroot=s' => \$recoveryroot,# "prefix" for saving the images of excised pixels
           'replace=s'      => \$replace,    # replace the input images with the results.
           'magicked=s'     => \$magicked,   # magicked state of the run
           'release'        => \$release,    # NAN masked pixels for release
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'no-op'          => \$no_op,      # Don't do any operations?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --magic_ds_id --camera --run_state --streaks --stage --stage_id --component --uri --path_base --outroot --magicked",
           -exitval => 3) unless
    defined $magic_ds_id and
    defined $camera and
    defined $run_state and
    defined $streaks and
    defined $streaks_path_base and
    defined $stage and
    defined $stage_id and
    defined $exp_id and
    defined $component and
    defined $uri and
    defined $path_base and
    defined $outroot and
    defined $magicked;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

if ($logfile) {
    $ipprc->redirect_to_logfile($logfile) or my_die( "Unable to redirect output", $magic_ds_id, $component, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Continuing script $0 on $host at $date\n\n";
}

$cam_reduction = 'DEFAULT' if !$cam_reduction or ($cam_reduction eq 'NULL');

my $recipe_psastro = $ipprc->reduction($cam_reduction, 'PSASTRO'); # Recipe to use
&my_die("Unrecognised PSASTRO recipe", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_psastro;

my ($skycell_args, $class_id, $skycell_id);

if (($stage eq "raw") or ($stage eq "chip") or $stage eq "chip_bg") {
    $class_id = $component;
    $skycell_args = " -class_id";
} elsif ($stage eq "warp" or $stage eq "warp_bg") {
    $skycell_id = $component;
    $skycell_args = " -skycell_id";
} elsif ($stage eq "diff") {
    $skycell_id = $component;
} elsif ($stage ne "camera") {
    &my_die("Invalid value for stage: $stage", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
}

&my_die("Invalid value for run-state: $run_state", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR)
    unless ($run_state eq 'new') or ($run_state eq 'update');

$inv_streaks_path_base = undef if defined($inv_streaks_path_base) and ($inv_streaks_path_base eq "NULL");
$inv_streaks = undef if defined($inv_streaks) and ($inv_streaks eq "NULL");

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $dirname = dirname($path_base);
my $basename = basename($path_base);
my $nebulousInput = inNebulous($dirname);
my $nebulousOutput = inNebulous($outroot);

# parse replace arguments check for errors and set up
# the appropriate paths
my $nebulous;
if (defined($replace) and ($replace eq "T")) {
    # for camera stage we need a handle to the nebulous server
    if ($stage eq 'camera') {
        my $nebulousServer = metadataLookupStr( $ipprc->{_siteConfig}, 'NEB_SERVER' );
        &my_die("cannot find NEB_SERVER in site configuration",
                                        $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR)
            if !$ nebulousServer;

        $nebulous = eval { Nebulous::Client->new( proxy => $nebulousServer ); };
        if ($@ or not defined $nebulous) {
            &my_die ("Unable to create a Nebulous::Client object with proxy $nebulousServer",
                                    $magic_ds_id, $component , $PS_EXIT_UNKNOWN_ERROR);
        }
    }
    $replace = 1;
} else {
    $replace = 0;
}


# We don't use recoveryroot for camera stage
if ($stage eq 'camera') {
    $recoveryroot = undef;
}

# default value is "NULL" do not use
if (defined($recoveryroot) and ($recoveryroot eq "NULL")) {
    $recoveryroot = undef;
}

if (($stage eq "raw") and $replace and ! $recoveryroot) {
    &my_die("Can not replace raw files without defining recoveryroot.",
        $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
}

# create the output directories if it is not a nebulous path and it doesn't exist
if (! $nebulousOutput) {
    if (! -e $outroot ) {
        my $code = system "mkdir -p $outroot";
        &my_die("cannot create output directory $outroot", $magic_ds_id, $component,
                $code >> 8) if $code;
    }
}

# XXX: create a DESTREAK file rule. For now just steal one that will work
my $error;
my $statsFile = $ipprc->prepare_output("PSASTRO.STATS", "$outroot/$exp_id.mds.$magic_ds_id.$stage_id.$component", undef, 1, \$error) or &my_die("failed to prepare output for stats", $magic_ds_id, $component, $error);

my $backup_path_base;
my $tmproot;
if ($replace) {
    # in replace mode, we place the output files in the same "directory" as the inputs
    # Nebulous requires this for the two inputs to nebSwap which we use
    # We prepend the path with SR_ This causes the filenames for instances of the swapped files to
    # have SR in them.
    $tmproot = $dirname . "/SR_";
    $backup_path_base = $tmproot . "$basename";
} else {
    # note: trailing / is necessary
    $tmproot = "$outroot/";
    $backup_path_base = $tmproot . $basename;
}

my $recovery_path_base;
if ($recoveryroot) {
    # recoveryroot is a path to prepend to the basenames of the input files
    if (inNebulous($recoveryroot)) {
        # if recoveryroot is a nebulous path we ignore the actual path and put the files in the
        # the same "directory" as the input files
        $recoveryroot = "$dirname/REC_";
        $recovery_path_base = $recoveryroot . $basename;
    } else {
        # otherwise we put the files in recoveryroot.
        # Regardless, we prefix the basename with 'REC_'
        $ipprc->outroot_prepare($recoveryroot);
        $recovery_path_base = "$recoveryroot/REC_$basename";
    }
}

my $temp_dir;

if ($stage ne "camera") {
    # get skycell list if needed
    my ($sfh, $skycell_list);
    if ($skycell_args) {
        my $command = "$magicdstool -magic_ds_id $magic_ds_id -getskycells";
        $command .= " $skycell_args";
        $command .= " -dbname $dbname" if $dbname;
        my @diff_components;
        if ($mismatched_tess) {
            # tessellation for this skycell does not match the tessellation used for the magic analysis
            # used to calculate the streaks file
            @diff_components = get_overlaps($path_base, $component, $diff_tess_id);
        } else {
            @diff_components = ($component);
        }

        # hash of uris by skycell.
        my %diff_skycells;
        foreach my $component (@diff_components) {
            my $this_command = "$command $component";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $this_command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform magicdstool -diffskyfile $skycell_args: $error_code", $magic_ds_id, $component, $error_code);
            }

            my $getskycells_output = join "", @$stdout_buf;
            if ($getskycells_output) {
                my $metadata = $mdcParser->parse($getskycells_output) or
                    &my_die("Unable to parse metadata config doc", $magic_ds_id, $component, $PS_EXIT_PROG_ERROR);

                my $skycells = parse_md_list($metadata) or
                        &my_die("Unable to parse metadata list", $magic_ds_id, $component, $PS_EXIT_PROG_ERROR);

                foreach my $skycell (@$skycells) {
                    my $skycell_uri;
                    $skycell->{quality} = undef;
		    # XXXXXXXX remake all skycells to avoid dead instances (ipp064)
                    if (0 and $skycell->{data_state} eq "full" and $skycell->{quality} == 0) {
                        $skycell_uri = $ipprc->filename("PPSUB.OUTPUT", $skycell->{path_base});
			my $skycell_id = $skycell->{skycell_id};
                        $diff_skycells{$skycell_id} = $skycell_uri;
                    } else {
                        # diff run must have been cleaned up, need to create this skycell file on the fly
                        if (!defined $temp_dir ) {
                            $temp_dir = tempdir( CLEANUP => !$save_temps);
                        }
                        my $skycell_id = $skycell->{skycell_id};
                        if (!$diff_skycells{$skycell_id}) {
                            $skycell_uri = "$temp_dir/$skycell_id";
                            $ipprc->skycell_file($skycell->{tess_id}, $skycell_id, $skycell_uri, $verbose) or
                                &my_die("failed to create skycell file for $skycell_id", $magic_ds_id, $component, $PS_EXIT_PROG_ERROR);
                        }
                        $diff_skycells{$skycell_id} = $skycell_uri;
                    }
                }
            }
        }
        # write the skycell list file
        ($sfh, $skycell_list) = tempfile( "/tmp/skycell_list.XXXX", UNLINK => !$save_temps);
        foreach my $skycell_id (keys %diff_skycells) {
            print $sfh $diff_skycells{$skycell_id} . "\n";
        }
        close $sfh
    }

    my ($image, $mask, $ch_mask, $weight, $astrom, $sources);

    # if we're destreaking a bothways diff need to combine the
    # two streaks files
    my ($allstreaks_fh, $allstreaks_name);

    # Set name of streaks files from their path_base. Note. ne 'NULL' test is for backward compatability
    # with runs that don't have path_base set yet
    if ($streaks_path_base ne 'NULL') {
        $streaks = "$streaks_path_base.streaks";
    }
    if ($inv_streaks_path_base and ($inv_streaks_path_base ne 'NULL')) {
        $inv_streaks = "$inv_streaks_path_base.streaks";
    }

    my $streaks_resolved = $ipprc->file_resolve($streaks) or &my_die("failed to resolve streaks file $streaks", $magic_ds_id, $component, $PS_EXIT_SYS_ERROR);
    my $inv_streaks_resolved;
    if ($inv_streaks) {
        $inv_streaks_resolved = $ipprc->file_resolve($inv_streaks) or &my_die("failed to resolve inverse streaks file $inv_streaks", $magic_ds_id, $component, $PS_EXIT_SYS_ERROR);
    }
        

    if ($stage eq "raw") {
        $image = $uri;
        $astrom = $ipprc->filename("PSASTRO.OUTPUT", $cam_path_base);
        $mask   = $ipprc->filename("PSASTRO.OUTPUT.MASK", $cam_path_base, $class_id) if $release ;
    } elsif ($stage eq "chip") {

        # Check to see if we're using dynamic masks
        my $dynamicMasks;               # Use dynamic masks?
        {
            # Get the PSASTRO recipe
            my $command = "$ppConfigDump -camera $camera -recipe PSASTRO $recipe_psastro -dump-recipe PSASTRO -";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    # note verbose == 0 to avoid polluting log files with almost always useless information
                run(command => $command, verbose => 0);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform ppConfigDump: $error_code", $magic_ds_id, $component,
                        $PS_EXIT_CONFIG_ERROR);
            }
            my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
                &my_die("Unable to parse metadata config doc", $magic_ds_id, $component,
                        $PS_EXIT_CONFIG_ERROR);

            $dynamicMasks = metadataLookupBool($recipeData, 'REFSTAR_MASK');
        }

        # we use the mask output from the camera stage for input and replace
        # the output of the chip stage with that mask as well.
        $image  = $ipprc->filename("PPIMAGE.CHIP", $path_base, $class_id);
        $weight = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $path_base, $class_id);
        $astrom = $ipprc->filename("PSASTRO.OUTPUT", $cam_path_base);

        # if magicked is non-zero we are updating a previously magicked component. In this case we don't
        # touch the camera mask
        if (!$magicked and $dynamicMasks) {
            $mask = $ipprc->filename("PSASTRO.OUTPUT.MASK", $cam_path_base, $class_id);
            $ch_mask = $ipprc->filename("PPIMAGE.CHIP.MASK", $path_base, $class_id);
        } else {
            $mask = $ipprc->filename("PPIMAGE.CHIP.MASK", $path_base, $class_id);
        }

        # only destreak cmf file if this is a new run
        if ($run_state eq 'new') {
            $sources = $ipprc->filename("PSPHOT.OUTPUT",  $path_base, $class_id);
        }

    } elsif ($stage eq "chip_bg") {
        $image  = $ipprc->filename("PPBACKGROUND.OUTPUT", $path_base, $class_id);
        $mask = $ipprc->filename("PPBACKGROUND.OUTPUT.MASK", $path_base, $class_id);
        $astrom = $ipprc->filename("PSASTRO.OUTPUT", $cam_path_base);
    } elsif ($stage eq "warp" or $stage eq "warp_bg") {
        $image  = $ipprc->filename("PSWARP.OUTPUT", $path_base);
        $mask   = $ipprc->filename("PSWARP.OUTPUT.MASK", $path_base);
        $weight = $ipprc->filename("PSWARP.OUTPUT.VARIANCE", $path_base);
        $sources    = $ipprc->filename("PSWARP.OUTPUT.SOURCES", $path_base);
    } elsif ($stage eq "diff") {
        $image  = $ipprc->filename("PPSUB.OUTPUT", $path_base);
        $mask   = $ipprc->filename("PPSUB.OUTPUT.MASK", $path_base);
        $weight = $ipprc->filename("PPSUB.OUTPUT.VARIANCE", $path_base);
        $sources    = $ipprc->filename("PPSUB.OUTPUT.SOURCES", $path_base);

        if ($inv_streaks_resolved) {
            # create a temporary file containing the contents of the
            # two streaks files
            ($allstreaks_fh, $allstreaks_name) = tempfile ("/tmp/all.streaks.XXXX",
                    UNLINK => !$save_temps);

            combine_streaks($allstreaks_fh, $streaks_resolved, $inv_streaks_resolved);

            # apply the combined streaks to both the forward and inverse diffs
            $streaks_resolved = $allstreaks_name;
        }
    }

    {
        my $command = "$streaksremove -stage $stage -tmproot $tmproot -streaks $streaks_resolved -image $image";

        $command .= " -stats $statsFile";
        $command .= " -class_id $class_id" if defined $class_id;
        $command .= " -recovery $recoveryroot" if defined $recoveryroot;
        $command .= " -astrom $astrom" if defined $astrom;
        $command .= " -mask $mask" if defined $mask;
        $command .= " -chip_mask $ch_mask" if defined $ch_mask;
        $command .= " -weight $weight" if defined $weight;
        $command .= " -sources $sources" if defined $sources;
        $command .= " -skycelllist $skycell_list" if defined $skycell_list;
        $command .= " -replace" if $replace;
        $command .= " -release" if $release;
        $command .= " -dbname $dbname" if defined $dbname;
        unless (defined $no_op) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform streaksremove: $error_code", $magic_ds_id, $component, $error_code);
            }
        } else {
            print "skipping command $command\n";
        }
    }
    if (($stage eq "diff") and $inv_streaks_resolved) {
        $image   = $ipprc->filename("PPSUB.INVERSE", $path_base);
        $mask    = $ipprc->filename("PPSUB.INVERSE.MASK", $path_base);
        $weight  = $ipprc->filename("PPSUB.INVERSE.VARIANCE", $path_base);
        $sources = $ipprc->filename("PPSUB.INVERSE.SOURCES", $path_base);

        # Note: we create a stats file for the inverse procesing but we don't look at the results
        my $invStatsFile = "$outroot/$exp_id.mds.$magic_ds_id.$stage_id.$component.inv.stats";

        my $command = "$streaksremove -stage $stage -tmproot $tmproot -streaks $streaks_resolved -image $image";
        $command .= " -stats $invStatsFile";

        $command .= " -recovery $recoveryroot" if defined $recoveryroot;
        $command .= " -mask $mask" if defined $mask;
        $command .= " -weight $weight" if defined $weight;
        $command .= " -sources $sources" if defined $sources;
        $command .= " -replace" if $replace;
        $command .= " -release" if $release;
        $command .= " -dbname $dbname" if defined $dbname;
        unless (defined $no_op) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform streaksremove: $error_code", $magic_ds_id, $component, $error_code);
            }
        } else {
            print "skipping command $command\n";
        }
    }
} else {
    # camera stage. The only work to do is to censor the detections file

    my $tempOutRoot = "/tmp/destreak";
    my ($maskListFile,   $maskListName)   = tempfile( "$tempOutRoot.mask.list.XXXX",   UNLINK => !$save_temps);
    my $files;
    {
        my $cam_id = $stage_id;
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
    }
    foreach my $file (@$files) {
        my $class_id = $file->{class_id};
        my $quality = $file->{quality};
        if (!$quality) {
            my $mask = $ipprc->filename("PSASTRO.OUTPUT.MASK", $path_base, $class_id);
            if (! $ipprc->file_exists($mask)) {
                # camera mask doesn't exist for this chip. Fall back to the chip mask
                $mask= $ipprc->filename("PPIMAGE.CHIP.MASK", $file->{path_base}, $class_id);
            }
            print $maskListFile "$mask\n";
        }
    }
    close $maskListFile;

    my $astrom = $ipprc->filename("PSASTRO.OUTPUT", $path_base);
    {
        my $command = "$censorObjects -file $astrom -masklist $maskListName $backup_path_base";
        # $command .= " -replace" if $replace;
        $command .= " -dbname $dbname" if defined $dbname;
        unless (defined $no_op) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform censorObjects: $error_code", $magic_ds_id, $component, $error_code);
            }
            my $output = $ipprc->filename("CENSOR.OUTPUT", $backup_path_base);
            &my_die("expected output file $output not found ", $magic_ds_id, $component, $PS_EXIT_DATA_ERROR)
                unless $ipprc->file_exists($output);

            if ($replace) {
                $nebulous->swap($astrom, $output) or
                    &my_die("nebulous swap failed $astrom $output", $magic_ds_id, $component, $PS_EXIT_UNKNOWN_ERROR);
            }
        } else {
            print "skipping command $command\n";
        }
    }
}

my $statsFlags;
if (!$no_op and $stage ne "camera") {
    file_check($statsFile);
    {
        my $resolvedStatsFile = $ipprc->file_resolve($statsFile);
        my $command = "$ppStatsFromMetadata $resolvedStatsFile - STREAKSREMOVE";

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppStatsFromMetadata: $error_code", $magic_ds_id, $component, $error_code);
        }
        foreach my $line (@$stdout_buf) {
            $statsFlags .= " $line";
        }
        chomp $statsFlags;
    }
}

# XXX: if recovery and/or backup files were expected make sure they exist

# Input result into database
{
    my $command = "$magicdstool";
    $command   .= " -magic_ds_id $magic_ds_id";
    $command   .= " -component $component";
    $command   .= " -setmagicked" if $replace;
    if ($run_state eq 'new') {
        $command .= " -adddestreakedfile";
        $command .= " -backup_path_base $backup_path_base" if $backup_path_base;
        $command .= " -recovery_path_base $recovery_path_base" if $recovery_path_base;
        $command .= " $statsFlags" if $statsFlags;
    } else {
        $command .= " -tofullfile";
    }
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform magicdstool -addresult: $error_code", $magic_ds_id, $component,
                $error_code);
        }
    } else {
        print "Skipping command: $command\n";
    }
}



### Pau.


sub file_check
{
    my $file = shift;           # Name of file
    &my_die("Unable to find output file: $file", $magic_ds_id, $component, $PS_EXIT_SYS_ERROR) unless
        $ipprc->file_exists($file);
}

sub inNebulous
{
    my $path = shift;

    my $scheme = file_scheme($path);

    return $scheme and ($scheme eq "neb");
}

sub combine_streaks
{
    my $fout = shift;
    my $fn1 = shift;
    my $fn2 = shift;

    my ($n1, @streaks1) = read_streaks_file($fn1);
    my_die("failed to read streaks from $fn1", $magic_ds_id, $component,
        $PS_EXIT_UNKNOWN_ERROR) if $n1 < 0;

    my ($n2, @streaks2) = read_streaks_file($fn2);
    my_die("failed to read streaks from $fn2", $magic_ds_id, $component,
        $PS_EXIT_UNKNOWN_ERROR) if $n2 < 0;

    print $fout $n1 + $n2 . "\n";

    foreach my $line (@streaks1, @streaks2) {
        print $fout $line;
    }

    close $fout
        or my_die("failed to close combined streaks file", $magic_ds_id,
                     $component, $PS_EXIT_UNKNOWN_ERROR);
}

sub read_streaks_file
{
    my $filename = shift;
    my $fh;
    open $fh, "<$filename" or my_die("failed to open $filename",
                    $magic_ds_id, $component, $PS_EXIT_UNKNOWN_ERROR);

    # first line is the number of streaks
    my $line = <$fh>;
    chomp $line;
    my $nstreaks = $line;

    my @streaks;

    foreach $line (<$fh>) {
        push @streaks, $line;
    }

    close $fh;

    return ($nstreaks, @streaks);
}

            # @diff_components = get_overlaps($path_base, $component, $tess_id, $diff_tess_id);
sub get_overlaps {
    my $path_base = shift;
    my $component = shift;
    my $diff_tess_id = shift;

    my $tess_dir = $ipprc->tessellation_catdir( $diff_tess_id ); 
    my $catdir = $ipprc->convert_filename_absolute($tess_dir);

    my $cmf = $ipprc->file_resolve("$path_base.cmf");
    &my_die("Unable to resolve $path_base.$cmf", $magic_ds_id, $component, $PS_EXIT_SYS_ERROR) unless $cmf;

    my $command = "$dvoImageOverlaps -D CATDIR $catdir -accept-astrom $cmf";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dvoImageOverlaps $error_code", $magic_ds_id, $component,
            $error_code);
    }
    my @list;
    my $output = join "\n", @$stdout_buf;
    if ($output) {
        foreach my $line (split "\n", $output) {
            my (undef, undef, $skycell_id) = split " ", $line;
            push @list, $skycell_id;
        }
    }
    return @list
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $magic_ds_id = shift;    # Magic DS identifier
    my $component = shift;      # class_id or skycell_id
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    my $command = "$magicdstool";
    
    if ($run_state eq 'new') {
        $command .= " -adddestreakedfile";
    } else {
        $command .= " -updatedestreakedfile";
    }
    $command   .= " -magic_ds_id $magic_ds_id";
    $command   .= " -component $component";
    $command .= " -backup_path_base $backup_path_base" if $backup_path_base;
    $command .= " -recovery_path_base $recovery_path_base" if $recovery_path_base;
    $command   .= " -fault $exit_code";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            carp("failed to update database for $magic_ds_id $component");
        }
    } else {
        print "Skipping command: $command\n";
    }

    carp($msg);
    exit $exit_code;
}

__END__
