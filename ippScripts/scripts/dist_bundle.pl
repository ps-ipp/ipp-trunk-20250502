#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
# print "\n\n";
# print "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile tempdir );
use File::Basename qw( basename );
use Digest::MD5::File qw( file_md5_hex );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

# we can record the location an appropriate magic.mdc file for the file command 
my $file_magic = "$ENV{PSCONFDIR}/$ENV{PSCONFIG}/etc/compress.mgc";
### TEST print "magic: $file_magic\n";
### TEST system ("file -m $file_magic o60523g0045o.2132764.wrp.2688740.skycell.1566.004.pswarp.mdc");

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# filerules that are not included in 'cleaned' distribution bundles
# I could simplify the function below by using a two level hash but
# that would be less clear I think
my %chip_cleaned = ( 'PPIMAGE.CHIP' => 'image',
                     'PPIMAGE.CHIP.MASK' => 'mask',
                     'PPIMAGE.CHIP.VARIANCE' => 'variance' );
my %chip_bg_cleaned = ( 'PPBACKGROUND.OUTPUT' => 'image',
                        'PPBACKGROUND.OUTPUT.MASK' => 'mask' );
my %camera_cleaned = ( 'PSASTRO.OUTPUT.MASK' => 'mask' );
my %fake_cleaned;
my %warp_cleaned = ( 'PSWARP.OUTPUT' => 'image',
                     'PSWARP.OUTPUT.MASK' => 'mask',
                     'PSWARP.OUTPUT.VARIANCE' => 'variance' );
my %warp_bg_cleaned = ( 'PSWARP.OUTPUT' => 'image',
                        'PSWARP.OUTPUT.MASK' => 'mask' );
my %diff_cleaned = ( 'PPSUB.OUTPUT' => 'image',
                     'PPSUB.OUTPUT.MASK' => 'mask',
                     'PPSUB.OUTPUT.VARIANCE' => 'variance',
                     'PPSUB.INVERSE' => 'inv_image',
                     'PPSUB.INVERSE.MASK' => 'inv_mask',
                     'PPSUB.INVERSE.VARIANCE' => 'inv_variance' );

my %stack_cleaned = ( 'PPSTACK.UNCONV' => 'image',
                      'PPSTACK.UNCONV.MASK' => 'mask',
                      'PPSTACK.UNCONV.VARIANCE' => 'variance',
                      'PPSTACK.UNCONV.EXP' => 'exp',
                      'PPSTACK.UNCONV.EXPNUM' => 'expnum',
                      'PPSTACK.UNCONV.EXPWT' => 'expwt'
                      );

my %stack_convolved = ( 'PPSTACK.OUTPUT' => 'image',
                      'PPSTACK.OUTPUT.MASK' => 'mask',
                      'PPSTACK.OUTPUT.VARIANCE' => 'variance',
                      'PPSTACK.OUTPUT.EXP' => 'exp',
                      'PPSTACK.OUTPUT.EXPNUM' => 'expnum',
                      'PPSTACK.OUTPUT.EXPWT' => 'expwt'
                      );
my %empty_cleaned = ();

# Look for programs we need
my $missing_tools;
my $streaksrelease   = can_run('streaksrelease') or (warn "Can't find streaksrelease" and $missing_tools = 1);
my $bgtool   = can_run('bgtool') or (warn "Can't find bgtool" and $missing_tools = 1);
my $staticskytool   = can_run('staticskytool') or (warn "Can't find staticskytool" and $missing_tools = 1);
my $file_cmd   = can_run('file') or (warn "can't find program file" and $missing_tools = 1);
my $zcat   = can_run('zcat') or (warn "can't find program zcat" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($camera, $stage, $class_id, $stage_id, $component, $path_base, $chip_path_base, $alt_path_base, $clean, $exp_type);
my ($outroot, $run_state, $data_state, $magicked, $no_magic, $poor_quality, $results_file, $prefix);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);

