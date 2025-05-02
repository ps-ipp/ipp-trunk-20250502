#! /usr/bin/env perl
#
#
# Run a detectability query job
#
#

use strict;
use warnings;

use Sys::Hostname;
use Carp;
use File::Basename;
use File::Copy;
use IPC::Cmd 0.36 qw( can_run run );
use Getopt::Long qw( GetOptions );
use Pod::Usage qw( pod2usage );
use File::Temp qw( tempfile tempdir);

use PS::IPP::PStamp::RequestFile qw( :standard );
use PS::IPP::PStamp::Job qw( :standard );
use PS::IPP::Config qw( :standard );
use PS::IPP::Metadata::List qw( parse_md_list );

use Astro::FITS::CFITSIO qw( :constants );
Astro::FITS::CFITSIO::PerlyUnpacking(1);

#
# Set up
#
my $host = hostname();

my $EXTVER = 1.0;
my $EXTNAME = 'MOPS_DETECTABILITY_RESPONSE';
my ($req_id,$job_id,$req_name,$product,$need_magic,$missing_tools,$project);
my ($output_base,$dbname,$dbserver,$verbose,$save_temps);
GetOptions(
    'output_base=s'   =>      \$output_base,
    'job_id=s'        =>      \$job_id,
    'dbname=s'        =>      \$dbname,
    'dbserver=s'      =>      \$dbserver,
    'verbose'         =>      \$verbose,
    'save-temps'      =>      \$save_temps,
    ) or pod2usage(2);

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --job_id --output_base",
	   -exitval => 3,
    ) unless
    defined $job_id and defined $output_base;

