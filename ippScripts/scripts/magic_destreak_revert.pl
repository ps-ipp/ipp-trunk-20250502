#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print STDERR "\n\n";
print STDERR "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
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
my $isdestreaked = can_run('isdestreaked') or (warn "Can't find isdestreaked" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($magic_ds_id, $camera, $streaks, $stage, $stage_id, $component, $uri, $path_base, $bothways, $cam_path_base, $cam_reduction, $magicked, $run_state);
my ($outroot, $recovery_path_base, $replace, $release, $bytes, $md5sum);
my ($dbname, $save_temps, $verbose, $no_update, $no_op, $logfile);

GetOptions(
           'magic_ds_id=s'  => \$magic_ds_id,# Magic destreak run identifier
           'camera=s'       => \$camera,     # camera for evaluating file rules
           'stage=s'        => \$stage,      # raw, chip, warp, or diff
           'stage_id=s'     => \$stage_id,   # exp_id, chip_id, warp_id, or diff_id
           'run-state=s'    => \$run_state, # current state of run
           'component=s'    => \$component,  # the class_id or skycell_id
           'path_base=s'    => \$path_base,  # path_base of the input
           'cam_path_base=s'=> \$cam_path_base,  # path_base of the associated camera run
           'cam_reduction=s'=> \$cam_reduction,  # reduction class of the associated camera run
           'outroot=s'      => \$outroot,     # "directory" for temporary images (may be nebulous)
           'recovery_path_base=s' => \$recovery_path_base,# "directory" for saving the images of excised pixels
           'replace=s'      => \$replace,    # replace the input images with the results.
           'bothways=s'     => \$bothways,   # run has inverse files (bothways diff)
           'magicked=s'     => \$magicked,   # magicked state of the run
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'no-op'          => \$no_op,      # Don't do any operations?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --magic_ds_id --camera --stage --stage_id --component --path_base --outroot --magicked",
           -exitval => 3) unless
    defined $magic_ds_id and
    defined $camera and
    defined $stage and
    defined $stage_id and
    defined $component and
    defined $path_base and
    defined $magicked and
    defined $run_state and
    defined $outroot;

if (defined($replace) and ($replace eq "T")) {
    $replace = 1;
} else {
    $replace = 0;
}

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

$ipprc->redirect_to_logfile($logfile) or my_die( "Unable to redirect output", $magic_ds_id, $component, $PS_EXIT_SYS_ERROR ) if $logfile;

&my_die("bytes and md5sum are is required for raw stage stage", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR) if ($replace and $stage eq 'raw' and (!$bytes or !$md5sum));

&my_die("cam_path_base is required for chip stage", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR) if ($stage eq 'chip' and !$cam_path_base);




$cam_reduction = 'DEFAULT' if !defined $cam_reduction or ($cam_reduction eq "NULL");
my $recipe_psastro = $ipprc->reduction($cam_reduction, 'PSASTRO'); # Recipe to use
&my_die("Unrecognised PSASTRO recipe", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_psastro;

my $nebulousServer = metadataLookupStr( $ipprc->{_siteConfig}, 'NEB_SERVER' );
&my_die("cannot find NEB_SERVER in site configuration", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR) if !$nebulousServer;

my $nebulous = eval { Nebulous::Client->new( proxy => $nebulousServer ); };
if ($@ or not defined $nebulous) {
    &my_die ("Unable to create a Nebulous::Client object with proxy $nebulousServer", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
}


my $class_id;
my $skycell_id;
if (($stage eq "raw") or ($stage eq "chip")) {
    $class_id = $component;
} elsif ($stage eq "warp") {
    $skycell_id = $component;
} elsif ($stage eq "diff") {
    $skycell_id = $component;
} elsif ($stage ne "camera") {
    &my_die("Invalid value for stage: $stage", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $dirname  = dirname($path_base);
my $basename = basename($path_base);
my $nebulousInput = inNebulous($dirname);

&my_die("replace not allowed for non-nebulous files", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR)
    if ($replace eq "T") and !$nebulousInput;


# default value is "NULL" do not use it
if (defined($recovery_path_base) and ($recovery_path_base eq "NULL")) {
    $recovery_path_base = undef;
}

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
    # note: the trailing / here is necessary
    $tmproot = "$outroot/";
    $backup_path_base = $tmproot . $basename;
}


my ($image, $mask, $ch_mask, $weight, $sources, $astrom);
my ($bimage, $bmask, $bch_mask, $bweight, $bsources, $bastrom);
my ($rimage, $rmask, $rch_mask, $rweight);

if ($stage eq "raw") {
    if ($path_base =~ /.*\.fits$/) {
        # removal of uri from rawImfile hasn't happened yet
        $image = $path_base;
    } else {
        # XXX: should have a file rule
        $image = $path_base . ".fits";
    }
    if ($backup_path_base =~ /.*\.fits$/) {
        $bimage = $backup_path_base;
    } else {
        $bimage = $backup_path_base . ".fits";
    }
} elsif ($stage eq "chip") {
    # Check to see if we're using dynamic masks
    my $dynamicMasks;               # Use dynamic masks?
    {
        # Get the PSASTRO recipe
        my $command = "$ppConfigDump -camera $camera -recipe PSASTRO $recipe_psastro -dump-recipe PSASTRO -";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            # note verbose == 0 to keep from polluting log file with lots of usually useless information
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

    $image  = $ipprc->filename("PPIMAGE.CHIP", $path_base, $class_id);
    $weight = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $path_base, $class_id);

    # we use the mask output from the camera stage for input and replace
    # the output of the chip stage with that mask as well.
    # Note that when destreaking as part of an update $magicked is non-zero.
    # In this case we do not touch the camera stage mask so there is no need to revert it
    if ($dynamicMasks and (!$magicked or ($run_state eq 'goto_restored'))) {
        $mask = $ipprc->filename("PSASTRO.OUTPUT.MASK", $cam_path_base, $class_id);
        $ch_mask = $ipprc->filename("PPIMAGE.CHIP.MASK", $path_base, $class_id);
    } else {
        $mask = $ipprc->filename("PPIMAGE.CHIP.MASK", $path_base, $class_id);
    }

    # only revert sources if this is a new run, not one being updated
    if (($run_state eq 'new') or ($run_state eq 'goto_restored')) {
        $sources = $ipprc->filename("PSPHOT.OUTPUT", $path_base, $class_id);
        $bsources = $ipprc->filename("PSPHOT.OUTPUT", $backup_path_base, $class_id);
    }

    $bimage  = $ipprc->filename("PPIMAGE.CHIP", $backup_path_base, $class_id);
    # This is somewhat kludgey but it works whether the mask is camera mask or chip mask
    $bmask   = dirname($backup_path_base) . "/SR_" . basename($mask);
    $bch_mask= $ipprc->filename("PPIMAGE.CHIP.MASK", $backup_path_base, $class_id);
    $bweight = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $backup_path_base, $class_id);

    if ($recovery_path_base) {
        $rimage  = $ipprc->filename("PPIMAGE.CHIP", $recovery_path_base, $class_id);
        # This is somewhat kludgey but it works whether the mask is camera mask or chip mask
        $rmask   = dirname($recovery_path_base) . "/REC_" . basename($mask);
        $rch_mask= $ipprc->filename("PPIMAGE.CHIP.MASK", $recovery_path_base, $class_id);
        $rweight = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $recovery_path_base, $class_id);
    }
} elsif ($stage eq "camera") {
    $astrom =  $ipprc->filename("PSASTRO.OUTPUT", $path_base);
    $bastrom = $ipprc->filename("PSASTRO.OUTPUT", $backup_path_base);
} elsif ($stage eq "warp") {
    $image  = $ipprc->filename("PSWARP.OUTPUT", $path_base);
    $mask   = $ipprc->filename("PSWARP.OUTPUT.MASK", $path_base);
    $weight = $ipprc->filename("PSWARP.OUTPUT.VARIANCE", $path_base);
    $sources = $ipprc->filename("PSWARP.OUTPUT.SOURCES", $path_base);
    $bimage  = $ipprc->filename("PSWARP.OUTPUT", $backup_path_base);
    $bmask   = $ipprc->filename("PSWARP.OUTPUT.MASK", $backup_path_base);
    $bweight = $ipprc->filename("PSWARP.OUTPUT.VARIANCE", $backup_path_base);
    $bsources = $ipprc->filename("PSWARP.OUTPUT.SOURCES", $backup_path_base);
    if ($recovery_path_base) {
        $rimage  = $ipprc->filename("PSWARP.OUTPUT", $recovery_path_base);
        $rmask   = $ipprc->filename("PSWARP.OUTPUT.MASK", $recovery_path_base);
        $rweight = $ipprc->filename("PSWARP.OUTPUT.VARIANCE", $recovery_path_base);
    }
} elsif ($stage eq "diff") {
    my $name = "PPSUB.OUTPUT";
    $image  = $ipprc->filename($name, $path_base);
    $mask   = $ipprc->filename("$name.MASK", $path_base);
    $weight = $ipprc->filename("$name.VARIANCE", $path_base);
    $sources = $ipprc->filename("$name.SOURCES", $path_base);
    $bimage  = $ipprc->filename($name, $backup_path_base);
    $bmask   = $ipprc->filename("$name.MASK", $backup_path_base);
    $bweight = $ipprc->filename("$name.VARIANCE", $backup_path_base);
    $bsources = $ipprc->filename("$name.SOURCES", $backup_path_base);
    if ($recovery_path_base) {
        $rimage  = $ipprc->filename($name, $recovery_path_base);
        $rmask   = $ipprc->filename("$name.MASK", $recovery_path_base);
        $rweight = $ipprc->filename("$name.VARIANCE", $recovery_path_base);
    }
}

revert_files($replace, $image, $mask, $weight, $sources, $astrom, $bimage, $bmask, $bweight, $bsources, $bastrom);
if ($recovery_path_base) {
    delete_recovery_files($rimage, $rmask, $rweight, $rch_mask);
}

if ($stage eq "diff" and $bothways) {
    my $name = "PPSUB.INVERSE";
    $image  = $ipprc->filename($name, $path_base);
    $mask   = $ipprc->filename("$name.MASK", $path_base);
    $weight = $ipprc->filename("$name.VARIANCE", $path_base);
    $sources = $ipprc->filename("$name.SOURCES", $path_base);
    $bimage  = $ipprc->filename($name, $backup_path_base);
    $bmask   = $ipprc->filename("$name.MASK", $backup_path_base);
    $bweight = $ipprc->filename("$name.VARIANCE", $backup_path_base);
    $bsources = $ipprc->filename("$name.SOURCES", $backup_path_base);
    revert_files($replace, $image, $mask, $weight, $sources, undef, $bimage, $bmask, $bweight, $bsources, undef);
    if ($recovery_path_base) {
        $rimage  = $ipprc->filename($name, $recovery_path_base);
        $rmask   = $ipprc->filename("$name.MASK", $recovery_path_base);
        $rweight = $ipprc->filename("$name.VARIANCE", $recovery_path_base);
        delete_recovery_files($rimage, $rmask, $rweight);
    }
}

# now revert the row in the database
{
    my $command = "$magicdstool -revertdestreakedfile -i_am_sure";
    $command   .= " -state $run_state";
    $command   .= " -magic_ds_id $magic_ds_id";
    $command   .= " -component $component";
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

sub revert_files {
    my $replace = shift;
#    return if !$replace;

    my $image = shift;
    my $mask = shift;
    my $weight = shift;
    my $sources = shift;
    my $astrom = shift;
    my $bimage = shift;
    my $bmask = shift;
    my $bweight = shift;
    my $bsources = shift;
    my $bastrom = shift;

    if ($image) {
        revert_file($replace, $image, $bimage) or
            &my_die("failed to restore image file", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
    }

    if ($mask) {
        if (!revert_file($replace, $mask, $bmask)) {
            &my_die("failed to restore mask file", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
        }
    }

    if ($ch_mask) {
        if (!revert_file($replace, $ch_mask, $bch_mask)) {
            &my_die("failed to restore chip mask file", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
        }
    }


    if ($weight) {
        revert_file($replace, $weight, $bweight) or
            &my_die("failed to restore variance image", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
    }

    if ($sources) {
        revert_file($replace, $sources, $bsources) or
            &my_die("failed to restore sources file", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
    }

    if ($astrom) {
        revert_file($replace, $astrom, $bastrom) or
            &my_die("failed to restore astrometry file", $magic_ds_id, $component, $PS_EXIT_CONFIG_ERROR);
    }
}

sub check_keyword
{
    my $filename = shift;
    my $command = "$isdestreaked $filename";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    if (!defined $error_code) {
        print STDERR "run( $command ) returned undef error_code!!!!!\n";
        return $PS_EXIT_UNKNOWN_ERROR;
    }
    return $error_code >> 8;
}

sub revert_file
{
    my $replace = shift;
    my $original = shift;
    my $backup   = shift;
    my $force = 1; # force deletion of backup files in nebulous

    if (!$replace) {
        # we don't need to do all of this checking unless the destreak run is
        # replace mode just delete the backup 
        my $error_code = $ipprc->kill_file($backup);
        if ($error_code) {
            print STDERR "failed to kill $backup: error_code $error_code\n";
            return 0;
        } else {
            return 1;
        }
    }

    my $o_path = $ipprc->file_resolve($original);
    my $original_result;
    if ($o_path) {
        # invoke the program isdestreaked to check for the destreaked keyword
        $original_result = check_keyword($o_path);
    } else {
        print STDERR "no instances found for $original\n";
        $original_result = $PS_EXIT_DATA_ERROR;
    }
    my $original_is_destreaked = $original_result eq 0;
    my $original_is_not_destreaked = $original_result eq 42;

    my $b_path = $ipprc->file_resolve($backup);
    my $backup_result;
    if ($b_path) {
        $backup_result = check_keyword($b_path);
    } else {
        print STDERR "no instances found for $backup\n";
        $backup_result = $PS_EXIT_DATA_ERROR;
    }
    my $backup_is_destreaked = $backup_result eq 0;
    my $backup_is_not_destreaked = $backup_result eq 42;

    if ($backup_is_not_destreaked) {
        if ($original_is_not_destreaked) {
            print STDERR "\nBoth files appear to not be destreaked.\n";
            print STDERR "original: $original\n";
            print STDERR "backup:   $backup\n";
            my $o_basename = basename($o_path);
            my $b_basename = basename($b_path);
            # if original file does not have SR_ in basename and the backup does then the 
            # files are in their original state. Just delete back up file
            # otherwise go to failed_revert for manual inspection.
	    # XXX: We probably can get by with just checking the basename of the original file
            if (!($o_basename =~ /SR_/) ) {
            # if (!($o_basename =~ /SR_/) and ($b_basename =~ /SR_/)) {
                print STDERR " basenames are as expected it is safe to delete backup file\n";
                if (! $ipprc->file_delete($backup, $force)) {
                    print STDERR "failed to delete $backup\n";
                    return 0;
                }
                return 1;
            } else {
                # this is an unexpected result. throw an error so this can be checked manually
                print STDERR " basenames for files are as NOT as expected unsafe to revert\n";
                print STDERR " o_basename: $o_basename\n";
                print STDERR " b_basename: $b_basename\n";
                return 0;
            }
        }
        # XXX TODO if stage is raw, check that backup has the correct size and md5sum

        if ($verbose) {
            print "ready to swap $backup\n";
            print "           to $original\n";
        }
        # Do we need to make this test? After the swap we're going to delete the file anyways

        if (! $nebulous->swap($backup, $original)) {
            print "failed to swap $backup\n";
            print "            to $original\n";
            return 0;
        }

        if ($b_path) {
            print "ready to delete backup\n" if $verbose;
            if (! $ipprc->file_delete($backup, $force)) {
                print "failed to delete $backup\n";
                return 0;
            }
        }

    } elsif ($original_is_not_destreaked) {
        print "original uri: $original is not a destreaked file no need to swap backup_result: $backup_result\n";
        # delete the 'backup' (destreaked target) file if it exists
        if ($b_path) {
            if (! $ipprc->file_delete($backup, $force)) {
                print "failed to delete $backup\n";
                return 0;
            }
        }
    } else {
        print STDERR "Error: neither file is an un de-streaked file.\n";
        print STDERR " backup key: $backup status: $backup_result\n";
        print STDERR " original key: $original status $original_result\n";
        return 0;
    }
    return 1;
}


sub delete_recovery_files
{
    foreach my $file (@_) {
        next if !$file;
	# don't care if this fails or not (it will fail if storage
	# object doesn't exist
        $ipprc->kill_file($file);
    }
}

sub inNebulous
{
    my $path = shift;

    my $scheme = file_scheme($path);

    return $scheme and ($scheme eq "neb");
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $magic_ds_id = shift;    # Magic DS identifier
    my $component = shift;      # class_id or skycell_id
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    # fault the whole run if one of the components fails to revert
    my $command = "$magicdstool -updaterun";
    $command   .= " -magic_ds_id $magic_ds_id";
    if ($run_state ne 'update') {
        $command .= " -set_state failed_revert";
    } else {
        $command .= " -set_state failed_revert_ud";
    }
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

    exit $exit_code;
}

__END__