GetOptions(
           'results_file=s' => \$results_file,     # ouptput file name for results
           'camera=s'       => \$camera,     # camera for evaluating file rules
           'stage=s'        => \$stage,      # raw, chip, warp, or diff
           'stage_id=s'     => \$stage_id,   # exp_id, chip_id, warp_id, or diff_id
           'component=s'    => \$component,  # the class_id or skycell_id
           'path_base=s'    => \$path_base,  # path_base of the input
           'chip_path_base=s'=> \$chip_path_base,  # path base for camera stage (to enable us to find the mask filefor raw images)
           'state=s'        => \$run_state,  # state of the run
           'data_state=s'   => \$data_state, # data_state for this component
           'poor_quality'   => \$poor_quality,  # the processing for this component did not produced images
           'no_magic'       => \$no_magic,   # magic is not required for this distribution run
           'magicked'       => \$magicked,   # magicked state for this component
           'alt_path_base=s'=> \$alt_path_base,  # path to alternate inputs
           'outroot=s'      => \$outroot,    # outroot
           'exp_type=s'     => \$exp_type,   # exp_type (only used for raw stage
           'prefix=s'       => \$prefix,     # "prefix" to apply to filenames
           'clean'          => \$clean,      # create clean distribution
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --camera --stage --stage_id --component --exp_type --path_base --outroot",
           -exitval => 3) unless
    defined $camera and
    defined $stage and
    defined $stage_id and
    defined $component and
    defined $path_base and
    defined $outroot;

$no_magic = 1;

if ($stage eq 'raw' and !$clean and !$no_magic) {
    
    # for raw stage need to have exposure type defined and if the type is OBJECT we need
    # a chip_path_base so we can find the chip mask file
    if (!defined $exp_type or ($exp_type eq 'OBJECT' and !defined $chip_path_base)) {
        pod2usage( -msg => "Required options: --chip_path_base --exp_type for raw stage", -exitval => 3);
    }
}

# Determine the value of a "good" burntool run.
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $config_cmd = "$ppConfigDump -camera $camera -get-key BURNTOOL.STATE.GOOD";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run ( command => $config_cmd, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform ppConfigDump: $error_code", 0, 0, $class_id, $PS_EXIT_SYS_ERROR);
}

my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
    &my_die("Unable to parse metadata config doc", 0, 0, $class_id, $PS_EXIT_SYS_ERROR);

my $burntoolStateGood = 999;
foreach my $cfg (@$recipeData) {
    if ($cfg->{name} eq 'BURNTOOL.STATE.GOOD') {
        $burntoolStateGood = $cfg->{value};
    }
}
if ($burntoolStateGood == 999) {
    &my_die("Failed to determine BURNTOOL.STATE.GOOD", $burntoolStateGood, $class_id, 0, $PS_EXIT_SYS_ERROR);
}


$ipprc->redirect_output($logfile) if $logfile;

$ipprc->define_camera($camera);

$ipprc->outroot_prepare($outroot);

my $num_sky_inputs;
if ($stage eq 'sky') {
    $num_sky_inputs = get_num_sky_inputs($stage_id);
}
# Get the list of data products for this component
# note: We my_die in get_file_list if something goes wrong.

my $file_list = get_file_list($stage, $component, $path_base, $clean, $num_sky_inputs,$burntoolStateGood);

if (($stage ne 'raw') and ($stage ne 'fake') and ($stage ne 'stack_summary') and !$poor_quality) {
    # If the file list is empty it is an error because we should at least get a config dump file
    # except for fake stage which doesn't do anything yet, stack_summary stage, and of course raw files have
    # no config dump because they aren't processed
    &my_die("empty file list", $component, $PS_EXIT_CONFIG_ERROR) if (!scalar @$file_list);
}

# set up directory for temporary files

my $temproot = metadataLookupStr($ipprc->{_siteConfig}, "TEMP.DIR");
$temproot = "/tmp" if !defined $temproot;
&my_die("directory for TEMP.DIR $temproot does not exist", $component, $PS_EXIT_CONFIG_ERROR) if ! -e $temproot;

my $tmpdir  = tempdir("$temproot/dist.XXXX", CLEANUP => !$save_temps);

#
# we need to run set masked pixels to NAN in the image and variance images
# unless
#   1. we are building a clean distribution bundle
#   2. magic is not required for this distRun
#   3. the processing for the component produced no images (warp or diff with bad quality for example)
my $nan_masked_pixels = ! ($clean || (($stage eq "camera") || ($stage eq 'fake') || ($stage eq 'stack') || ($stage eq 'sky')) || $no_magic || $poor_quality);

my ($image, $mask, $variance);
my ($inv_image, $inv_mask, $inv_variance);

