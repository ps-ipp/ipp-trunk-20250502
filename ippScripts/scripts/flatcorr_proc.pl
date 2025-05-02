#!/usr/bin/env perl

## USAGE:flatcorr_proc.pl --dbname --corr_id

## this script does the following steps:

# extract the details of the flatcorr run: dvodb, filter, camera, etc?

# relphot -D CATDIR $dvodb -grid (outgrid.fits) (filter) -region 0 360 -90 90 (other parameters?)

# dvoMakeCorr -file outgrid.fits -ref ref.fits outcorr

# dettool -register -det_type FLATCORR -filelevel (level) -workdir -inst, etc

# foreach $imfile ()
#   dettool -register_imfile -uri, etc, etc

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

use Storable qw(freeze thaw);
use File::Basename qw( basename);
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration
use File::Spec;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ($corr_id, $det_type, $dvodb, $camera, $region, $filter, $dbname, $workdir, $make_correction, $verbose, $no_update, $no_op);
GetOptions(
    'corr_id|i=s'      => \$corr_id,
    'det_type|d=s'     => \$det_type,
    'dvodb|c=s'        => \$dvodb,
    'camera=s'         => \$camera,
    'region|r=s'       => \$region,
    'filter|f=s'       => \$filter,
    'dbname|d=s'       => \$dbname,# Database name
    'workdir|w=s'      => \$workdir, # Working directory for output files
    'make_correction'  => \$make_correction,   # Generate the correction image and save to the detrend database
    'verbose'          => \$verbose,   # Print to stdout
    'no-update'        => \$no_update,
    'no-op'            => \$no_op,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --corr_id --dvodb --camera --region --filter --workdir",
           -exitval => 3) unless
    defined $corr_id and
    defined $dvodb and
    defined $det_type and
    defined $camera and
    defined $region and
    defined $workdir and
    defined $filter;

# Look for programs we need
my $missing_tools;
my $relphot     = can_run('relphot')      or (warn "Can't find relphot"      and $missing_tools = 1);
my $addstar     = can_run('addstar')      or (warn "Can't find addstar"      and $missing_tools = 1);
my $dvoMakeCorr = can_run('dvoMakeCorr')  or (warn "Can't find dvoMakeCorr"  and $missing_tools = 1);
my $detselect   = can_run('detselect')    or (warn "Can't find detselect"    and $missing_tools = 1);
my $dettool     = can_run('dettool')      or (warn "Can't find dettool"      and $missing_tools = 1);
my $flatcorr    = can_run('flatcorr')     or (warn "Can't find flatcorr"     and $missing_tools = 1);

if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR);
}

$ipprc->define_camera($camera);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# dvodb must be non-nebulous; workdir may be in nebulous
my $outgrid = "$dvodb/flatcorr/$camera.$filter.$corr_id";
my $outcorr = "$workdir/$camera.$filter.$corr_id";

# check for existing directory, generate if needed
print "preparing $outcorr";
$ipprc->outroot_prepare($outcorr);

if (not -e "$dvodb/flatcorr") {
    mkdir "$dvodb/flatcorr" or &my_die ("Unable to make output directory for relphot $dvodb/flatcorr", $corr_id, 3);
}

# parse the region (RAs,RAe:DECs,DECe) : item = +/-NNN.NNNN
my @coords = split (":", $region);
my ($RAs, $RAe) = split (",", $coords[0]);
my ($DECs, $DECe) = split (",", $coords[1]);

# Run addstar -resort to ensure the db is indexed
# XXX addstar should be able to recognize and skip indexed tables
{
    my $command = "$addstar -resort";
    $command .= " -D CATDIR $dvodb";
    $command .= " -region $RAs $RAe $DECs $DECe";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die ("Unable to perform addstar -resort for dvodb $dvodb on region $region: $error_code", $corr_id, $error_code);
    }
}

# Run relphot (filter) for the specified region (need to clarify the options like imfreeze)
# need to convert the camera name to the dvo camera value
{
    my $camdir = $ipprc->dvo_cameradir(); # Camera directory for addstar

    my $command = "$relphot $filter";
    $command .= " -D CAMERA $camdir";
    $command .= " -D CATDIR $dvodb";
    $command .= " -region $RAs $RAe $DECs $DECe";
    $command .= " -outroot $outgrid";
    $command .= " -imfreeze -reset -grid -mosaic";
    # XXX the -mosaic option should be recipe-selected (only valid for multichip cameras)
    # XXX update the catdir after the analysis is done?

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die ("Unable to perform relphot -grid for dvodb $dvodb on region $region: $error_code", $corr_id, $error_code);
    }
}

