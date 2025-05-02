#!/usr/bin/env perl

# this program has two jobs:
# 1: for the given exp_tag, generate the binned & mosaiced JPEG images
# 2: examine the collection of per-imfile statistics and reject.  At the moment,
#    this program (and the database) only allows rejection at the exposure level,
#    not at the imfile level.

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
print "\n\n";
print "Starting script $0 on $host\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );             # tools to run UNIX programs with control over I/O
# use IPC::Run qw ( start finish timeout );
use IPC::Run;

use PS::IPP::Metadata::Config;                   # tools to parse the psMetadataConfig files

use PS::IPP::Metadata::List qw( parse_md_list ); # tools to parse a metadata into a hash list
use Statistics::Descriptive;                     # tools for calculating basic statistical quantities
use File::Temp qw( tempfile );                   # tools to construct temp files
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt ); # option parsing
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
my $ppImage = can_run('ppImage') or (warn "Can't find ppImage" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# parse the command-line options
my ( $det_id, $iter, $exp_id, $exp_tag, $det_mode, $det_type, $camera, $filter, $reject, $outroot, $dbname, $reduction,
     $verbose, $no_update, $no_op, $save_temps, $redirect );
GetOptions(
           'det_id|d=s'        => \$det_id,
           'iteration=s'       => \$iter,
           'exp_id|e=s'        => \$exp_id,
           'exp_tag|=s'        => \$exp_tag,
           'det_type|t=s'      => \$det_type,
           'det_mode=s'        => \$det_mode,
           'camera=s'          => \$camera,
           'outroot|w=s'       => \$outroot,   # output file base name
           'filter=s'          => \$filter,
           'reject'            => \$reject,
           'dbname|d=s'        => \$dbname, # Database name
           'reduction|=s'      => \$reduction,
           'verbose'           => \$verbose,   # Print to stdout
           'no-update'         => \$no_update,
           'no-op'             => \$no_op,
           'save-temps'        => \$save_temps, # Save temporary files?
           'redirect-output'   => \$redirect,   # redirect output to LOG.EXP
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --det_id --iteration --exp_id --exp_tag --det_type --camera --outroot",
           -exitval => 3) unless
    defined $det_id   and
    defined $iter     and
    defined $exp_id   and
    defined $exp_tag  and
    defined $det_type and
    defined $det_mode and
    defined $camera   and
    defined $outroot;

# force det_type to be upper-case in this script
$det_type = uc($det_type);

# load IPP config information for the specified camera
my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $det_id, $iter, $exp_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
my $logDest = $ipprc->filename("LOG.EXP", $outroot) or &my_die("Missing entry from camera config", $det_id, $iter, $exp_id, $PS_EXIT_CONFIG_ERROR);
$ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $det_id, $iter, $exp_id, $PS_EXIT_SYS_ERROR ) if $redirect;

# Recipes to use based on reduction class
$reduction = 'DETREND' unless defined $reduction;

my $recipe = $ipprc->reduction($reduction, $det_type . '_JPEG_RESID'); # Recipe to use
&my_die("Unrecognised detrend type: $det_type", $det_id, $iter, $PS_EXIT_PROG_ERROR) unless defined $recipe;

# variables used for I/O
my ($command, $success, $error_code, $full_buf, $stdout_buf, $stderr_buf);

# Get list of normalizations by class_id : stored as $norms; save to temp file for ppImage runs below
my (%norms, $normsName);
if ($det_mode eq 'master') {
    # dettool command to select imfile data for this exp_id
    $command  = "$dettool -normalizedstat";
    $command .= " -det_id $det_id";
    $command .= " -iteration $iter";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -residimfile: $error_code\n");
        exit($error_code);
    }
    if (@$stdout_buf == 0) {
        &my_die("No normalizations were found", $det_id, $iter, $PS_EXIT_PROG_ERROR);
    }

    # Parse the stdout buffer into a metadata
    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $det_id, $iter, $exp_id, $PS_EXIT_PROG_ERROR);

    # parse the file info in the metadata
    my $normsMD = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $det_id, $iter, $exp_id, $PS_EXIT_PROG_ERROR);


    # write the normalizations to a file as a metadata config file in the form: class_id F32 value
    # XXX a possible optimization: if there is only one imfile, skip normalization
    # Correcting the resid imfiles and stats requires us to *divide* by the normalization
    # we do this by inverting the normalization here:
    my $normsFile;
    ($normsFile, $normsName) = tempfile( "/tmp/$exp_tag.detresid.$det_id.$iter.norms.XXXX", UNLINK => !$save_temps );
    print "saving norms to $normsName\n";
    foreach my $norm (@$normsMD) {
        my $class_id = $norm->{class_id};
        my $normalization = 1.0 / $norm->{norm};

        $norms{$class_id} = $normalization;
        printf $normsFile "$class_id F32 $normalization\n",
    }
    close $normsFile;
}

