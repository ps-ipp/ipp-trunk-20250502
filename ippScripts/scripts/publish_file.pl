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
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use Carp;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $pubtool = can_run('pubtool') or (warn "Can't find pubtool" and $missing_tools = 1);
my $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $regtool = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
my $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $ppMops = can_run('ppMops') or (warn "Can't find ppMops" and $missing_tools = 1);
my $ppMonet = can_run('ppMonet') or (warn "Can't find ppMonet" and $missing_tools = 1);
my $dsreg = can_run('dsreg') or (warn "Can't find dsreg" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ( $pub_id, $camera, $stage, $stage_id, $fileset, $format, $product, $workdir, $need_magic );
my ( $dbname, $verbose, $no_update, $no_op, $save_temps, $redirect );
my ( $output_format, $difftype );

GetOptions(
    'pub_id=s'          => \$pub_id, # Publish identifier
    'camera=s'          => \$camera, # Camera name
    'stage=s'           => \$stage,       # Stage of interest
    'stage_id=s'        => \$stage_id,    # Stage identifier
    'product=s'         => \$product,     # Datastore product name
    'fileset=s'         => \$fileset,     # Fileset name
    'workdir=s'         => \$workdir,     # Working directory
    'need-magic'        => \$need_magic,  # do we require censored detections?
    'dbname=s'          => \$dbname,    # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op, # Don't do any operations
    'save-temps'        => \$save_temps, # Save temporary files?
    'redirect-output'   => \$redirect,   # Redirect output to log file?
    'output_format=i'   => \$output_format, # Output format for ppMops
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --pub_id --camera --stage --stage_id --product --workdir",
           -exitval => $PS_EXIT_CONFIG_ERROR) unless
    defined $pub_id and
    defined $camera and
    defined $product and
    defined $stage and
    defined $stage_id and
    defined $workdir and
    defined $dbname;

my $outroot = "$workdir/$product.$pub_id"; # Output root name

my $ipprc = PS::IPP::Config->new( $camera ) or
    &my_die( "Unable to set up", $pub_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

$ipprc->outroot_prepare( $outroot );
my $logDest = "$outroot.log";
$ipprc->redirect_output($logDest) or &my_die( "Unable to redirect output", $pub_id, $PS_EXIT_SYS_ERROR ) if $redirect;

my $mdcParser = PS::IPP::Metadata::Config->new;

my $mops = ($product =~ /^IPP-MOPS/ ? 1 : 0); # Format for MOPS?
my ($dsFile, $dsFileName) = tempfile("/tmp/publish.$pub_id.ds.XXXX", UNLINK => !$save_temps );
my $dsType = $mops ? "IPP-MOPS" : $product; # Type for DataStore

my $comment;                    # Comment for exposure
my $exp_name_1;                    # Name of exposure 1
my $exp_name_2;                    # Name of exposure 2
if ($stage eq 'camera') {
    my $command =  "camtool -processedexp -cam_id $stage_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die( "Unable to retrieve filename", $pub_id, $PS_EXIT_SYS_ERROR) unless $success;

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config", $pub_id, $PS_EXIT_PROG_ERROR);

    my $components = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $pub_id, $PS_EXIT_PROG_ERROR);

    &my_die("More than one entry for cam_id $stage_id", $pub_id, $PS_EXIT_PROG_ERROR) if scalar @$components > 1;

    my $comp = $$components[0]; # Component of interest
    my $path_base = $comp->{path_base}; # Base name for file
    my $file = $ipprc->filename( "PSASTRO.OUTPUT", $path_base );
    $file = $ipprc->file_resolve($file);
    my $exp_name = $comp->{exp_name};
    my $exp_id = $comp->{exp_id};
    my $chip_id = $comp->{chip_id};
    my $cam_id = $comp->{cam_id};
    my $zp = $comp->{zpt_obs};
    my $zp_err = $comp->{zpt_stdev};
    my $astrom = sqrt($comp->{sigma_ra}**2 + $comp->{sigma_dec}**2);
    my $name = "cam_$cam_id";
    $comment = $comp->{comment};

    if ($product eq "MONET") {
        my $output = $ipprc->file_resolve( "$outroot.csv.gz", 'create' ) or
            &my_die( "Unable to resolve output file", $pub_id, $PS_EXIT_SYS_ERROR);

        my $command = "$ppMonet $file $output";
        $command .= " -exp_name " . $exp_name if defined $exp_name;
        $command .= " -exp_id " . $exp_id if defined $exp_id;
        $command .= " -chip_id " . $chip_id if defined $chip_id;
        $command .= " -cam_id " . $cam_id if defined $cam_id;
        $command .= " -zp " . $zp if defined $zp;
        $command .= " -zp_error " . $zp_err if defined $zp_err;
        $command .= " -astrom_rms " . $astrom if defined $astrom;

        unless ($no_op) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            &my_die( "Unable to translate", $pub_id, $PS_EXIT_SYS_ERROR) unless $success;
            &my_die( "Unable to find translated file $output", $pub_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists( $output );
        } else {
            print "Not running: $command\n";
        }

        $file = $output;
    }
    print $dsFile "$file|||$product|$name|\n";

} elsif ($stage eq 'diff' or $stage eq 'diffphot') {
    my $command;                # Command to run
    if ($stage eq 'diff') {
        $command = "difftool -diffskyfile -diff_id $stage_id";
    } elsif ($stage eq 'diffphot') {
        $command = "diffphottool -data -diff_phot_id $stage_id";
    }
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die( "Unable to retrieve filename", $pub_id, $PS_EXIT_SYS_ERROR) unless $success;

    my $components = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config", $pub_id, $PS_EXIT_PROG_ERROR);

    my ($mopsPositiveFile, $mopsPositiveFileName) = tempfile("/tmp/publish.$pub_id.mops.pos.XXXX", UNLINK => !$save_temps ) if $mops;
    my ($mopsNegativeFile, $mopsNegativeFileName) = tempfile("/tmp/publish.$pub_id.mops.neg.XXXX", UNLINK => !$save_temps ) if $mops;

    my %positive;               # Data for positive diff detections
    my %negative;               # Data for negative diff detections
    foreach my $comp ( @$components ) {
        my $path_base = $comp->{path_base}; # Base name for file
        if (!$need_magic and $comp->{magicked}) {
            # This client is authorized to receive uncensored detections
            # Get the uri for the "backup" files
            print "Using uncensored input from $path_base\n";
            $path_base = $ipprc->destreaked_filename($path_base);
        }
        next if defined $comp->{quality} and $comp->{quality} > 0;
        print "Warning: mis-matched comments\n" if defined $comment and $comment ne $comp->{comment};
        $comment = $comp->{comment} unless defined $comment;
        $exp_name_1 = $comp->{exp_name_1} unless defined $exp_name_1;
        $exp_name_2 = $comp->{exp_name_2} unless defined $exp_name_2;

        (carp "Bad zpt_obs or exp_time for component" and next) if not defined $comp->{zpt_obs} or not defined $comp->{exp_time};
        my $zp = $comp->{zpt_obs} + 2.5 * log($comp->{exp_time}) / log(10);
        my $astrom = sqrt($comp->{sigma_ra_1}**2 + $comp->{sigma_dec_1}**2);

        my $skycell_id = $comp->{skycell_id};
        my $filename;
        if ($stage eq 'diff') {
            $filename = $ipprc->filename( "PPSUB.OUTPUT.SOURCES", $path_base );
        } elsif ($stage eq 'diffphot') {
            $filename = $ipprc->filename( "PSPHOT.OUT.CMF.MEF", "$path_base.pos" );
        }
        &my_die("input file does not exist: $filename", $pub_id, $PS_EXIT_SYS_ERROR) if !$ipprc->file_exists($filename);
        my $resolved = $ipprc->file_resolve($filename);

        &my_die("unable to resolve input file: $filename", $pub_id, $PS_EXIT_SYS_ERROR) if !$resolved;

        $filename = $resolved;

        my $cam_id = $comp->{cam_id_1};
        #print "Getting info from camera stage $cam_id\n";
        my $cam_command =  "$camtool -processedexp -cam_id $cam_id";
        $cam_command .= " -dbname $dbname" if defined $dbname;
        my ( $cam_success, $cam_error_code, $cam_full_buf, $cam_stdout_buf, $cam_stderr_buf ) =
            run(command => $cam_command, verbose => $verbose);
        &my_die( "Unable to retrieve filename", $pub_id, $PS_EXIT_SYS_ERROR) unless $cam_success;
        my $cam_metadata = $mdcParser->parse(join "", @$cam_stdout_buf) or
            &my_die("Unable to parse metadata config", $pub_id, $PS_EXIT_PROG_ERROR);
        my $cam_components = parse_md_list($cam_metadata) or
            &my_die("Unable to parse metadata list", $pub_id, $PS_EXIT_PROG_ERROR);
        &my_die("More than one entry for cam_id $stage_id", $pub_id, $PS_EXIT_PROG_ERROR) if scalar @$cam_components > 1;
        my $cam_comp = $$cam_components[0];

        my $exp_id = $comp->{exp_id_1};
        #print "Getting info from raw stage $exp_id\n";
        my $reg_command =  "$regtool -processedimfile -exp_id $exp_id";
        $reg_command .= " -dbname $dbname" if defined $dbname;
        my ( $reg_success, $reg_error_code, $reg_full_buf, $reg_stdout_buf, $reg_stderr_buf ) =
            run(command => $reg_command, verbose => $verbose);
        &my_die( "Unable to retrieve filename", $pub_id, $PS_EXIT_SYS_ERROR) unless $reg_success;
        my $reg_metadata = $mdcParser->parse(join "", @$reg_stdout_buf) or
            &my_die("Unable to parse metadata config", $pub_id, $PS_EXIT_PROG_ERROR);
        my $reg_components = parse_md_list($reg_metadata) or
            &my_die("Unable to parse metadata list", $pub_id, $PS_EXIT_PROG_ERROR);
        my $reg_comp = $$reg_components[0];

        # Now get the difftype value from diff->{diff_mode}
        my $difftype = "";
        if ($comp->{diff_mode} == 1) {
            $difftype = "WW";
        } elsif ($comp->{diff_mode} == 2) {
            $difftype = "WS";
        } elsif ($comp->{diff_mode} == 3) {
            $difftype = "SW"; # Not used yet
        } elsif ($comp->{diff_mode} == 4) {
            $difftype = "SS";
        } else {
            $difftype = "Unsupported diff_mode value: [" . $comp->{diff_mode} . "]";
        }

        my $data = { zp => $zp,
                     zp_err => $comp->{zpt_stdev},
                     astrom => sqrt($comp->{sigma_ra_1}**2 + $comp->{sigma_dec_1}**2),
                     exp_name => $comp->{exp_name_1},
                     exp_id => $comp->{exp_id_1},
                     chip_id => $comp->{chip_id_1},
                     cam_id => $comp->{cam_id_1},
                     fake_id => $comp->{fake_id_1},
                     warp_id => $comp->{warp1},
                     diff_id => $comp->{diff_id},
                     camera => $comp->{camera},
                     output_format => $comp->{output_format},
                     direction => 1,
                     comment => $reg_comp->{comment},
                     obsmode => $reg_comp->{obs_mode},
                     difftype => $difftype,
                     sky => $cam_comp->{bg},
                     shutoutc => $reg_comp->{dateobs},
        };

        #warn("Checking for positive");
        diff_check(\%positive, $data, "positive");

        if ($mops) {
            print $mopsPositiveFile "$filename\n";
        } else {
            print $dsFile "$filename|||$product|${skycell_id}.pos|\n";
        }

        # Negative direction
        if (defined $comp->{bothways} and $comp->{bothways}) {
            my $filename;
            if ($stage eq 'diff') {
                $filename = $ipprc->filename( "PPSUB.INVERSE.SOURCES", $path_base );
            } elsif ($stage eq 'diffphot') {
                $filename = $ipprc->filename( "PSPHOT.OUT.CMF.MEF", "$path_base.neg" );
            }

            $filename = $ipprc->file_resolve($filename);

            my $data = { zp => $zp,
                         zp_err => $comp->{zpt_stdev},
                         astrom => sqrt($comp->{sigma_ra_2}**2 + $comp->{sigma_dec_2}**2),
                         exp_name => $comp->{exp_name_2},
                         exp_id => $comp->{exp_id_2},
                         chip_id => $comp->{chip_id_2},
                         cam_id => $comp->{cam_id_2},
                         fake_id => $comp->{fake_id_2},
                         warp_id => $comp->{warp2},
                         diff_id => $comp->{diff_id},
			 camera => $comp->{camera},
			 # missing output_format?
                         direction => 0,
                         comment => $reg_comp->{comment},
                         obsmode => $reg_comp->{obs_mode},
                         difftype => $difftype,
                         sky => $cam_comp->{bg},
                         shutoutc => $reg_comp->{dateobs},
            };

            #warn("Checking for negative");
            diff_check(\%negative, $data, "negative");

            if ($mops) {
                print $mopsNegativeFile "$filename\n";
            } else {
                print $dsFile "$filename|||$dsType|${skycell_id}.neg|\n";
            }
        }
    }

    close $mopsPositiveFile if $mops;
    close $mopsNegativeFile if $mops;

    if ($mops) {
        if (scalar keys %positive > 0) {
            my $output = mops_combine(\%positive, "$outroot.pos.mops", $mopsPositiveFileName);
            print $dsFile "$output|||$dsType|positive|\n";
        }
        if (scalar keys %negative > 0) {
            my $output = mops_combine(\%negative, "$outroot.neg.mops", $mopsNegativeFileName);
            print $dsFile "$output|||$dsType|negative|\n";
        }
    }
}

close $dsFile;

unless ($no_update) {
    my $command = "$dsreg --add pub.$pub_id.$stage.$stage_id --link --abspath --product $product --type $dsType --list $dsFileName";
    $command .= " --ps0 \"$comment\"" if defined $comment;
    $command .= " --ps1 \"$exp_name_1\"" if defined $exp_name_1;
    $command .= " --ps2 \"$exp_name_2\"" if defined $exp_name_2;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die( "Unable to register with data store", $pub_id, $PS_EXIT_SYS_ERROR) unless $success;
}

unless ($no_update) {
    my $command = "$pubtool -add -pub_id $pub_id -path_base $outroot";
    $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
    $command .= " -hostname $host" if defined $host;

    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die( "Unable to register with data store", $pub_id, $PS_EXIT_SYS_ERROR) unless $success;
}

### Pau.

# Check inputs for a diff
sub diff_check
{
    my $data = shift;           # Data hash
    my $comp = shift;           # Component data
    my $name = shift;           # Name of component

    $data->{zp}        = $comp->{zp}        unless defined $data->{zp};
    $data->{zp_err}    = $comp->{zp_err}    unless defined $data->{zp_err};
    $data->{astrom}    = $comp->{astrom}    unless defined $data->{astrom};
    $data->{exp_name}  = $comp->{exp_name}  unless defined $data->{exp_name};
    $data->{exp_id}    = $comp->{exp_id}    unless defined $data->{exp_id};
    $data->{chip_id}   = $comp->{chip_id}   unless defined $data->{chip_id};
    $data->{cam_id}    = $comp->{cam_id}    unless defined $data->{cam_id};
    $data->{fake_id}   = $comp->{fake_id}   unless defined $data->{fake_id};
    $data->{warp_id}   = $comp->{warp_id}   unless defined $data->{warp_id};
    $data->{diff_id}   = $comp->{diff_id}   unless defined $data->{diff_id};
    $data->{camera}    = $comp->{camera}    unless defined $data->{camera};
    # missing output_format?
    $data->{direction} = $comp->{direction} unless defined $data->{direction};
    $data->{comment}   = $comp->{comment}   unless defined $data->{comment};
    $data->{obsmode}   = $comp->{obsmode}   unless defined $data->{obsmode};
    $data->{difftype}  = $comp->{difftype}  unless defined $data->{difftype};
    $data->{sky}       = $comp->{sky}       unless defined $data->{sky};
    $data->{shutoutc}  = $comp->{shutoutc}  unless defined $data->{shutoutc};

    &my_die("zp value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{zp} and $comp->{zp} != $data->{zp};
    &my_die("zp_err value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{zp_err} and $comp->{zp_err} != $data->{zp_err};
    &my_die("astrom value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{astrom} and $comp->{astrom} != $data->{astrom};
    &my_die("exp_name value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{exp_name} and $comp->{exp_name} ne $data->{exp_name};
    &my_die("exp_id value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{exp_id} and $comp->{exp_id} != $data->{exp_id};
    &my_die("chip_id value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{chip_id} and $comp->{chip_id} != $data->{chip_id};
    &my_die("cam_id value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{cam_id} and $comp->{cam_id} != $data->{cam_id};
    &my_die("fake_id value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{fake_id} and $comp->{fake_id} != $data->{fake_id};
    &my_die("warp_id value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{warp_id} and $comp->{warp_id} != $data->{warp_id};
    &my_die("diff_id value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{diff_id} and $comp->{diff_id} != $data->{diff_id};
    &my_die("camera value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{camera} and $comp->{camera} ne $data->{camera};
    &my_die("direction value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{direction} and $comp->{direction} != $data->{direction};
    &my_die("comment value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{comment} and $comp->{comment} ne $data->{comment};
    &my_die("obsmode value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{obsmode} and $comp->{obsmode} ne $data->{obsmode};
    &my_die("difftype value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{difftype} and $comp->{difftype} ne $data->{difftype};
    &my_die("sky value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{sky} and $comp->{sky} != $data->{sky};
    &my_die("shutoutc value for $name doesn't match", $pub_id, $PS_EXIT_SYS_ERROR) if defined $data->{shutoutc} and $comp->{shutoutc} ne $data->{shutoutc};

    return 1;
}

# Combine multiple files for MOPS
sub mops_combine
{
    my $data = shift;           # Data
    my $output = shift;         # Output name
    my $input = shift;          # Input name

    $output = $ipprc->file_resolve( $output, 'create' ) or
        &my_die( "Unable to resolve output file $output", $pub_id, $PS_EXIT_SYS_ERROR);

    my $command = "$ppMops $input $output";
    $command .= " -exp_name " . $data->{exp_name} if defined $data->{exp_name};
    $command .= " -exp_id " . $data->{exp_id} if defined $data->{exp_id};
    $command .= " -chip_id " . $data->{chip_id} if defined $data->{chip_id};
    $command .= " -cam_id " . $data->{cam_id} if defined $data->{cam_id};
    $command .= " -fake_id " . $data->{fake_id} if defined $data->{fake_id};
    $command .= " -warp_id " . $data->{warp_id} if defined $data->{warp_id};
    $command .= " -diff_id " . $data->{diff_id} if defined $data->{diff_id};
    $command .= " -camera " . $data->{camera} if defined $data->{camera};
    $command .= " -inverse" if defined $data->{direction} and $data->{direction} == 0;
    $command .= " -zp " . $data->{zp} if defined $data->{zp};
    $command .= " -zp_error " . $data->{zp_err} if defined $data->{zp_err};
    $command .= " -astrom_rms " . $data->{astrom} if defined $data->{astrom};
    $command .= " -version " . $data->{output_format} if defined $data->{output_format};
    $command .= " -comment \"" . $data->{comment} . "\"" if defined $data->{comment};
    $command .= " -obsmode \"" . $data->{obsmode} . "\"" if defined $data->{obsmode};
    $command .= " -difftype " . $data->{difftype} if defined $data->{difftype};
    $command .= " -sky " . $data->{sky} if defined $data->{sky};
    $command .= " -shutoutc \"" . $data->{shutoutc} . "\"" if defined $data->{shutoutc};

    unless ($no_op) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die( "Unable to translate", $pub_id, $PS_EXIT_SYS_ERROR) unless $success;
        &my_die( "Unable to find translated file $output", $pub_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists( $output );
    } else {
        print "Not running: $command\n";
    }

    return $output;
}


sub my_die
{
    my $msg = shift;            # Exit message
    my $pub_id = shift;         # Publish run identifier
    my $fault = shift;          # Fault code

    $fault = $PS_EXIT_PROG_ERROR unless defined $fault;

    carp($msg);
    if (defined $pub_id and not $no_update) {
        my $command = "$pubtool -add";
        $command .= " -pub_id $pub_id";
        $command .= " -path_base $outroot";
        $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        $command .= " -hostname $host" if defined $host;
        $command .= " -fault $fault";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
    }
    exit $fault;
}


__END__