my $num_files = 0;
foreach my $file (@$file_list) {
    # check whether this file rule refers to an image, mask, or variance fits image
    my $file_rule = $file->{file_rule};
    my $image_type = get_image_type($stage, $file_rule);

    # skip useless empty trace files
    next if ($file_rule =~ /TRACE/);

    # if this is an image and we are building a clean bundle or if quality is bad skip this rule
    next if $image_type and ($clean or $poor_quality);

    if ($stage eq 'stack') {
        # skip convolved stacks since they are deleted just after they are created now
        next if $stack_convolved{$file_rule};
    }

    # if magic is required, don't ship jpegs or binned fits images
    next if !$no_magic && (($file_rule =~ /.BIN1/) or ($file_rule =~ /.BIN2/) or ($file_rule =~ /.JPEG1/)
            or ($file_rule =~ /.JPEG2/));

    if ($stage eq "diff") {
        next if $file_rule =~ /CONV/;
	## exclude other products since not used any longer MEH 20190226 -- really should be a config option
        #next if $file_rule =~ /STATS/;
        #next if $file_rule =~ /LOG/;
        #next if $file_rule =~ /TRACE/;
        #next if $file_rule =~ /JPEG1/;
        #next if $file_rule =~ /JPEG2/;
        #next if $file_rule =~ /KERNELS/;
        #next if $file_rule =~ /BACKMDL/;
        #next if $file_rule =~ /PSF/;
        #next if $file->{name} =~ /mdc/;
        	
    }
    if ($stage eq "SSdiff") {
        next if $file_rule =~ /CONV/;
    }

    my $file_name = $file->{name};

    # look for file names that were processed remotely (LANL or UH Cray for example)
    # and change them to nebulous names
    if ( !($file_name =~ '^neb:') ) {
        # XXX: These two definitions should probably live in a config file
        my @remote_paths_to_replace =  ('/lus/scratch/watersc', '/scratch3/watersc1');
        my $local_path = 'neb://any';
        # find filenames that begin with the remote paths and change them to a nebulous path
        foreach my $remote_path (@remote_paths_to_replace) {
            if ($file_name =~ "^$remote_path") {
                $file_name =~ s{^$remote_path}{$local_path};
                last;
            }
        }
    }
    my $path = $ipprc->file_resolve($file_name);

    if (!$path and $file_rule =~ /LOG/) {
        my $compressed_file_name = $file_name . '.bz2';
        my $compressed_path = $ipprc->file_resolve($compressed_file_name);
        if ($compressed_path) {
            $path = $compressed_path;
            $file_name = $compressed_file_name;
        }
    }


    if (!$path) {
        # skip this file if $poor_quality
        # this is for compatability with older runs which don't have the files list in the
        # config dump.
        # Once we give up on supporting that we can remove the next line. (If the file is in the list
        # it must exist)
        next if $poor_quality;

        # skip file stats file. Due to a bug the update process destroys them sometimes
        # XXX: perhaps only do this for stages where we know that this happens
        next if $file_rule =~ /STATS/;
	# don't fail if these "non-essential files" are missing
	next if $file_rule =~ /LOG/;
	next if $file_rule =~ /TRACE/;
	next if $file_rule =~ /BIN/;

        # chip stage cmf files are not regenerated after cleanup so don't complain if
        # they are missing. They are of limited utility anyways. USE the smfs
        next if ($stage eq 'chip' and $file_rule eq 'PSPHOT.OUTPUT');

        &my_die("failed to resolve  $file_name", $component, $PS_EXIT_DATA_ERROR);
    }

    # open the file to make sure it exists (and to work around the failed mount phenomena)
    my $fh = open_with_retries($path);
    close $fh;

    # we need to pre-process the image before adding to the bundle. Save the path names.
    # the images will be created below
    $num_files++;
    if ($image_type && $nan_masked_pixels) {
        # save the
        if ($image_type eq 'image') {
            $image = $file_name;
        } elsif ($image_type eq 'mask') {
            $mask = $file_name;
        } elsif ($image_type eq 'variance') {
            $variance = $file_name;;
        } elsif ($image_type eq 'inv_image') {
            $inv_image = $file_name;
        } elsif ($image_type eq 'inv_mask') {
            $inv_mask = $file_name;;
        } elsif ($image_type eq 'inv_variance') {
            $inv_variance = $file_name;;
        } elsif ($image_type eq 'inv_variance') {
            $inv_variance = $file_name;;
        } else {
            &my_die("invalid image type found: $image_type", $component,
                       $PS_EXIT_PROG_ERROR);
        }
    } else {
        # create a symbolic link from the file in the nebulous repository
        # in the temporary directory
        symlink $path, "$tmpdir/" . basename($file_name);
    }
}