# Get list of imfile files
my $cmdflags;
my (@files);
{
    # dettool command to select imfile data for this exp_id
    $command  = "$dettool -residimfile";
    $command .= " -det_id $det_id";
    $command .= " -iteration $iter";
    $command .= " -exp_id $exp_id";
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -residimfile: $error_code\n");
        exit($error_code);
    }
    if (@$stdout_buf == 0) {
        &my_die("No imfiles were found", $det_id, $iter, $PS_EXIT_PROG_ERROR);
    }

    # Parse the stdout buffer into a metadata
    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $det_id, $iter, $exp_id, $PS_EXIT_PROG_ERROR);

    # since I can't figure out how to do input and output within PERL, I'm writing the (modified) metadata to a temp file
    my ($statFile, $statName) = tempfile( "/tmp/$exp_tag.detresid.$det_id.$iter.stats.XXXX", UNLINK => !$save_temps );

    print $statFile "rawResidImfile MULTI\n";

    # parse the file info in the metadata
    # as we parse the list of files and their stats, apply the normalization to the relevant fields
    # also, write out the modified metadata set
    foreach my $mdItem (@$metadata) {
        if ($mdItem->{class} ne "metadata") {
            carp "MD element ", $mdItem->{name}, " isn't of type METADATA --- ignored.\n";
            next;
        }
        my %hash;               # Hash element
        my $mdComponents = $mdItem->{value}; # Components of the metadata

        # determine the class_id for this block:
        my $class_id;
        foreach my $data (@$mdComponents) {
            unless ($data->{name} eq "class_id") { next; }
            $class_id = $data->{value};
            last;
        }

        # a new metadata block
        print $statFile "rawResidImfile  METADATA\n";

        # modify and save the data in this block:
        foreach my $data (@$mdComponents) {
            my $norm = 1.0;
            if ($det_mode eq "master") {
                $norm = $norms{$class_id};
            }

            # fields to modify by the normalization:
            if ($data->{name} eq "bg")            { $data->{value} *= $norm; }
            if ($data->{name} eq "bg_stdev")      { $data->{value} *= $norm; }
            if ($data->{name} eq "bg_mean_stdev") { $data->{value} *= $norm; }
            if ($data->{name} eq "bg_skewness")   { $data->{value} *= $norm; }
            if ($data->{name} eq "bg_kurtosis")   { $data->{value} *= $norm; }
            if ($data->{name} eq "bin_stdev")     { $data->{value} *= $norm; }

            # write out the metadata, save on the array of hashes
            print $statFile "  $data->{name}  $data->{type}  $data->{value}\n";
            $hash{$data->{name}} = $data->{value};
        }
        print $statFile "END\n";
        push @files, \%hash;
    }
    close $statFile;

    # parse the stats in the metadata file
    $command = "$ppStatsFromMetadata $statName - DETREND_RESID_EXP";
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
}

# outroot examples (HOST components must be set)
# file://data/ipp004.0/gpc1/20080130
# neb:///ipp004-v1/gpc1/20080130
# neb:///*/gpc1/20080130 (volume not specified)

# check for existing directory, generate if needed
$ipprc->outroot_prepare($outroot);

my $jpeg1Name  = $ipprc->filename("PPIMAGE.JPEG1", $outroot); # Binned JPEG #1
my $jpeg2Name  = $ipprc->filename("PPIMAGE.JPEG2", $outroot); # Binned JPEG #2

# XXX in debug mode, unlink = 0
my ($list1File, $list1Name) = tempfile( "/tmp/$exp_tag.detresid.$det_id.$iter.b1.list.XXXX", UNLINK => !$save_temps );
my ($list2File, $list2Name) = tempfile( "/tmp/$exp_tag.detresid.$det_id.$iter.b2.list.XXXX", UNLINK => !$save_temps );
foreach my $file (@files) {
    print $list1File ($ipprc->filename( "PPIMAGE.BIN1", $file->{path_base}, $file->{class_id} ) . "\n");
    print $list2File ($ipprc->filename( "PPIMAGE.BIN2", $file->{path_base}, $file->{class_id} ) . "\n");
}
close $list1File;
close $list2File;