if ($make_correction) {

    # get a single input exposure
    # flatcorr -inputexp -corr_id $corr_id -limit 1
    my $chip_id = &get_chip_id();

    # get the list of imfiles for the single input exposure
    # flatcorr -inputimfile -chip_id $chip_id
    my $files = &get_imfiles($chip_id);

    # set up the detrend run to store the corrected imfiles
    my $det_id = &get_det_id($$files[0]);

    # make the (full-sized) detrend correction images
    &make_detrend_imfiles($det_id, $files);

    # set the detrun state to 'stop'
    my $command = "$dettool -updatedetrun";
    $command .= " -det_id $det_id";
    $command .= " -state stop";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn ("Unable to perform dettool -updatedetrun: $error_code");
        exit($error_code);
    }
}

# Push the results into the database
{
    my $command = "$flatcorr -addprocess";
    $command .= " -corr_id $corr_id";
    $command .= " -hostname $host" if defined $host;
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_update) {

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn ("Unable to perform regtool -addprocessedimfile: $error_code");
            exit($error_code);
        }
    } else {
        print "skipping command: $command\n";
    }
}

# get a single input exposure
# flatcorr -inputexp -corr_id $corr_id -limit 1
sub get_chip_id {

    my $command = "$flatcorr -inputexp";
    $command .= " -corr_id $corr_id";
    $command .= " -limit 1";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform camtool: $error_code", $corr_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $corr_id, $PS_EXIT_PROG_ERROR);

    # extract the metadata for the files into a hash list
    my $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $corr_id, $PS_EXIT_PROG_ERROR);

    # check for existence
    my $file = $$files[0];
    my $chip_id = $file->{chip_id};

    return $chip_id;
}

# get the list of imfiles for the single input exposure
# flatcorr -inputimfile -chip_id $chip_id
sub get_imfiles {
    my $chip_id = shift;

    my $command = "$flatcorr -inputimfile";
    $command .= " -chip_id $chip_id";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform camtool: $error_code", $corr_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $corr_id, $PS_EXIT_PROG_ERROR);

    # extract the metadata for the files into a hash list
    my $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $corr_id, $PS_EXIT_PROG_ERROR);

    return $files;
}

# set up the detrend run to store the corrected imfiles
sub get_det_id {
    # get the filelevel from the supplied chip
    my $file = shift;

    my $filelevel = $file->{filelevel};
    my $telescope = $file->{telescope};

    my $command = "$dettool -register_detrend";
    $command .= " -det_type $det_type";
    $command .= " -filelevel $filelevel";
    $command .= " -workdir $workdir";
    $command .= " -inst $camera";
    $command .= " -telescope $telescope";
    $command .= " -filter $filter";
    $command .= " -dbname $dbname" if defined $dbname;

    ## the flat-field correction is valid for any airmass, exptime, solangle
    ## for now, we assume the posangle and ccd_temp are not important
    ## XXX someone needs to set the use_begin, use_end values??
    ## XXX inherit a label from the flatcorrRun?

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die ("Unable to register new detrend: $error_code", $corr_id, $PS_EXIT_PROG_ERROR);
    }

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $corr_id, $PS_EXIT_PROG_ERROR);

    # extract the metadata for the files into a hash list
    my $output = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $corr_id, $PS_EXIT_PROG_ERROR);

    # $file = $$output[0];
    my $det_id = $$output[0]->{det_id};
    return $det_id;
}

# use input chip images as reference images to make the detrend correction images
sub make_detrend_imfiles {
    my $det_id = shift;
    my $files = shift;

    foreach my $file (@$files) {
        # create the detrend correction for the imfiles based on the input imfiles
        my $reffile = $file->{uri};
        my $class_id = $file->{class_id};

        my $uri = $ipprc->filename("DVOCORR.OUTPUT", $outcorr, $class_id);
        unless ($uri) {
            &my_die ("Unable to find DVOCORR.OUTPUT in filerules", $corr_id, $PS_EXIT_PROG_ERROR);
        }

        my $command = "$dvoMakeCorr $outcorr";
        $command .= " -file $outgrid.fits";
        $command .= " -ref $reffile";

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);

        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die ("Unable to perform dvoMakeCorr: $error_code", $corr_id, $PS_EXIT_PROG_ERROR);
        }

        # register the detrend correction imfile
        $command = "$dettool -register_detrend_imfile";
        $command .= " -det_id $det_id";
        $command .= " -class_id $class_id";
        $command .= " -uri $uri";
        $command .= " -dbname $dbname" if defined $dbname;

        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);

        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die ("Unable to register new detrend: $error_code", $corr_id, $PS_EXIT_PROG_ERROR);
        }
    }
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $corr_id    = shift;
    my $exit_code  = shift;

    carp($msg);
    if (not $no_update) {
        my $command = "$flatcorr -addprocess";
        $command .= " -corr_id $corr_id";
        $command .= " -fault $exit_code";
        $command .= " -hostname $host" if defined $host;
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

# Pau.

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}

__END__