if (!$clean) {
    if ($stage eq 'chip_bg') {
        # add the variance file from the original chipRun
        my $command = "$bgtool -listchip -chip_bg_id $stage_id -class_id $component";
        $command .= " -dbname $dbname" if $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $component, $error_code);
        }
        my $mdcParser = PS::IPP::Metadata::Config->new; 
        my $list = $mdcParser->parse( join "", @$stdout_buf) or 
            &my_die("Unable to parse bgtool metadata", $component, $PS_EXIT_SYS_ERROR);
        my $parsed = parse_md_list($list);

        # result is an array with one element
        my $chip_data = $parsed->[0];

        my $chip_path_base = $chip_data->{path_base};
        $variance = $ipprc->filename('PPIMAGE.CHIP.VARIANCE', $chip_path_base, $component) or
            &my_die("Unable to resolve variance file name from $chip_path_base", $component, $PS_EXIT_SYS_ERROR);

    } elsif ($stage eq 'warp_bg') {
        # add the variance file from the original warpRun
        my $command = "$bgtool -listwarp -warp_bg_id $stage_id -skycell_id $component";
        $command .= " -dbname $dbname" if $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $component, $error_code);
        }
        my $mdcParser = PS::IPP::Metadata::Config->new; 
        my $list = $mdcParser->parse( join "", @$stdout_buf) or 
            &my_die("Unable to parse bgtool metadata", $component, $PS_EXIT_SYS_ERROR);
        my $parsed = parse_md_list($list);
        my $warp_data = $parsed->[0];
        my $warp_path_base = $warp_data->{path_base};
        $variance = $ipprc->filename('PSWARP.OUTPUT.VARIANCE', $warp_path_base) or
            &my_die("Unable to resolve variance file name from $warp_path_base", $component, $PS_EXIT_SYS_ERROR);
    }
}

if ($nan_masked_pixels) {
    # One last check as to whether magic has been applied to the inputs

    # Note: the sql for disttool -pendingcomponent won't select a component that
    # requires magic and hasn't been magicked, but we check again here

    if (!($magicked or $no_magic or $alt_path_base)) {
        &my_die("cannot create distribution bundle ${stage}_id $stage_id because the data has not been magic desreaked", $component, $PS_EXIT_DATA_ERROR);
    }

    &my_die("no image found in file list", $component, $PS_EXIT_CONFIG_ERROR) if !$image;
    if ($stage ne "raw") {
        &my_die("no mask image found in file list", $component, $PS_EXIT_CONFIG_ERROR) if !$mask;
        if (($stage ne "chip_bg") and ($stage ne "warp_bg")) {
            &my_die("no variance image found in file list", $component, $PS_EXIT_CONFIG_ERROR) if !$variance;
        } else {
            &my_die("variance is not defined", $component, $PS_EXIT_CONFIG_ERROR) if !$variance;
        }
    }

    my $class_id;
    # run streaksrelease to set masked pixels to NAN
    if ($stage eq "raw") {
        $class_id = $component;
        # we can use the chip mask because disttool demands that magic have been run
        # and so the camera mask and the chip mask are the same
        #$mask = $ipprc->filename("PPIMAGE.CHIP.MASK", $chip_path_base, $component);
        # XXX: hack the pending query passes in the camera path base
        $mask = $ipprc->filename("PSASTRO.OUTPUT.MASK", $chip_path_base, $component);
        my $mask_resolved = $ipprc->file_resolve($mask);
        my $fh = open_with_retries($mask_resolved);
        close $fh;
    } elsif ($stage eq "chip" or $stage eq "chip_bg") {
        $class_id = $component;
    }

    my $release_stage;
    if ($stage eq "chip_bg") {
        $release_stage = 'chip';
    } elsif ($stage eq 'warp_bg') {
        $release_stage = 'warp';
    } else {
        $release_stage = $stage;
    }


    my $command = "$streaksrelease -stage $release_stage -image $image -outroot $tmpdir";
    $command .= " -class_id $class_id" if $class_id;
    $command .= " -mask $mask" if $mask;
    $command .= " -chip_mask $mask" if ($stage eq 'chip' and $mask);
    $command .= " -weight $variance" if $variance;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $component, $error_code);
    }
    if ($inv_image) {
        $command = "$streaksrelease -stage $release_stage -image $inv_image -outroot $tmpdir";
        $command .= " -mask $inv_mask" if $inv_mask;
        $command .= " -weight $inv_variance" if $inv_variance;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $component, $error_code);
        }
    }
}