# build the JPEG images
unless ($no_op) {
    # Make the jpeg for binning 1
    # XXX EAM : supply the collection of normalizations as a metadata
    $command = "$ppImage -list $list1Name $outroot"; # Command to run
    $command .= " -recipe PPIMAGE PPIMAGE_J1";
    $command .= " -recipe JPEG $recipe";
    $command .= " -normlist $normsName" if defined $normsName;
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run ppImage: $error_code", $det_id, $iter, $exp_id, $error_code);
    }
    &my_die("Unable to find expected output file: $jpeg1Name", $det_id, $iter, $exp_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($jpeg1Name);

    # Make the jpeg for binning 2
    # XXX EAM : supply the collection of normalizations as a metadata
    $command = "$ppImage -list $list2Name $outroot"; # Command to run
    $command .= " -recipe PPIMAGE PPIMAGE_J2";
    $command .= " -recipe JPEG $recipe";
    $command .= " -normlist $normsName" if defined $normsName;
    $command .= " -dbname $dbname" if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run ppImage: $error_code", $det_id, $iter, $exp_id, $error_code);
    }
    &my_die("Unable to find expected output file: $jpeg2Name", $det_id, $iter, $exp_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($jpeg2Name);
}

#### measure stats and reject if needed ####

# Rejection thresholds & related data
my $expected                = rejection_limit( 'EXPECTED',         $det_type, $filter );
my $reject_imfile_mean      = rejection_limit( 'IMFILE.MEAN',      $det_type, $filter );
my $reject_imfile_flux      = rejection_limit( 'IMFILE.FLUX',      $det_type, $filter );
my $reject_imfile_stdev     = rejection_limit( 'IMFILE.STDEV',     $det_type, $filter );
my $reject_imfile_skewness  = rejection_limit( 'IMFILE.SKEWNESS',  $det_type, $filter );
my $reject_imfile_kurtosis  = rejection_limit( 'IMFILE.KURTOSIS',  $det_type, $filter );
my $reject_imfile_meanstdev = rejection_limit( 'IMFILE.MEANSTDEV', $det_type, $filter );
my $reject_imfile_snr       = rejection_limit( 'IMFILE.SNR',       $det_type, $filter );
my $reject_imfile_bin_stdev = rejection_limit( 'IMFILE.BIN.STDEV', $det_type, $filter );
my $reject_imfile_bin_snr   = rejection_limit( 'IMFILE.BIN.SNR',   $det_type, $filter );
my $reject_exp_mean         = rejection_limit( 'EXP.MEAN',         $det_type, $filter );
my $reject_exp_flux         = rejection_limit( 'EXP.FLUX',         $det_type, $filter );
my $reject_exp_stdev        = rejection_limit( 'EXP.STDEV',        $det_type, $filter );
my $reject_exp_meanstdev    = rejection_limit( 'EXP.MEANSTDEV',    $det_type, $filter );
my $reject_exp_snr          = rejection_limit( 'EXP.SNR',          $det_type, $filter );
my $reject_exp_bin_stdev    = rejection_limit( 'EXP.BIN.STDEV',    $det_type, $filter );
my $reject_exp_bin_snr      = rejection_limit( 'EXP.BIN.SNR',      $det_type, $filter );

# storage array
my @fluxes;

