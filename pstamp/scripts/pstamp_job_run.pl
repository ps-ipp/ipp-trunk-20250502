#!/bin/env perl
###
### pstamp_job_run.pl
###
###     Run a given postage stamp Job
###

use warnings;
use strict;

use Sys::Hostname;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Carp;
use File::Basename;
use File::Copy;
use File::Temp qw(tempfile tempdir);
use Digest::MD5::File qw( file_md5_hex );
use PS::IPP::PStamp::RequestFile qw( :standard );
use PS::IPP::PStamp::Job qw( :standard );
use IPC::Cmd 0.36 qw( can_run run );
use POSIX;

use PS::IPP::Metadata::Config;
#use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw( :standard );

my ($job_id, $redirect_output, $outputBase, $rownum, $jobType, $options); 
my ($verbose, $dbname, $dbserver, $no_update, $save_temps);
my $very_verbose = 0; # for debugging

GetOptions(
    'job_id=s'          =>  \$job_id,
    'job_type=s'        =>  \$jobType,
    'rownum=s'          =>  \$rownum,
    'output_base=s'     =>  \$outputBase,
    'options=s'         =>  \$options,
    'redirect-output'   =>  \$redirect_output,
    'dbname=s'          =>  \$dbname,
    'dbserver=s'        =>  \$dbserver,
    'verbose'           =>  \$verbose,
    'no-update'         =>  \$no_update,
    'save-temps'        =>  \$save_temps,
);


my $host = hostname();
if ($verbose) {
    print "\n\n";
    print "Starting script $0 on $host\n\n";
}

die "job_id is required" if !$job_id;

my_die( "job_type is required", $job_id, $PS_EXIT_PROG_ERROR) if !$jobType;
my_die("rownum is required", $job_id, $PS_EXIT_PROG_ERROR) if !$rownum;
my_die("output_base is required", $job_id, $PS_EXIT_PROG_ERROR) if !$outputBase;

# ppstamp requires an input file
$options = $PSTAMP_SELECT_IMAGE if !$options;

my $ipprc = PS::IPP::Config->new(); # IPP Configuration
if ($redirect_output) {
    my $logDest = "$outputBase.log";
    $ipprc->redirect_output($logDest)
        or my_die ("unable to redirect output to $logDest", $job_id, $PS_EXIT_UNKNOWN_ERROR);
}

if ($verbose && $save_temps) {
    # we're probably debugging turn up the verbosity
    $very_verbose = 1;
}

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

my $temproot = metadataLookupStr($ipprc->{_siteConfig}, "TEMP.DIR");
$temproot = "/tmp" if !defined $temproot;

my $missing_tools;

my $pstamptool = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $ppstamp = can_run('ppstamp') or (warn "Can't find ppstamp" and $missing_tools = 1);
my $pstamp_get_image_job = can_run('pstamp_get_image_job.pl') or (warn "Can't find pstamp_get_image_job.pl" and $missing_tools = 1);
my $psgetcalibinfo = can_run('psgetcalibinfo') or (warn "Can't find psgetcalibinfo" and $missing_tools = 1);
my $dquery_job_run = can_run('dquery_job_run.pl') or (warn "Can't find dquery_job_run.pl" and $missing_tools = 1);
my $whichnode = can_run('whichnode') or (warn "can't find whichnode" and $missing_tools = 1);
my $ppBackground = can_run('ppBackground') or (warn "Can't find ppBackground" and $missing_tools = 1);
my $ppBackgroundStack = can_run('ppBackgroundStack') or (warn "Can't find ppBackgroundStack" and $missing_tools = 1);
my $stack_bkg_mk_mdc = can_run('stack_bkg_mk_mdc.pl') or (warn "Can't find stack_bkg_mk_mdc.pl" and $missing_tools = 1);
my $fpack = can_run('fpack') or (warn "Can't find fpack" and $missing_tools = 1);
my $staticskytool = can_run('staticskytool') or (warn "Can't find staticskytool" and $missing_tools = 1);