my $file_name;
my $bytes;
my $md5sum;

if ($num_files) {
    # create the tarfile
#    my $tbase = basename($path_base);
#    $tbase .= ".$component" if $component;
#    $file_name = ($prefix ? $prefix : "") . "$tbase.tgz";
    my $error;
    my $output_tarfile;
    my $rule;
    if ($stage eq 'chip' or $stage eq 'chip_bg' or $stage eq 'raw') {
        $rule = "DIST.OUTPUT.CHIP.BUNDLE";
    } else {
        $rule = "DIST.OUTPUT.BUNDLE";
    }
    $output_tarfile = $ipprc->prepare_output($rule, $outroot, $component, 1, \$error)
            or &my_die("Failed to prepare output tarfile: $error", $component, $error);
    $file_name = basename($output_tarfile);


    my $scheme = file_scheme($output_tarfile);

    my $tarfile;
    if ($scheme) {
        $ipprc->file_create($output_tarfile) 
            or &my_die("Failed to create $output_tarfile: $error", $component, $error);
        $tarfile = $ipprc->file_resolve($output_tarfile);

        if ($scheme eq 'neb') {
            &my_die("output file $output_tarfile not found", $component, $PS_EXIT_SYS_ERROR)
                unless ($tarfile and -e $tarfile);
        }
    } else {
        # no scheme the filename and the resolved filename are the same
        $tarfile = $output_tarfile;
    }

    my $command = "tar -C $tmpdir --owner=ipp --group=users -czhf $tarfile .";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $component, $error_code);
    }

    # tell the module not to die on error
    $Digest::MD5::File::NOFATALS = 1;
    $md5sum = file_md5_hex($tarfile);
    &my_die("unable to compute md5sum for $tarfile", $component, $PS_EXIT_UNKNOWN_ERROR) if !$md5sum;

    $bytes = -s $tarfile;

    delete_tmpdir($tmpdir);
} else {
    # no files for this component
    # XXX: Does this ever happen?
    $file_name = "none";
    $bytes = 0;
    $md5sum = "0";
}

if ($results_file) {
    open RF, ">$results_file" or &my_die("unable to open results file $results_file", $component, $PS_EXIT_UNKNOWN_ERROR);
    print RF "bundleResults MULTI\n\n";
    print RF "bundleResults METADATA\n";
    print RF "   name      STR    $file_name\n";
    print RF "   bytes     S64    $bytes\n";
    print RF "   md5sum    STR    $md5sum\n";
    print RF "END\n\n";

    close RF;
}

exit 0;

### Pau.

# return the image type (image, mask, or variance) if this file rule refers to
# one of the big fits files that are not included in a clean distribution
sub get_image_type {
    my $stage = shift;
    my $rule = shift;
    my $type;

    if ($stage eq "raw") {
        if ($rule eq "RAW.IMAGE") {
            $type = 'image';
        }
    } elsif ($stage eq "chip") {
        $type = $chip_cleaned{$rule};
    } elsif ($stage eq "chip_bg") {
        $type = $chip_bg_cleaned{$rule};
    } elsif ($stage eq "camera") {
        $type = $camera_cleaned{$rule};
    } elsif ($stage eq "fake") {
        $type = $fake_cleaned{$rule};
    } elsif ($stage eq "warp") {
        $type = $warp_cleaned{$rule};
    } elsif ($stage eq "warp_bg") {
        $type = $warp_bg_cleaned{$rule};
    } elsif ($stage eq "diff") {
        $type = $diff_cleaned{$rule};
    } elsif ($stage eq "SSdiff") {
        $type = $diff_cleaned{$rule};
    } elsif ($stage eq "stack") {
        $type = $stack_cleaned{$rule};
    } elsif ($stage eq "sky" or $stage eq "skycal" or $stage eq "stack_summary" or $stage eq "ff") {
        $type = $empty_cleaned{$rule};
    } else {
        &my_die("$stage is not a valid stage", $component, $PS_EXIT_CONFIG_ERROR);
    }
    return $type;
}