my $psphotForced      = can_run('psphotForced') or (warn "Can't find psphotForced" and $missing_tools = 1);
my $ppCoord           = can_run('ppCoord') or (warn "Can't find ppCoord" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $workdir = dirname($output_base);
my $params_file = "${output_base}params.mdc";

unless (defined $verbose) {
    $verbose = 0;
}

my $ipprc = PS::IPP::Config->new();
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

if (!$dbserver) {
    $dbserver = metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

my $imagedb = 'gpc1';
my %args;

open IN, "<$params_file" 
    or my_die("failed to open params file: $params_file", "", "", "", "", "", "", $PS_EXIT_UNKNOWN_ERROR);

my $data = $mdcParser->parse(join "", (<IN>))
    or my_die("failed to parse paramsfile: $params_file", "", "", "", "", "", "", $PS_EXIT_UNKNOWN_ERROR);
my $refs =  parse_md_list($data);
if (scalar @$refs < 2) {
    my_die("invalid paramsfile: $params_file", "", "", "", "", "", "", $PS_EXIT_UNKNOWN_ERROR);
}

my $params = shift @$refs;
my @rows = @$refs;

my $nRows = scalar @rows;
print "Read $nRows rows from $params_file\n";
    
#exit 0;

# run ppCoord and psphotForced to calculate the required data.
my $fpa_id = $params->{FPA_ID};
my $fault = $params->{FAULT};
my $catalog = $params->{CATALOG};
my $psf   = $params->{PSF};
my $image  = $params->{IMAGE};
my $mask  = $params->{MASK};
my $weight= $params->{WEIGHT};
my $stage = $params->{STAGE};
my $stage_id = $params->{STAGE_ID};
my $component = $params->{COMPONENT_ID};
# print "Input is from $stage $stage_id $component\n";

# XXX: TODO: if there's a fault or quality problem, then we can't process this image.
# Perhaps we shouldn't even get here

# Create coordinate file to convert to positions.
my ($coordfile,$coordname) = tempfile("${workdir}/detect.coords.$job_id.XXXX", 
                                      UNLINK => !$save_temps);
my ($targetfile,$targetname) = tempfile("${workdir}/detect.targets.$job_id.XXXX", 
                                        UNLINK => !$save_temps);
foreach my $row (@rows) {
    print $coordfile "$row->{RA1_DEG} $row->{DEC1_DEG}\n";
}

# Convert the sky coordinates to image coordinates with ppCoord.
my $command = "ppCoord -astrom $catalog -radec $coordname";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $command, verbose => $verbose);
## MEH need to adjust error_code to rc
unless ($success) {
    my $rc = $error_code >> 8;
    my_die("Unable to perform $command. Error_code: $error_code $rc",
           $params->{QUERY_ID},$fpa_id,$params->{'MJD-OBS'},
           $params->{FILTER},$params->{OBSCODE},$params->{STAGE},
           $rc);
}
my @response = split /\n/, (join "", @$stdout_buf);
my $i = 0;
foreach my $line (@response) {
    my ($r_ra,$r_dec,$trash,$r_x,$r_y,$r_chip) = split /\s+/, $line;
    print $targetfile "$r_x $r_y\n";
    my $row = $rows[$i];
    if ((abs($r_ra - $row->{RA1_DEG}) < 1e-8) && (abs($r_dec - $row->{DEC1_DEG}) < 1e-8)) {
        $row->{X_PXL} = $r_x;
        $row->{Y_PXL} = $r_y;
        $row->{EXTENSION_BASE} = $r_chip;
    } else {
        $error_code = $PS_EXIT_PROG_ERROR;
        my_die("Unable to match input RA/DEC with output RA/DEC: ($row->{RA1_DEG},$row->{DEC1_DEG}) -> ($r_ra,$r_dec) $error_code",
           $params->{QUERY_ID},$fpa_id,$params->{'MJD-OBS'},
           $params->{FILTER},$params->{OBSCODE},$params->{STAGE},
           $error_code);
    }
    $i++;
}

# Run psphotForced on the target list.
my $outroot = "${output_base}detectability.${stage}.${stage_id}";
$outroot .= ".$component" unless $stage eq 'chip';

$params->{PROC_ERROR} = 0;
my $psphot_cmd = "$psphotForced -psf $psf ";
$psphot_cmd .= "-file $image ";
$psphot_cmd .= "-mask $mask ";
$psphot_cmd .= "-variance $weight ";
$psphot_cmd .= "-srctext $targetname $outroot ";
$psphot_cmd .= "-D OUTPUT.FORMAT PS1_DV1 ";

( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $psphot_cmd, verbose => $verbose);
unless ($success) {
    # $params->{PROC_ERROR} = $PSTAMP_SYSTEM_ERROR;
    my $rc = $error_code >> 8;
    my_die("$psphot_cmd failed: $error_code $rc",
           $params->{QUERY_ID},$fpa_id,$params->{'MJD-OBS'},
           $params->{FILTER},$params->{OBSCODE},$params->{STAGE},
           $rc);
}

my $results = "${output_base}results.txt";
open OUT, ">$results" or 
        my_die("Failed to open results file $results",
           $params->{QUERY_ID},$fpa_id,$params->{'MJD-OBS'},
           $params->{FILTER},$params->{OBSCODE},$params->{STAGE},
           $PS_EXIT_UNKNOWN_ERROR);

# Read the output cmf and produce the data we need for the response file
my $class_id = $params->{CLASS_ID};
my $cmf = "${outroot}.${class_id}.cmf";
my $cmfbase = basename($cmf);
my ($tmp_Npix,$tmp_Qfact,$tmp_flux,$tmp_flux_error) = read_cmf_file($cmf,$rows[0]->{EXTENSION_BASE});
$i = 0;
print OUT "ROWNUM PROC_ERROR NPIX QFACTOR FLUX FLUX_SIG FPA_ID STAGE STAGE_ID COMPONENT CMFFILE\n";
foreach my $row (@rows) {
    $row->{PROC_ERROR} = $params->{PROC_ERROR};

    $row->{NPIX} = ${ $tmp_Npix }[$i];
    $row->{QFACTOR} = ${ $tmp_Qfact }[$i];
    $row->{FLUX} = ${ $tmp_flux }[$i];
    $row->{FLUX_SIG} = ${ $tmp_flux_error }[$i];
    
    print OUT "$row->{ROWNUM} $row->{PROC_ERROR} $row->{NPIX} $row->{QFACTOR} $row->{FLUX} $row->{FLUX_SIG} $fpa_id $stage $stage_id $component $cmfbase\n";
    $i++;
}
close(OUT);

exit 0;

#
# Utilities
#

# Taken largely from detect_query_read
sub read_cmf_file {
    my $cmf_file = shift;
    my $class_id = shift;
    my $extname = $class_id . ".psf";

    my @tmp_Npix = ();
    my @tmp_Qfact = ();
    my @tmp_flux = ();
    my @tmp_flux_err = ();

    my ($detect_F_col,$detect_N_col,$detect_flux_col,$detect_mag_col) = (-1, -1, -1, -1);

    
    my $status = 0;
    my $inFits = Astro::FITS::CFITSIO::open_file( $cmf_file, READONLY, $status ); # Open CMF
    check_fitsio($status);
    $inFits->movnam_hdu(BINARY_TBL, $extname, 0, $status) and check_fitsio($status);

    my $inHeader = $inFits->read_header();

    my $CMFversion = $inHeader->{EXTTYPE};
    $CMFversion =~ s/\'//g;
    $CMFversion =~ s/\s+//g;

    # This is the only data we actually care about
    my $column_defs;
    if ($CMFversion eq 'PS1_V2') {
	$column_defs = [
	    { name => 'PSF_QF',   type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_NPIX', type => '1J', writetype => TLONG },
	    { name => 'PSF_INST_MAG', type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_INST_MAG_SIG', type => '1E', writetype => TDOUBLE },
	    ];
    }
    elsif ($CMFversion eq 'PS1_DV1') {
	$column_defs = [
	    { name => 'PSF_QF',   type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_NPIX', type => '1J', writetype => TLONG },
	    { name => 'PSF_INST_FLUX', type => '1E', writetype => TDOUBLE },
	    { name => 'PSF_INST_FLUX_SIG', type => '1E', writetype => TDOUBLE },
	    ];
    }

    my @colNames; 
    my @colTypes;
    my @colWriteType;
    my %colData;
    foreach my $colSpec (@$column_defs) {
	push @colNames, $colSpec->{name};
	push @colTypes, $colSpec->{type};
	push @colWriteType, $colSpec->{writetype};
    }
    my $numRows;
    $inFits->get_num_rows($numRows, $status) and check_fitsio($status);

    my $correct_error = 0;
#    print STDERR "Ncols:" .  $#{ $column_defs } . "\n";
    foreach my $col (@$column_defs) {
	my ($col_num,$col_type,$col_data);
	$inFits->get_colnum(0, $col->{name}, $col_num, $status) and check_fitsio($status);
	if ($status != 0) {
	    $status = 0;
	    next;
	}
	$inFits->get_coltype($col_num, $col_type, undef, undef, $status) and check_fitsio($status);
	$inFits->read_col($col_type, $col_num, 1, 1, $numRows, 0, $col_data, undef, $status) and check_fitsio($status);
#	print STDERR "$col\t>>" . $col->{name} . "<<\t>>" . @{ $col_data } . "<<\n";
	if ($col->{name} eq 'PSF_QF') {
	    @tmp_Qfact = @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_NPIX') {
	    @tmp_Npix = @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_INST_MAG') {
	    @tmp_flux = map { $_ = 10**(-0.4 * $_) } @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_INST_MAG_SIG') {
	    @tmp_flux_err = map { $_ = $_ / (2.5 * (log(exp(1)) / log(10))) } @{ $col_data };
	    $correct_error = 1;
	}
	elsif ($col->{name} eq 'PSF_INST_FLUX') {
	    @tmp_flux = @{ $col_data };
	}
	elsif ($col->{name} eq 'PSF_INST_FLUX_SIG') {
	    @tmp_flux_err = @{ $col_data };
	}
    }
    if ($correct_error) {
	for (my $c = 0; $c <= $#tmp_flux_err; $c++) {
	    $tmp_flux_err[$c] = $tmp_flux_err[$c] * $tmp_flux[$c];
	}
    }
    $inFits->close_file( $status ) and check_fitsio($status);    
#    print STDERR "$CMFversion\n";
#    print STDERR "Q: $#tmp_Qfact\t@tmp_Qfact\nN: $#tmp_Npix\t@tmp_Npix\n";
    return(\@tmp_Npix, \@tmp_Qfact, \@tmp_flux, \@tmp_flux_err);
}


# From Astro::FITS::CFITSIO demo
sub check_fitsio
{
    my $status = shift;         # Status of FITSIO calls

    if ($status != 0) {
        my $msg;                # Message to output
        Astro::FITS::CFITSIO::fits_get_errstatus( $status , $msg );
	carp("CFITSIO error: $status => $msg");
        die "CFITSIO error: $msg\n";
    }
}

sub check_component {
    my ($stage, $stage_id, $component, $imagedb) = @_;

    print "Checking status of component $stage $stage_id $component\n";

    my $command;
    if ($stage eq 'diff') {
        $command = "difftool -diffskyfile -diff_id $stage_id -skycell_id $component";
    } elsif ($stage eq 'stack') {
        $command = "stacktool -sumskyfile -stack_id $stage_id";
    } elsif ($stage eq 'warp') {
        $command = "warptool -warped -warp_id $stage_id -skycell_id $component";
    } elsif ($stage eq 'chip') {
        $command = "chiptool -processedimfile -chip_id $stage_id -class_id $component";
    } else {
        die("check_component not implemented for stage $stage yet");
    }

    $command .= " -dbname $imagedb";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
        my $rc = $error_code >> 8;
        carp "$command failed $error_code $rc";
        exit $PS_EXIT_UNKNOWN_ERROR;
    }
    my $output = join "", @$stdout_buf;
    my $metadata = $mdcParser->parse($output);
    my $results = parse_md_list($metadata);
    if (scalar @$results != 1) {
        carp "$command returned too many components: " . scalar @$results;
        exit $PS_EXIT_UNKNOWN_ERROR;
    }
    my $it = $results->[0];

    if ($stage eq 'stack') {
        $it->{data_state} = $it->{state};
    }

    my $return_status = 0;
    if ($it->{quality}) {
        print "  Bad quality: $it->{quality}\n";
        $return_status = $PSTAMP_GONE;
    } elsif ($it->{fault}) {
        print "  Faulted: $it->{fault}\n";
        $return_status = $PSTAMP_GONE;
    } elsif ($it->{data_state} ne 'full') {
        # XXX does this work for stack?
        carp "  Faulted: $it->{data_state}\n";
        $return_status = $PSTAMP_GONE;
    } else {
        print "  Component ok.\n";
    }

    return $return_status;
}

sub my_die {
    my $message = shift;
    my $QUERY_ID = shift;
    my $FPA_ID = shift;
    my $MJD_OBS = shift;
    my $FILTER = shift;
    my $OBSCODE = shift;
    my $STAGE = shift;

    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    #carp("$message : $QUERY_ID $FPA_ID $MJD_OBS $FILTER $OBSCODE $STAGE");
    carp("$message : $QUERY_ID $FPA_ID $MJD_OBS $FILTER $OBSCODE $STAGE $exit_code");

    exit($exit_code);

}

sub exit_with_failure {
    my $status = shift;
    my $message = shift;
    carp("$message");
    exit($status);
}