if ($missing_tools) {
    my_die("Can't find required tools", $job_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files


my $jobStatus;
if ($jobType eq "stamp") {
    my $params = read_params_file($outputBase);

    my $argString;
    $argString = $params->{job_args};
    
    my_die("argument list is empty", $job_id, $PS_EXIT_DATA_ERROR) if !$argString;

    my $stage = $params->{stage};
    my_die("stage is not defined", $job_id, $PS_EXIT_DATA_ERROR) if !$stage;

    if ($stage eq 'stack_summary') {

        # remove options not supported by stack summary
        $options &= ~($PSTAMP_SELECT_SOURCES | $PSTAMP_SELECT_BACKMDL | $PSTAMP_SELECT_INVERSE 
            | $PSTAMP_RESTORE_BACKGROUND);

        # stackSummary outputs do not follow the usual IPP conventions for file names.
        # The parser (actually Job.pm) has deferred handling this until here
        update_stack_summary_filenames($params);

    } elsif ($stage ne 'stack') {
        # ignore options only supported by stack and stack_summary
        $options &= ~($PSTAMP_SELECT_EXP | $PSTAMP_SELECT_NUM);
    }
   

    if ($stage eq "raw") {
        # zap options that don't apply to raw stage
        $options &= ~($PSTAMP_SELECT_MASK | $PSTAMP_SELECT_VARIANCE); 
    }

    if ($options & $PSTAMP_RESTORE_BACKGROUND) {
        #  this subroutine creates a background restored version of the image or fails.
        # upon success params->{image} is replaced with the new (temporary) version
        my $error_code = create_background_restored_images($params);
        if ($error_code) {
            # if error code is one of the permanant ones set state of job to stop
            my_die("failed to create background restored images", $job_id, $error_code, 
                $error_code >= $PSTAMP_FIRST_ERROR_CODE ? 'stop' : undef);
        }
    }

    my $image = $params->{image};
    my $mask;
    my $variance;

    my $fileArgs = " -file $params->{image}";
    my @file_list = ($params->{image});
    
    if ($options & $PSTAMP_SELECT_MASK) {
        $mask = $params->{mask};
        $fileArgs .= " -mask $mask";
        push @file_list, $mask;
    }
    if ($options & $PSTAMP_SELECT_VARIANCE) {
        $variance = $params->{weight};
        $fileArgs .= " -variance $variance";
        push @file_list, $variance;
    }

    if ($params->{astrom}) {
        $argString .= " -astrom $params->{astrom}";
        push @file_list, $params->{astrom};
    }

    if ($options & $PSTAMP_SELECT_SOURCES) {
        # Extract sources from astrometry file if provided. This will be the smf for chip stage
        # or the skycal cmf for stacks
        if ($params->{astrom}) {
            $argString .= " -write_cmf";
            if ($stage eq 'stack') {
                # Set psphot recipe to STACKPHOT so that the extended source paramters will
                # be copied from the cmf file.
                $argString .= " -recipe PSPHOT STACKPHOT"
            }
        } elsif ($stage eq 'stack') {
            # no astrom file for stack (skycal cmf) Go find a staticsky cmf and use that
            # otherwise silently ignore the request for sources since the stack smf is not useful.
            my $staticsky_cmf = findStaticskyCMF($params);
            if ($staticsky_cmf) {
                print "Using staticsky cmf file $staticsky_cmf for sources\n";
                $argString .= " -write_cmf";
                $fileArgs  .= " -sources $staticsky_cmf";
                # Set psphot recipe to STACKPHOT so that the extended source paramters will
                # be copied from the cmf file.
                $argString .= " -recipe PSPHOT STACKPHOT";
                push @file_list, $staticsky_cmf;
            } else {
                print "No skycal or staticsky cmf for $params->{stack_id} ignoring request for sources\n";
            }
        } elsif ($params->{cmf}) {
            $argString .= " -write_cmf";
            $fileArgs  .= " -sources $params->{cmf}";
            push @file_list, $params->{cmf};
        } else {
            print "Could not find suitable sources file will not write cmf\n";
        }
    }

    # check that actual input files exist
    check_files($PSTAMP_NOT_AVAILABLE, @file_list);

    # find our output directory
    my $outdir = dirname($outputBase);

    my ($calib_fd, $calibfile);
    if ($stage eq 'chip' or $stage eq 'warp') {
        my $cam_id = $params->{cam_id};
        if (!$cam_id) {
            carp "no cam_id found in job params\n";
            exit $PS_EXIT_PROG_ERROR;
        }
        ($calib_fd, $calibfile) = tempfile ("$outdir/calib.XXXX", UNLINK => !$save_temps);
        close $calib_fd;

        my $command = "$psgetcalibinfo --cam_id $cam_id --output $calibfile --dbname $params->{imagedb}";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);

        my $exitStatus;
        if (WIFEXITED($error_code)) {
            $exitStatus = WEXITSTATUS($error_code);
        } else {
            print STDERR "psgetcalibinfo failed error_code: $error_code\n";
            $exitStatus = $PS_EXIT_SYS_ERROR;
        }
        exit $exitStatus if $exitStatus;

        if (-s $calibfile == 0) {
            print "no calibration information found for $cam_id\n";
            $calibfile = undef;
        }
    }

    # unless the stage is stack_summary we use ppstamp to make postage stamps (including -wholefile)
    # if stage is stack_summary we make copies of the input files to the outputs
    my $use_ppstamp = ($stage ne 'stack_summary');

    my $exitStatus;
    if ($use_ppstamp) {
        my $command = "$ppstamp $outputBase $argString $fileArgs";
        $command .= " -write_jpeg" if ($options & $PSTAMP_SELECT_JPEG);
        $command .= " -nocompress" if ($options & $PSTAMP_SELECT_UNCOMPRESSED);
        $command .= " -stage $stage";
        $command .= " -forheader $calibfile" if $calibfile;
	## MEH hack for centeroffchip -- needs constraint for coord_mask 0 all in sky values..
	$command .= " -centeroffchip" if ($options & $PSTAMP_MULTI_OVERLAP_IMAGE);

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);

        if (WIFEXITED($error_code)) {
            $exitStatus = WEXITSTATUS($error_code);
        } else {
            print STDERR "ppstamp failed error_code: $error_code\n";
            $exitStatus = $PS_EXIT_SYS_ERROR;
        }

        # if stage is stack deal with EXP and NUM images if selected by running ppstamp again
        if (!$exitStatus and $stage eq 'stack' and ($options & ($PSTAMP_SELECT_EXP | $PSTAMP_SELECT_NUM))) {
            # XXX Here I am assuming that nothing relevant gets added to $argString 
            # Need to check
            my $roiArgs = $params->{job_args};

            # XXX: define expnum and exptime input images in the params file so that we don't have to
            # make assumptions about the file rules here
            my $pathBase = $params->{stack_path_base};
            $pathBase = $params->{path_base} unless defined $pathBase;
            unless (defined $pathBase) {
                my_die("stack path base undefined found when creating exp or expnum image", $job_id, $PS_EXIT_PROG_ERROR);
            }
            $pathBase .= ".unconv";
            my $tmpBase = "$outputBase.TMP";
            # nocompress because exp image gets corrupted if we do
            my $command = "$ppstamp $tmpBase $roiArgs  -nocompress -stage stack";
            # XXX: use file rules
            # Image is the EXP (exptime) image
            $command .= " -file $pathBase.exp.fits";
            # treat number image as mask
            $command .= " -mask $pathBase.num.fits" if ($options & $PSTAMP_SELECT_NUM);

            $command .= " -forheader $calibfile" if $calibfile;

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);

            if (WIFEXITED($error_code)) {
                $exitStatus = WEXITSTATUS($error_code);
            } else {
                print STDERR "ppstamp failed error_code: $error_code\n";
                $exitStatus = $PS_EXIT_SYS_ERROR;
            }

            # command to compress fits files to stdout using gzip
            my $fpack_gzip = "$fpack -g -S";
            $fpack_gzip .= " -D" unless $save_temps;

            if (!$exitStatus and ($options & $PSTAMP_SELECT_NUM)) {
                # rename the num image if selected since it is not a mask
                # Change .exp.mk to .num
                my $tmpName = "$tmpBase.mk.fits";
                my $numName = "$outputBase.num.fits";
                # optionally fpack it while we're here unless user wants uncompressed images
                unless ($options & $PSTAMP_SELECT_UNCOMPRESSED) {
                    my $command = "$fpack_gzip $tmpName > $numName";
                    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run(command => $command, verbose => $verbose);

                    if (WIFEXITED($error_code)) {
                        $exitStatus = WEXITSTATUS($error_code);
                    } else {
                        print STDERR "ppstamp failed error_code: $error_code\n";
                        print STDERR "@$stderr_buf\n";
                        $exitStatus = $PS_EXIT_SYS_ERROR;
                    }
                } elsif (!$exitStatus) {
                    rename $tmpName, $numName 
                        or my_die ("failed to rename $tmpName to $numName", $job_id, $PS_EXIT_UNKNOWN_ERROR);
                }

            }
            # fpack the exp image if not -nocompress
            unless ($exitStatus or ($options & $PSTAMP_SELECT_UNCOMPRESSED)) {
                my $command = "$fpack_gzip $tmpBase.fits > $outputBase.exp.fits";
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);

                if (WIFEXITED($error_code)) {
                    $exitStatus = WEXITSTATUS($error_code);
                } else {
                    print STDERR "ppstamp failed error_code: $error_code\n";
                    print STDERR "@$stderr_buf\n";
                    $exitStatus = $PS_EXIT_SYS_ERROR;
                }
            } elsif (!$exitStatus) {
                rename "$tmpBase.fits", "$outputBase.exp.fits"
                    or my_die ("failed to rename $tmpBase.fits to $outputBase.exp.fits", $job_id, $PS_EXIT_UNKNOWN_ERROR);
            }
        }
    } else {
        $exitStatus = justCopyFiles($outputBase, \$options, $params);
    }

    if ($exitStatus == 0) {
        my $reglist = "$outdir/reglist$job_id";

        my $F;
        open $F, ">$reglist" or my_die( "can't open $reglist for output", $job_id, $PS_EXIT_UNKNOWN_ERROR);

        # Figure out what output images were produced

        # Note: we are assuming the contents of the PSTAMP filerules here.
        my %extensions = ( $PSTAMP_SELECT_IMAGE    => 'fits', 
                           $PSTAMP_SELECT_MASK     => 'mk.fits',
                           $PSTAMP_SELECT_VARIANCE => 'wt.fits',
                           $PSTAMP_SELECT_SOURCES  => 'cmf',
                           $PSTAMP_SELECT_JPEG     => 'jpg',
                           $PSTAMP_SELECT_EXP      => 'exp.fits',
                           $PSTAMP_SELECT_NUM      => 'num.fits',
                           $PSTAMP_SELECT_EXPJPEG  => 'exp.jpg',
                           $PSTAMP_SELECT_NUMJPEG  => 'num.jpg');

        my $output_mask = $options & ($PSTAMP_SELECT_IMAGE | $PSTAMP_SELECT_MASK | $PSTAMP_SELECT_VARIANCE 
            | $PSTAMP_SELECT_JPEG | $PSTAMP_SELECT_SOURCES
            | $PSTAMP_SELECT_EXP | $PSTAMP_SELECT_NUM 
            | $PSTAMP_SELECT_EXPJPEG | $PSTAMP_SELECT_NUMJPEG);


        foreach my $key (keys (%extensions)) {
            my $do_this_one = $key & $output_mask;

            next if (! $do_this_one);

            my $extension = $extensions{$key};

            my $basename = basename($outputBase);

            my $filename = "${basename}.${extension}";
            my $path  = "${outputBase}.${extension}";

            # XXX is pstamp always the right file type, if not where do we get the right one?
            print $F file_registration_line($filename, $path, "pstamp") . "\n";
        }

        get_other_outputs($F, $outputBase, $options, $params);

        close $F;
        $jobStatus = $PS_EXIT_SUCCESS;
    } elsif ($exitStatus == $PSTAMP_NO_OVERLAP || $exitStatus == $PSTAMP_NO_VALID_PIXELS) {
        $jobStatus = $exitStatus;
    } else {
        my_die( "ppstamp failed with error code: $exitStatus", $job_id, $exitStatus);
    }
} elsif ($jobType eq "get_image") {

    my $pstamp_bundle_root = metadataLookupStr($ipprc->{_siteConfig}, "PSTAMP_BUNDLE_ROOT");

    my $params = read_params_file($outputBase);
    my $imagedb = $params->{imagedb};

    my $command = "$pstamp_get_image_job --job_id $job_id --output_base $outputBase --rownum $rownum";
    $command .= " --bundleroot $pstamp_bundle_root" if $pstamp_bundle_root;
    $command .= " --imagedb $imagedb" if $imagedb;
    $command .= " --dbname $dbname" if $dbname;
    $command .= " --dbserver $dbserver" if $dbserver;
    $command .= " --verbose" if $verbose;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    if ($success) {
        $jobStatus = $PS_EXIT_SUCCESS;
    } else {
        $jobStatus = $error_code >> 8;
        my_die( "pstamp_get_image_job failed with error code: $jobStatus", $job_id, $jobStatus);
    }
} elsif ($jobType eq "detect_query") {
    # my $outdir = dirname($outputBase);
    # my $argslist = "$outdir/parse.args";
    # open ARGSLIST, "<$argslist" or my_die("failed to open argslist file $argslist", $job_id, $PS_EXIT_UNKNOWN_ERROR);
    # my $argString = <ARGSLIST>;
    # close ARGSLIST;
    # chomp $argString;

    # XXX: should we do any other sanity checking?
    # my_die("arglist file $argslist is empty", $job_id, $PS_EXIT_DATA_ERROR) if !$argString;

    my $command = "$dquery_job_run --job_id $job_id --output_base $outputBase";
    $command .= " --dbname $dbname" if $dbname;
    $command .= " --dbserver $dbserver" if $dbserver;
    $command .= " --verbose" if $verbose;
    $command .= " --save-temps" if $save_temps;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);

    # Although dqueryparse can potentially force more updates (and more jobs), it should still return success.
    if ($success) {
	$jobStatus = $PS_EXIT_SUCCESS;
    } else {
	$jobStatus = $error_code >> 8;
	my_die("dquery_job_run.pl failed with error code: $jobStatus", $job_id, $jobStatus);
    }
} elsif ($jobType eq "child") {
    # the only thing jobs of jobType child is to finish
    $jobStatus = 0;
} else {
    my_die("unknown jobType $jobType found", $job_id, $PS_EXIT_PROG_ERROR);
}