sub open_with_retries {
    my $name = shift;

    my $tries = 1;
    my $max_tries = 5;
    my $opened = 0;
    while (!$opened && ($tries <= $max_tries)) {
        $opened = open(IN, "<$name");
        if (!$opened) {
            print STDERR "WARNING failed to open $name re-try $tries\n";
            $tries++;
            sleep 5;
        }
    }

    &my_die("failed to open $name after $max_tries tries\n", $component,
                    $PS_EXIT_DATA_ERROR) if (!$opened);

    return *IN;
}

sub get_file_list {
    my $stage = shift;
    my $component = shift;
    my $path_base = shift;
    my $clean = shift;
    my $num_sky_inputs = shift;
    my $bt_state_good = shift;

    my @file_list;
    if ($stage eq "raw") {
        # raw files have no '.mdc' file thus we need to build the list of files by
        # hand
        # XXX do we want to distribute the registration log files? I vote no
        if (!$clean) {
            my %image;
            $image{file_rule} = "RAW.IMAGE";
            my $image_name;
            if ($alt_path_base) {
                $image_name = "$alt_path_base.fits";
            } else {
                $image_name = "$path_base.fits";
            }
            $image{name} = $image_name;
            push @file_list, \%image;

            if ($exp_type eq 'OBJECT' and $camera eq 'GPC1') {
                my %burntool_table;
                $burntool_table{file_rule} = "BURNTOOL.TABLE";
		
    		if($bt_state_good == 15) {
    		    $burntool_table{name} = "$path_base.burn.v15.tbl";	
    		} else {
    		    $burntool_table{name} = "$path_base.burn.tbl";		
    		}		
                push @file_list, \%burntool_table;
            }
        }
        return \@file_list;
    }

    # we get the list of output data products for this run from the config dump file that
    # is created when the run is done
    my $config_file_rule;
    if ($stage eq "chip") {
        $config_file_rule = "PPIMAGE.CONFIG";
    } elsif ($stage eq "chip_bg") {
        $config_file_rule = "PPBACKGROUND.CONFIG";
    } elsif ($stage eq "camera") {
        $config_file_rule = "PSASTRO.CONFIG";
    } elsif ($stage eq 'fake') {
        # XXX: fake is a no op now return an emtpy list
        return \@file_list;
    } elsif ($stage eq "warp") {
        $config_file_rule = "PSWARP.CONFIG";
    } elsif ($stage eq "warp_bg") {
        $config_file_rule = "PSWARP.CONFIG";
    } elsif ($stage eq "diff") {
        $config_file_rule = "PPSUB.CONFIG";
    } elsif ($stage eq "SSdiff") {
        $config_file_rule = "PPSUB.CONFIG";
    } elsif ($stage eq "stack") {
        $config_file_rule = "PPSTACK.CONFIG";
    } elsif ($stage eq "sky") {
        $config_file_rule = "PSPHOT.STACK.CONFIG";
    } elsif ($stage eq "skycal") {
        $config_file_rule = "PSASTRO.CONFIG";
    } elsif ($stage eq "stack_summary") {
        # stack_summary stage does not use a config dump file.
        return build_stack_summary_file_list($path_base, \@file_list);
    } elsif ($stage eq "ff") {
        if ($component eq 'summary') {
            # full force summary does not use a config dump file. Cobble together a file list
            return build_ff_summary_file_list($path_base, \@file_list);
        } else {
            $config_file_rule = "PSPHOT.SKY.CONFIG";
        }
    } else {
        &my_die("$stage is not a valid stage", $component, $PS_EXIT_CONFIG_ERROR);
    }
    my $config_file = $ipprc->filename($config_file_rule, $path_base, $component) or
                &my_die("can't get filename for config dump file: $config_file_rule", $component,
                    $PS_EXIT_CONFIG_ERROR);

    my %config_file_hash;

    # add the configuration file to the list
    $config_file_hash{file_rule} = $config_file_rule;
    $config_file_hash{name} = $config_file;
    push @file_list, \%config_file_hash;

    my $resolved = $ipprc->file_resolve($config_file);

    if (!$resolved and $poor_quality) {
        print STDERR "non config file found but continuing since component has poor quality\n";
        return undef
    }
    &my_die("failed to resolve name of config dump file: $config_file_rule", $component,
                    $PS_EXIT_CONFIG_ERROR) if (!$resolved);

    &my_die("config dump file resolved but not accessible: $config_file_rule", $component,
                    $PS_EXIT_CONFIG_ERROR) if !$ipprc->file_exists($resolved);

    my $mdc_compressed;
    {
        my $command = "$file_cmd -m $file_magic $resolved";
#        my $command = "$file_cmd $resolved";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $component, $error_code);
        }
        my $output = join "", @$stdout_buf;
        # XXX: may need to to make this more robust
        $mdc_compressed = ($output =~ /gzip/);
    }
    my $inName;
    if ($mdc_compressed) {
        my $tmpfile;
        ($tmpfile, $inName) = tempfile( "/tmp/bundle.XXXX", UNLINK => !$save_temps );
        close($tmpfile) or &my_die("failed to close $inName", $component, $PS_EXIT_UNKNOWN_ERROR);

        my $command = "$zcat $resolved > $inName";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $component, $error_code);
        }


    } else {
        $inName = $resolved;
    }
    my $in = open_with_retries($inName);

    # we don't use the mdc parser because the perl parser is way is too slow for complicated config
    # files like this
    my $line;
    while ($line = <$in>) {
        if ($line =~ /FILES.OUTPUT/) {
            # print "found FILES.OUTPUT\n";
            last;
        }
    }
    &my_die("config dump file does not contain FILES.OUTPUT: $config_file", $component,
                    $PS_EXIT_CONFIG_ERROR) if (!$line);

    while ($line = <$in>) {
        chomp $line;
        my ($key, $type, $val) = split " ", $line;
        # skip blank lines
        next if !$key;
        # we're done when we find END
        last if $key eq "END";
        # skip multi and other lines
        next if ($type ne "STR");

        # printf "%-32.32s   %s\n", $key, $val;

        &my_die("no value found for file rule $key in $resolved", $component,
                $PS_EXIT_CONFIG_ERROR) if (!$val);

        my %file;
        $file{file_rule} = $key;
        $file{name} = $val;
	if ($val eq "STDERR" or $val eq "STDOUT") {
	    print STDERR "Skipping $key because filename is $val\n";
	    next;
	}
        push @file_list, \%file;
    }
    close $in;

    return \@file_list;
}
sub get_num_sky_inputs {
    my $sky_id = shift;

    my $command = "$staticskytool -inputs -sky_id $sky_id -simple";
    $command .= " -dbname $dbname" if $dbname;
    $command .= " | wc";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $component, $error_code);
    }
    my ($num_inputs, $words, $chars) = split " ", (join "", @$stdout_buf);
    if (!$num_inputs) {
        $num_inputs = "undefined" if !defined $num_inputs;
        &my_die("unexpected number of static sky inputs $num_inputs",  $component, $error_code);
    }

    return $num_inputs;
}