foreach my $file (@files) {
    my $name      = $file->{class_id};
    my $mean      = $file->{bg};        # Mean for this imfile
    my $stdev     = $file->{bg_stdev}; # Stdev for this imfile
    my $skewness  = $file->{bg_skewness}; # Skewness for this imfile
    my $kurtosis  = $file->{bg_kurtosis}; # Kurtosis for this imfile
    my $meanStdev = $file->{bg_mean_stdev}; # Stdev of Means for this imfile
    my $binStdev  = $file->{bin_stdev}; # Binned Stdev for this imfile

    # calculate and save the fluxes
    my $flux;
    if ($file->{exp_time} == 0.0) {
        $flux = $mean;
    } else {
        $flux = $mean / $file->{exp_time};
    }
    push @fluxes, $flux;

    $mean -= $expected;

    last if $no_op;

    # reject exposure if, for any imfiles, the mean residual counts
    # deviate from the expected value by more than N sigma, (sigma =
    # total pixel variance).  this test is sensible for images which
    # should have a predictable nominal residual mean count value (eg,
    # 0.0 for a bias).   in general, use with ADDITIVE components
    if ($reject_imfile_mean > 0) {
        if (abs($mean) > $reject_imfile_mean * $stdev) {
            print "Rejecting exposure based on bad imfile mean for $name: ";
            $reject = 1;
        } else {
            print "Imfile mean for $name meets requirements: ";
        }
        print "$mean vs $reject_imfile_mean * $stdev\n";
    }  else {
        print "No rejection on imfile mean for $name\n";
    }

    # reject exposure if, for any imfiles, the mean residual flux
    # deviates from the expected value by more than N sigma, (sigma =
    # total pixel variance).  this test is sensible for images which
    # should have a predictable nominal residual flux value (eg, 0.0
    # for a bias).  in general, use with ADDITIVE components
    if ($reject_imfile_flux > 0) {
        if (abs($flux) > $reject_imfile_flux) {
            print "Rejecting exposure based on bad imfile flux for $name: ";
            $reject = 1;
        } else {
            print "Imfile flux for $name meets requirements: ";
        }
        print "$flux vs $reject_imfile_flux\n";
    }  else {
        print "No rejection on imfile flux for $name\n";
    }

    # reject exposure if, for any imfiles, the total pixel variance is
    # larger than the limit
    if ($reject_imfile_stdev > 0) {
        if ($stdev > $reject_imfile_stdev) {
            print "Rejecting exposure based on bad imfile stdev for $name: ";
            $reject = 1;
        } else {
            print "Imfile stdev for $name meets requirements: ";
        }
        print "$stdev vs $reject_imfile_stdev\n";

    } else {
        print "No rejection on imfile stdev for $name\n";
    }

    # reject exposure if, for any imfiles, the skewness is
    # larger than the limit
    if ($reject_imfile_skewness > 0) {
        if ($stdev > $reject_imfile_skewness) {
            print "Rejecting exposure based on bad imfile skewness for $name: ";
            $reject = 1;
        } else {
            print "Imfile skewness for $name meets requirements: ";
        }
        print "$skewness vs $reject_imfile_skewness\n";

    } else {
        print "No rejection on imfile skewness for $name\n";
    }

    # reject exposure if, for any imfiles, the kurtosis is
    # larger than the limit
    if ($reject_imfile_kurtosis > 0) {
        if ($stdev > $reject_imfile_kurtosis) {
            print "Rejecting exposure based on bad imfile kurtosis for $name: ";
            $reject = 1;
        } else {
            print "Imfile kurtosis for $name meets requirements: ";
        }
        print "$kurtosis vs $reject_imfile_kurtosis\n";

    } else {
        print "No rejection on imfile kurtosis for $name\n";
    }

    # reject exposure if, for any imfiles, the variance of the imfile
    # component means is larger than the limit
    if ($reject_imfile_meanstdev > 0) {
        if ($meanStdev > $reject_imfile_meanstdev) {
            print "Rejecting exposure based on bad imfile mean stdev for $name: ";
            $reject = 1;
        } else {
            print "Imfile mean stdev for $name meets requirements: ";
        }
        print "$meanStdev vs $reject_imfile_meanstdev\n";
    } else {
        print "No rejection on imfile mean stdev for $name\n";
    }

    # reject exposure if, for any imfiles, the signal-to-noise (ie,
    # the mean counts / total pixel variance) of the imfile component
    # means are less than the limit.  this test is sensible for images
    # which have finite residual flux such as a flat-field image.
    if ($reject_imfile_snr > 0) {
        if ($mean < $stdev * $reject_imfile_snr) {
            print "Rejecting exposure based on bad imfile S/N for $name: ";
            $reject = 1;
        } else {
            print "Imfile S/N for $name meets requirements: ";
        }
        print "mean: $mean vs stdev*SNlimit: " . $stdev * $reject_imfile_snr . "\n";
    } else {
        print "No rejection on imfile S/N for $name\n";
    }

    # reject exposure if, for any imfiles, the signal-to-noise of the
    # imfile component means, based on the stdev of the binned image
    # is less than the limit.  this test is sensible for images which
    # have finite residual flux such as a flat-field image.
    if ($reject_imfile_bin_stdev > 0) {
        if ($binStdev > $reject_imfile_bin_stdev) {
            print "Rejecting exposure based on bad imfile binned stdev for $name: ";
            $reject = 1;
        } else {
            print "Imfile binned stdev for $name meets requirements: ";
        }
        print "$binStdev vs $reject_imfile_bin_stdev\n";
    } else {
        print "No rejection on imfile binned stdev for $name\n";
    }
    if ($reject_imfile_bin_snr > 0) {
        if ($mean < $binStdev * $reject_imfile_bin_snr) {
            print "Rejecting exposure based on bad imfile binned S/N for $name: ";
            $reject = 1;
        } else {
            print "Imfile binned S/N for $name meets requirements: ";
        }
        print "mean: $mean vs binStdev*SNlimit: " . $binStdev * $reject_imfile_bin_snr . "\n";
    } else {
        print "No rejection on imfile binned S/N for $name\n";
    }
}