# mark the job stopped in the database
{
    my $command = "$pstamptool -updatejob -job_id $job_id -set_state stop"; 
    $command .= " -set_fault $jobStatus" if $jobStatus;
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;
    if (!$no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            die("Unable to perform $command: $error_code");
        }
    } else {
        print STDERR "skipping command: $command\n"
    }
}

exit 0;

# create a string to be passed as input to dsreg when registering this file in a fileset
# XXX move this to a module so it can be shared

sub file_registration_line {
    my $filename = shift;
    my $path     = shift;
    my $filetype = shift;
    if (-e $path) {
        my @finfo = stat($path);
        my_die("failed to stat $path", $job_id, $PS_EXIT_UNKNOWN_ERROR) unless (@finfo); 
        my $bytes = $finfo[7];
        my $md5sum = file_md5_hex($path);

        return "$filename|$bytes|$md5sum|$filetype|";
    } else {
        my_die("$filename not found at $path", $job_id, $PS_EXIT_UNKNOWN_ERROR);
    }
}
        
sub get_other_outputs {
    my $f = shift;
    my $output_base = shift;
    my $options = shift;
    my $params = shift;

    if ($options & ( $PSTAMP_SELECT_CMF | $PSTAMP_SELECT_PSF | $PSTAMP_SELECT_BACKMDL)) {
        if (!$params) {
            $params = read_params_file($output_base);
        }

        my $stage = $params->{stage};

        # raw files don't have any other data products
        return 1 if $stage eq "raw";

        # add the other data products if they are selected and exist.
        # silently skip them if they don't exist. Perhaps this should be
        # detected in pstampparse so that the user can be notified with 
        # a message in parse_error.txt ("warp does not have a background model")
        my $psf_file = $params->{psf} if ($options & $PSTAMP_SELECT_PSF);
        my $backmdl_file = $params->{backmdl} if ($options & $PSTAMP_SELECT_BACKMDL);
        my $pattern_file = $params->{pattern} if ($options & $PSTAMP_SELECT_BACKMDL);
        my $filter = $params->{filter};
        $filter = ' ' if !$filter;
	$filter = substr $filter, 0, 1;
        my $cmf_file;
#        if ($stage ne 'chip') {
#            # we don't ship chip stage cmf files because they may not be censored
#            $cmf_file = $params->{cmf} if ($options & $PSTAMP_SELECT_CMF);
#        }

        my $outdir = dirname($output_base);
        my $basename = basename($output_base);
        my ($rownum, $jobnum, $therest) = split /_/, $basename;
        &my_die("failed to split basename: $basename", $job_id, $PS_EXIT_CONFIG_ERROR) 
            if (!$therest or !$rownum or !$jobnum);

        # XXX: Here we are assuming the form of the output file names
	# if we change this in pstampparse we'll need to remember ....
	# (the last time I forgot)
        my $prefix = "${rownum}_${jobnum}_${filter}_";

        if ($cmf_file) {
            print "cmf file is $cmf_file\n";
            copy_and_register_file($f, $cmf_file, $outdir, $prefix);
        }
        if ($psf_file) {
            print "psf_file is $psf_file\n";
            copy_and_register_file($f, $psf_file, $outdir, $prefix);
        }
        if ($backmdl_file) {
            print "backmdl_file is $backmdl_file\n";;
            copy_and_register_file($f, $backmdl_file, $outdir, $prefix);
        }
       if (0) {
        # don't enable this yet
        if ($pattern_file) {
            print "pattern_file is $pattern_file\n";;
            copy_and_register_file($f, $pattern_file, $outdir, $prefix);
        }
       }
    }
}