# For stack_summary stage, we don't have a config dump file to give us the list of files.
# For now run neb-ls on the path base and take everything that matches.
sub build_stack_summary_file_list {
    my ($path_base, $file_list) = @_;

    my $scheme = file_scheme($path_base);

    if (!$scheme or $scheme ne 'neb') {
        &my_die("Building bundles for stack_summary not supported for no nebulous path_base.",
            $component, $PS_EXIT_PROG_ERROR);
    }

    my $command = "neb-ls $path_base" . '%';
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $component, $error_code);
    }
    my $nebls_output = join "", @$stdout_buf;

    my @files = split "\n", $nebls_output;

    my $i = 0;
    foreach my $f (@files) {
        my %file;
        $file{file_rule} = sprintf 'RULE.%d', $i++;
        $file{name} = 'neb://any/' . $f;
        push @$file_list, \%file;
    }

    return $file_list;
}

# psphotFullForceSummary does not produce a config dump file cobble together a file list
sub build_ff_summary_file_list {
    my ($path_base, $file_list) = @_;

    my %cmf ;
    $cmf{file_rule} = "SOURCES";
    $cmf{name} = "$path_base.cmf";
    push @$file_list, \%cmf;

    my %log;
    $log{file_rule} = "LOG";
    $log{name} = "$path_base.log";
    push @$file_list, \%log;

    return $file_list;
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $component = shift;      # class_id or skycell_id
    my $exit_code = shift;      # Exit code to add

    delete_tmpdir();

    carp($msg);
    exit $exit_code;
}

sub delete_tmpdir
{
    if (!$save_temps and $tmpdir and -e $tmpdir) {
        system "rm -r $tmpdir";
    }
}

__END__