# basic ensemble stats
my $mean               = &value_for_flag ($cmdflags, "-bg");
my $meanStdev          = &value_for_flag ($cmdflags, "-bg_mean_stdev");
my $stdev              = &value_for_flag ($cmdflags, "-bg_stdev");
my $binStdev           = &value_for_flag ($cmdflags, "-bin_stdev");
my $fringe_mean        = &value_for_flag ($cmdflags, "-fringe_0");
my $fringe_err         = &value_for_flag ($cmdflags, "-fringe_1");
my $fringe_mean_stdev  = &value_for_flag ($cmdflags, "-fringe_2");
my $dfringe_mean       = &value_for_flag ($cmdflags, "-fringe_resid_0");
my $dfringe_err        = &value_for_flag ($cmdflags, "-fringe_resid_1");
my $dfringe_mean_stdev = &value_for_flag ($cmdflags, "-fringe_resid_2");

# other stats (flux depends on bg and exp_time)
my $fluxStats = Statistics::Descriptive::Sparse->new(); # Statistics calculator for means
$fluxStats->add_data(@fluxes);
my $flux = $fluxStats->mean();  # Mean of the imfile means

# other stats
my $exp_snr = 0.0;
if ($stdev > 0) { $exp_snr = $mean / $stdev; }

## Reject based on the exposure ensemble stats
# reject if the exposure ensemble mean is deviant
unless ($no_op) {
    print "Exposure mean $mean, stdev $stdev, mean stdev $meanStdev, exp s/n: $exp_snr\n";

    # reject exposure if the ensemble mean residual counts deviate
    # from the expected value by more than N sigma, (sigma = total
    # pixel variance).  this test is sensible for images which should
    # have a predictable nominal residual mean count value (eg, 0.0
    # for a bias).  in general, use with ADDITIVE components
    if ($reject_exp_mean > 0) {
        if (abs($mean) > $reject_exp_mean * $stdev) {
            print "Rejecting exposure based on bad mean counts: ";
            $reject = 1;
        } else {
            print "Exposure mean counts meets requirements: ";
        }
        print "mean: $mean, stdev: $stdev (limit is: " . $reject_exp_mean * $stdev . ") \n";
    } else {
        print "No rejection for exp mean\n";
    }

    # reject exposure if, for any imfiles, the mean residual flux
    # deviates from the expected value by more than N sigma, (sigma =
    # total pixel variance).  this test is sensible for images which
    # should have a predictable nominal residual flux value (eg, 0.0
    # for a bias).  in general, use with ADDITIVE components
    if ($reject_exp_flux > 0) {
        if (abs($flux) > $reject_exp_flux * $stdev) {
            print "Rejecting exposure based on bad mean flux: ";
            $reject = 1;
        } else {
            print "Exposure mean flux meets requirements: ";
        }
        print "flux: $flux, stdev: $stdev (limit is: " . $reject_exp_flux * $stdev . ")\n";
    } else {
        print "No rejection for exp mean\n";
    }

    # reject exposure if the total pixel variance is larger than the
    # limit
    if ($reject_exp_stdev > 0) {
        if ($stdev > $reject_exp_stdev) {
            print "Rejecting exposure based on bad stdev: ";
            $reject = 1;
        } else {
            print "Exposure stdev meets requirements: ";
        }
        print "$stdev vs $reject_exp_stdev\n";
    } else {
        print "No rejection for exp stdev\n";
    }

    # reject exposure if the variance of the imfile means is larger
    # than the limit
    if ($reject_exp_meanstdev > 0) {
        if ($meanStdev > $reject_exp_meanstdev) {
            print "Rejecting exposure based on bad mean stdev: ";
            $reject = 1;
        } else {
            print "Exposure mean stdev meets requirements: ";
        }
        print "$meanStdev vs $reject_exp_meanstdev\n";
    } else {
        print "No rejection for exp mean stdev\n";
    }

    # reject if the signal-to-noise is insufficient
    if ($reject_exp_snr > 0) {
        if (abs($mean) < abs($stdev * $reject_exp_snr)) {
            print "Rejecting exposure based on poor S/N: \n";
            $reject = 1;
        } else {
            print "Exposure S/N meets requirements: \n";
        }
        print "signal: $mean vs noise: $stdev (s/n limit is: $reject_exp_snr)\n";
    } else {
        print "No rejection for exp S/N\n";
    }
    # reject if the exposure ensemble stdev is deviant
    if ($reject_exp_bin_stdev > 0) {
        if ($binStdev > $reject_exp_bin_stdev) {
            print "Rejecting exposure based on bad binned stdev: ";
            $reject = 1;
        } else {
            print "Exposure binned stdev meets requirements: ";
        }
        print "$stdev vs $reject_exp_bin_stdev\n";
    } else {
        print "No rejection for exp binned stdev\n";
    }
    # reject if the signal-to-noise is insufficient
    if ($reject_exp_bin_snr > 0) {
        if (abs($mean) < abs($binStdev * $reject_exp_bin_snr)) {
            print "Rejecting exposure based on poor binned S/N: \n";
            $reject = 1;
        } else {
            print "Exposure binned S/N meets requirements: \n";
        }
        print "signal: $mean vs noise: $binStdev (s/n limit is: $reject_exp_bin_snr)\n";
    } else {
        print "No rejection for exp binned S/N\n";
    }
}