sub read_params_file {
    my $output_base = shift;

    my $params_file = $output_base . ".mdc";
    open (IN, "<$params_file") 
        or my_die("failed to open params file: $params_file", $job_id, $PS_EXIT_UNKNOWN_ERROR);

    my $data = $mdcParser->parse(join "", (<IN>))
        or my_die("failed to parse params file: $params_file", $job_id, $PS_EXIT_UNKNOWN_ERROR);

    my $components = parse_md_list($data);

    my $n = scalar @$components;
    if ($n != 1) {
        my_die("params file $params_file contains unexpected number of components: $n",
                $job_id, $PS_EXIT_PROG_ERROR);
    }
    return $components->[0];
}

# copy_and_register_file ($f, $src, $destdir, $prefix);
sub copy_and_register_file {
    my $F = shift;
    my $src = shift;
    my $destdir = shift;
    my $prefix = shift;

    my $fn = $prefix . basename($src);
    my $dst = "$destdir/$fn";

    my $resolved = $ipprc->file_resolve($src);

    my_die("failed to resolve $src", $job_id, $PS_EXIT_UNKNOWN_ERROR) if !$resolved;

    if (!-e $resolved) {
        print STDERR "$src does not exist, skipping\n";
        return;
    }


    copy($resolved, $dst) or my_die("failed to copy $resolved to $dst", $job_id, $PS_EXIT_UNKNOWN_ERROR);

    print $F file_registration_line($fn, $dst, "fits") . "\n";
}

