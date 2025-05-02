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
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use Digest::MD5::File qw( file_md5_hex );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $disttool   = can_run('disttool') or (warn "Can't find disttool" and $missing_tools = 1);
my $dist_make_bundle   = can_run('dist_bundle.pl') or (warn "Can't find dist_bundle.pl" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($dist_id, $camera, $stage, $stage_id, $component, $path_base, $chip_path_base, $clean, $alt_path_base);
my ($outdir, $run_state, $data_state, $magicked, $no_magic, $poor_quality, $exp_type, $rerun);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);

GetOptions(
           'dist_id=s'      => \$dist_id,    # distribution run identifier
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
           'exp_type=s'     => \$exp_type,
           'alt_path_base=s'=> \$alt_path_base,  # path to alternate inputs
           'outdir=s'       => \$outdir,     # "directory" for outputs
           'clean'          => \$clean,      # create clean distribution
           'rerun'          => \$rerun,      # recreate an existing component and update the database
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dist_id --camera --stage --stage_id --component --path_base --outdir",
           -exitval => 3) unless
    defined $dist_id and
    defined $camera and
    defined $stage and
    defined $stage_id and
    defined $component and
    defined $path_base and
    defined $outdir;

my $ipprc = PS::IPP::Config->new($camera); # IPP configuration
$ipprc->redirect_to_logfile($logfile) if $logfile;

my $temproot = metadataLookupStr($ipprc->{_siteConfig}, "TEMP.DIR");

$temproot = "/tmp" if !defined $temproot;

if (($stage eq 'raw') and !$clean and !(defined $chip_path_base and defined $exp_type)) {
    pod2usage( -msg => "Required options: --chip_path_base --exp_type for raw stage", -exitval => 3);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my ($rf, $rf_name) = tempfile("$temproot/bundleresults.$dist_id.$component.XXXX", UNLINK => !$save_temps);
close $rf;

my $basename = basename($path_base);
my $outroot = "$outdir/$basename";

my ($file_name, $bytes, $md5sum);
{
    my $command = "$dist_make_bundle --camera $camera --stage $stage --stage_id $stage_id";
    $command .= " --results_file $rf_name";
    $command .= " --component $component --path_base $path_base --outroot $outroot";
    $command .= " --chip_path_base $chip_path_base" if $chip_path_base;
    $command .= " --state $run_state" if defined $run_state;
    $command .= " --data_state $data_state" if defined $data_state;
    $command .= " --poor_quality" if defined $poor_quality;
    $command .= " --no_magic" if defined $no_magic;
    $command .= " --magicked" if defined $magicked;
    $command .= " --alt_path_base $alt_path_base" if defined $alt_path_base;
    $command .= " --exp_type $exp_type" if defined $exp_type;
    $command .= " --clean" if defined $clean;
    $command .= " --save-temps" if defined $save_temps;
    $command .= " --dbname $dbname" if defined $dbname;
    $command .= " --verbose" if defined $verbose;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $component, $outdir, $error_code);
    }

    open $rf, "<$rf_name" or &my_die("failed to open results file $rf_name", $dist_id, $component, $outdir, $PS_EXIT_UNKNOWN_ERROR);

    my @lines = (<$rf>);

    my $metadata = $mdcParser->parse (join "", @lines) or
        &my_die("Unable to parse metadata config doc", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);

    my $results = parse_md_list($metadata);

    if ((scalar @$results) != 1) {
        my $n = scalar @$results;
        &my_die("Unexected number of results from dist_make_bundle.pl: $n", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);
    }
    my $result = $results->[0];

    $file_name = $result->{name};
    &my_die("undefined file name from dist_bundle.pl", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR) unless defined $file_name;

    $bytes = $result->{bytes};
    &my_die("undefined file size from dist_bundle.pl", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR) unless defined $bytes;

    $md5sum = $result->{md5sum};
    &my_die("undefined file md5sum from dist_bundle.pl", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR) unless defined $md5sum;
}


{
    my $command;
    if (!$rerun) {
        $command = "$disttool -addprocessedcomponent";
    } else {
        $command = "$disttool -updateprocessedcomponent";
    }
    
    $command .= " -dist_id $dist_id -component $component -outdir $outdir";
    $command .= " -name $file_name -bytes $bytes -md5sum $md5sum";
    $command .= " -dbname $dbname" if defined $dbname;

    unless (defined $no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $dist_id, $component, $outdir, $error_code);
        }
    } else {
        print "skipping command $command\n";
    }
}

exit 0;

### Pau.
sub my_die
{
    my $msg = shift;            # Warning message on die
    my $dist_id = shift;        # distRun.dist_id
    my $component = shift;      # class_id, skycell_id, or exposure
    my $outdir = shift;         # output directory
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    my $command;
    if (!$rerun) {
        $command = "$disttool -addprocessedcomponent";
    } else {
        $command = "$disttool -updateprocessedcomponent";
    }
    $command   .= " -dist_id $dist_id";
    $command   .= " -component $component";
    $command   .= " -outdir $outdir";
    $command   .= " -fault $exit_code";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            carp("failed to update database for $dist_id $component");
        }
    } else {
        print "Skipping command: $command\n";
    }

    carp($msg);
    exit $exit_code;
}

__END__