$command  = "$dettool -addresidexp -det_id $det_id -iteration $iter -exp_id $exp_id";
$command .= " -recip $recipe -path_base $outroot ";
$command .= ' -reject' if $reject;
$command .= " -dbname $dbname" if defined $dbname;
$command .= " $cmdflags";

unless ($no_update) {
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform dettool -addresidexp: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}


sub value_for_flag
{
    my $cmdflags = shift;
    my $flag = shift;

    my $value = 0.0;
    if ($cmdflags =~ m|$flag|) {
        ($value) = $cmdflags =~ m|$flag\s+(\S+)|;
    }
    $value;
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $det_id = shift;         # Detrend identifier
    my $iter = shift;           # Iteration
    my $exp_id = shift; # Exposure tag
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $det_id and defined $iter and defined $exp_id and not $no_update) {
        my $command = "$dettool -addresidexp";
        $command .= " -det_id $det_id";
        $command .= " -iteration $iter";
        $command .= " -exp_id $exp_id";
        $command .= " -path_base $outroot";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

# Retrieve the requested rejection limit, dying if not extant
sub rejection_limit
{
    my $name = shift;           # Rejection limit to
    my $type = shift;           # Type of exposure
    my $filter = shift;         # Filter

    my $value = $ipprc->rejection( $name, $det_type, $filter );
    if (not defined $value) {
        $filter = "(no filter)" if not defined $filter;
        die "Unable to determine $name rejection limit for $det_type with $filter.\n";
    }

    return $value;
}

__END__

####

## this function is not well though out.  it loads a collection of
## values (background mean and stdev) for a collection of imfile
## subcomponents.  it then measures the mean background, the
## background stdev (rms of ensemble stdevs), and the background mean
## stdev (rms of the means).  it then tries to reject based on the
## following criteria:
## (background / stdev > limit) for a single component
##    ---> this will ACCEPT for a poor images (large stdev)
## (stdev > limit)
## (mean background / stdev > limit) same problem
## (mean stdev > limit)

### I would suggest the following:
## 1) calculate the ensemble mean and stdev
## 2) for each component, is the (back - mean) / mean stdev > limit?
##    (ie, is it an outlier?)
## 3) is the component stdev > limit? (absolute test)
## 4) calculate the background stdev for the ensemble (excluding rejects)
## 5) is the mean stdev > limit (poor images)
## 6) calculate the ensemble stdev (rms) of remaining (excluding rejects)
## 7) is the ensemble stdev > limit

## for flats, the value used for stdev should be the fractional stdev
## (stdev/mean) and no rejection should be performed on the basis of
## the mean value, but could still be done on the mean stdev

## the same logical cuts above can be applied to components in an
## imfile, imfiles in an exposure, and exposures in a stack

### this function needs to avoid div by zero: compare Signal vs Noise*limit