sub check_files {
    my $error_code = shift;
    my $return_code = 1;
    foreach my $f (@_) {
        if (!$ipprc->file_exists($f)) {
            my $gone;
            if (storage_object_exists($f, \$gone)) {
                if ($error_code and $gone) {
                    my_die( "file $f is GONE:", $job_id, $PSTAMP_GONE, 'stop');
                } else {
                    print STDERR "file $f is not available\n";
                    $return_code = 0;
		    ## MEH 20210127 doesn't look like return_code is even used from check_files...
		    ## if file is not available, why is my_die not called with PSTAMP_NOT_AVAILABLE?
		    ## is this only the case when neb-host down and fails to wait for fix?
		    ## with multiple data nodes down, this needs to be a fault case
		    my_die( "file $f is not available:", $job_id, $error_code, 'stop'); 
                }
            } else {
                # This shouldn't happen. The job shouldn't have been queued unless the file
                # exist. I guess if cleanup got triggered after the job was queued
                my_die( "file $f does not exist:", $job_id, $error_code, 'stop');
            }
        }
    }
    return $return_code;
}

my $neb;
sub storage_object_exists
{
    my $file = shift;
    my $ref_all_gone = shift;

    if (!$neb) {
        my $scheme = file_scheme($file);
        if ($scheme and $scheme eq 'neb') {
            $neb = $ipprc->nebulous();
        } else {
            return 0;
        }
    }

    my $exists = $neb->storage_object_exists($file);
    if (!$exists) {
        return 0;
    }

    my $command = "$whichnode $file";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform whichnode: $error_code", $job_id, $PS_EXIT_CONFIG_ERROR, 'run');
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

# stack_summary stage does not currently use proper file rules.
# The parser defers handlint this to us..

sub update_stack_summary_filenames {
    my $params  = shift;

    my $path_base = $params->{path_base};

    $params->{image}  = $path_base . ".image.b1.fits";
    $params->{mask}   = $path_base . ".mask.b1.fits";
    $params->{weight} = $path_base . ".variance.b1.fits";
    $params->{jpeg}   = $path_base . ".image.0.b1.jpeg";
    $params->{exp}    = $path_base . ".exp.b1.fits";
    $params->{num}    = $path_base . ".num.b1.fits";
    $params->{expjpeg} = $path_base . ".exp.0.b1.jpeg";
    $params->{numjpeg} = $path_base . ".num.0.b1.jpeg";
}

sub myCopy {
    my ($dest, $src, $type, $dieOnFail) = @_;

    my $result;
    my $resolved = $ipprc->file_resolve($src);
    if ($resolved and $ipprc->file_exists($resolved)) {
        print "Copying $src to $dest\n" if $verbose;;
        $result = copy($resolved, $dest);
    } else {
        my $msg = "Specified source $type image $src not found.";
        if ($dieOnFail) {
            $msg .= "\n";
        } else {
            $msg .= " ignoring\n";
        }
        carp $msg;
        $result = 0;
    }
    
    if (!$result and $dieOnFail) {
        &my_die("Unable to copy $type image", $job_id, $PS_EXIT_SYS_ERROR, 'run');
    }
    return $result;
}

sub justCopyFiles {
    my ($outputBase, $r_options, $params) = @_;

    print "Just copying files for $job_id.\n";

    my $options = $$r_options;
    if ($options & $PSTAMP_SELECT_IMAGE) {
        myCopy("$outputBase.fits", $params->{image}, 'image', 1);
    }
    if ($options & $PSTAMP_SELECT_MASK) {
        if (!myCopy("$outputBase.mk.fits", $params->{mask}, 'mask', 0)) {
            $options &= ~$PSTAMP_SELECT_MASK;
        }
    }
    if ($options & $PSTAMP_SELECT_VARIANCE) {
        if (!myCopy("$outputBase.wt.fits", $params->{weight}, 'variance', 0)) {
            $options =  ~$PSTAMP_SELECT_VARIANCE;
        }
    }
    if ($options & $PSTAMP_SELECT_JPEG) {
        if (!myCopy("$outputBase.jpg", $params->{jpeg}, 'jpeg', 0)) {
            $options &= ~$PSTAMP_SELECT_JPEG;
        }
    }
    if ($options & $PSTAMP_SELECT_EXP) {
        if (!myCopy("$outputBase.exp.fits", $params->{exp}, 'exp', 0)) {
            $options &= ~$PSTAMP_SELECT_EXP;
        }
    }
    if ($options & $PSTAMP_SELECT_NUM) {
        if (!myCopy("$outputBase.num.fits", $params->{num}, 'num', 0)) {
            $options &= ~$PSTAMP_SELECT_NUM;
        }
    }
    if ($options & $PSTAMP_SELECT_EXPJPEG) {
        if (!myCopy("$outputBase.exp.jpg", $params->{expjpeg}, 'exp', 0)) {
            $options &= ~$PSTAMP_SELECT_EXPJPEG;
        }
    }
    if ($options & $PSTAMP_SELECT_NUMJPEG) {
        if (!myCopy("$outputBase.num.jpg", $params->{numjpeg}, 'num', 0)) {
            $options &= ~$PSTAMP_SELECT_NUMJPEG;
        }
    }

    $$r_options = $options;

    print "Done with copy.\n";

    return 0;
}

# use ppBackground or ppBackgroundStack to produce background restored images
sub create_background_restored_images {
    my $params = shift;
    my $stage = $params->{stage};

    if ($stage ne 'stack' and $stage ne 'chip') {
        # this function is only supported for chip and stack stage
        # perhaps the parser should catch this ...
        print STDERR "background restoration not currently supported for stage $params->{stage}\n";
        return $PSTAMP_BG_RESTORE_NOT_AVAILABLE;
    }

    my $tempdir = tempdir("$temproot/pstamp.$job_id.XXXX", CLEANUP => !$save_temps);
    my $imagedb = $params->{imagedb};
    my $camera = $params->{camera};

    # reinstantiate IPP Configuration using the now known camera so that we get the applicable file rules
    $ipprc = PS::IPP::Config->new($camera);

    my $input_image = $params->{image};
    my $input_mask = $params->{mask};
    my $input_variance = $params->{weight};

    if ($stage eq 'stack') {
        my $stack_id = $params->{stack_id};
        my $mdcfile = "$tempdir/stk.$stack_id.bkg_input.mdc";
        {
            my $command = "$stack_bkg_mk_mdc --stack_id $stack_id --camera $camera --dbname $imagedb > $mdcfile";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform stack_bg_mk_mdc.pl: $error_code", $job_id, $PS_EXIT_CONFIG_ERROR, 'run');
            }
        }

        my $outroot = "$tempdir/stk.$stack_id";

        # XXX: get otapath from a config file or detrend system
        my $otapath = "neb:///detrends/background_OTA_models.20140527/test_solutions";
        {
            my $command = "$ppBackgroundStack $outroot -input $mdcfile -image $input_image"
               . " -OTApath $otapath" ;
            $command .= " -mask $input_mask" if $input_mask;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $very_verbose);
            unless ($success) {
                # we turned off verbosity because ppBackgroundStack emits too much output
                # XXX: perhaps use a log file and save it with the job data
                print STDERR join "", @$stderr_buf;
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform ppBackgroundStack: $error_code", $job_id, $PS_EXIT_CONFIG_ERROR, 'run');
            }
        }
        # it's all good change the image to be passed to ppstamp
        $params->{image} = $ipprc->filename('PPBACKGROUND.STACK.OUTPUT', $outroot) or
                &my_die("Unable to resolve ppBackgroundStack output file name", $job_id, $PS_EXIT_UNKNOWN_ERROR, 'run');

    } elsif ($stage eq 'chip') {
        my $input_mdl = $params->{backmdl};
        my $chip_id = $params->{chip_id};
        my $class_id = $params->{class_id};
        my $outroot = "$tempdir/bgrestored.ch.$chip_id";
        {
            my $command = "$ppBackground $outroot -background $input_mdl -image $input_image";
            # XXX: I think ppBackground might always require mask and variance images
            $command .= " -mask $input_mask";
            $command .= " -variance $input_variance";

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                print STDERR join "", @$stderr_buf;
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform ppBackground: $error_code", $job_id, $PS_EXIT_CONFIG_ERROR, 'run');
            }
        }
        $params->{image} = $ipprc->filename('PPBACKGROUND.OUTPUT', $outroot, $class_id) or
                &my_die("Unable to resolve ppBackgroundStack output image name", $job_id, $PS_EXIT_CONFIG_ERROR, 'run');
        $params->{mask} = $ipprc->filename('PPBACKGROUND.OUTPUT.MASK', $outroot, $class_id) or
                &my_die("Unable to resolve ppBackgroundStack output mask name", $job_id, $PS_EXIT_CONFIG_ERROR, 'run');
        $params->{weight} = $ipprc->filename('PPBACKGROUND.OUTPUT.VARIANCE', $outroot, $class_id) or
                &my_die("Unable to resolve ppBackgroundStack output variance name", $job_id, $PS_EXIT_CONFIG_ERROR, 'run');

    } else {
        # can't get here
        &my_die( "background restoration not currently supported for stage $params->{stage}\n",
            $job_id, $PS_EXIT_PROG_ERROR, 'stop'); ;
    }

    return 0;
}
sub findStaticskyCMF {
    my $params = shift;
    my $stack_id = $params->{stack_id};

    &my_die( "NO stack_id found in findStaticskyCMF\n",
            $job_id, $PS_EXIT_PROG_ERROR, 'run') if !$stack_id;

    my $command = "$staticskytool -dbname $params->{imagedb} -result -stack_id $stack_id";
    my $results = runToolAndParse($command, $verbose);

    if (!$results) {
        print "no staticsky results found for stack $stack_id\n";
        return undef;
    }

    my $latest;
    my $latest_sky_id = 0;
    foreach my $result (@$results) {
        if ($result->{sky_id} > $latest_sky_id) {
            $latest = $result;
            $latest_sky_id = $result->{sky_id};
        }
    }
    if ($latest) {
        # XXXX Use proper file rule
        return $latest->{path_base} .  ".stk.$stack_id.cmf";
    } 
    # Can't happen can it?
    return undef;
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $job_id = shift;         # job identifier
    my $exit_code = shift;      # Exit code to add
    my $job_state = shift;      # new pstampJob.state

    $exit_code = $PS_EXIT_PROG_ERROR unless $exit_code;
    if ($exit_code > 100) {
        carp ("invalid exit code: $exit_code changing to $PS_EXIT_UNKNOWN_ERROR");
        $exit_code = $PS_EXIT_UNKNOWN_ERROR;
    }
    $job_state = 'run' unless $job_state;

    carp($msg);
    if (defined $job_id and not $no_update) {
        my $command = "$pstamptool -updatejob";
        $command .= " -job_id $job_id";
        $command .= " -set_fault $exit_code";
        # XXX: fix pstamptool to not require -state when -fault with nonzero value is provided
        $command .= " -set_state $job_state";
        $command .= " -dbname $dbname" if defined $dbname;
        $command .= " -dbserver $dbserver" if defined $dbserver;
        system($command);
    }
    exit $exit_code;
}

