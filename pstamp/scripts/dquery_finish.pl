#!/bin/env perl

# dquery_finish.pl

use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

use Sys::Hostname;
use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Copy;
use Astro::FITS::CFITSIO qw( :constants );
Astro::FITS::CFITSIO::PerlyUnpacking(1);

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw( :standard );

my ( $req_id, $req_name, $req_file, $product, $outdir, $dbname, $dbserver, $verbose, $save_temps );

GetOptions(
           'req_id=s'   => \$req_id,
           'req_name=s' => \$req_name,
           'req_file=s' => \$req_file,
           'product=s'  => \$product,
           'outdir=s'   => \$outdir,
	   'dbname=s'   => \$dbname,
	   'dbserver=s' => \$dbserver,
	   'verbose'    => \$verbose,
	   'save-temps' => \$save_temps,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

my $err = "";

$err .= "--req_id is required\n" if !$req_id;
$err .= "--req_name is required\n" if !$req_name;
$err .= "--product is required\n" if !$product;
$err .= "--outdir is required\n" if !$outdir;

die "$err" if $err;

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $dsreg  = can_run('dsreg')  or (warn "Can't find dsreg"  and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new(); # IPP Configuration

my $outputDataStoreRoot = metadataLookupStr($ipprc->{_siteConfig}, 'DATA_STORE_ROOT');
exit ($PS_EXIT_CONFIG_ERROR) unless defined $outputDataStoreRoot; # lookup failure outputs a message

if (!$dbserver) {
    $dbserver = metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

if ($product eq "NULL") {
    # parsing failed just with fault = 0 (this leaves previously set fault in place
    update_request($req_id, 0, $verbose);
    exit 0;
}

if (! -e $outdir ) {
    # something must have gone wrong at the parse stage
    print STDERR "output fileset directory $outdir does not exist\n" if $verbose;
    if (! mkdir $outdir ) {
        update_request($req_id, $PS_EXIT_SYS_ERROR, $verbose);
        die "cannot create output directory $outdir";
    }
} elsif (! -d $outdir) {
    update_request($req_id, $PS_EXIT_SYS_ERROR, $verbose);
    die "output fileset directory $outdir exists but is not a directory";
}


my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $request_fault = 0;
my @jobs;
{
    my $command = "$pstamptool -listjob -req_id $req_id";
    $command .= " -jobType detect_query";   # temporary
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
    my $output = join "", @$stdout_buf;
    if (!$output) {
        if ($verbose) {
            print STDERR "Request $req_id produced no jobs.\n";
        }
        # assume that the parser set the fault
    } else {
        my $metadata =  $mdcParser->parse($output) or die("Unable to parse metdata config doc for jobs list");
        my $jobs = parse_md_list($metadata);
        @jobs = @$jobs;
    }
}

my ($REGLIST, $reg_list) = tempfile("$outdir/reglist.XXXX", UNLINK => !$save_temps);
my $response_file = "response.$req_id.fits";
my $response_path = "$outdir/$response_file";

print $REGLIST "$response_file|||table|\n";

my @columns = qw(ROWNUM PROC_ERROR NPIX QFACTOR FLUX FLUX_SIG FPA_ID STAGE STAGE_ID COMPONENT CMFFILE);
my %colData;
foreach my $col (@columns) {
    $colData{$col} = [];
}

## ir parse_error.txt exists then also include it on datastore
if (-e "$outdir/parse_error.txt") {
    print $REGLIST "parse_error.txt|||text|\n";
}

my $i = 0;
foreach my $job (@jobs) {
    my $job_id = $job->{job_id};

    if ($job->{fault}) {
        # how do we get this information back to the user?
        print STDERR "job: $job_id faulted with $job->{fault}\n";
        next;
    }
    my $outputBase = $job->{outputBase};
    my $results = "${outputBase}results.txt";

    read_results($results, \%colData);

    my $cmf = $colData{CMFFILE}->[$i];
    print $REGLIST "$cmf|||table|\n";

    $i++;
}
close $REGLIST;

write_response_file($response_path, $req_name, \%colData);

if (-s $reg_list) {
    my $command = "$dsreg --add $req_name --product $product --list $reg_list";
    $command .= " --copy --datapath $outdir";
    $command .= " --type MOPS_DETECTABILITY_RESPONSE";
    $command .= " --ps0 $req_id";
    $command .= " --dbname $dbname" if $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $request_fault = $error_code >> 8;
        print STDERR "Unable to perform $command return code: $request_fault";
        # fall through to call stop request
    }
}

update_request($req_id, $request_fault, $verbose);

exit 0;

sub read_results {
    my $results_file = shift;
    my $results = shift;

    open IN, "<$results_file" or my_die("failed to open $results_file\n", $PS_EXIT_UNKNOWN_ERROR);

    foreach my $line (<IN>) {
        # skip header
        next if $line =~ /ROWNUM/;
        chomp $line;
        my @words = split " ", $line;
        my ($rownum, $proc_error, $npix, $qfactor, $flux, $flux_sig, $fpa_id, $stage, $stage_id, $component, $cmf) = @words;
        push @{$results->{ROWNUM}}, $rownum;
        push @{$results->{PROC_ERROR}}, $proc_error;
        push @{$results->{NPIX}}, $npix;
        push @{$results->{QFACTOR}}, $qfactor;
        push @{$results->{FLUX}}, $flux;
        push @{$results->{FLUX_SIG}}, $flux_sig;
        push @{$results->{FPA_ID}}, $fpa_id;
        push @{$results->{STAGE}}, $stage;
        push @{$results->{STAGE_ID}}, $stage_id;
        push @{$results->{COMPONENT}}, $component;
        push @{$results->{CMFFILE}}, $cmf;
    }
    # XXX: make sure that the size of each of these arrays is the same
}

sub write_response_file {
    my $output = shift;
    my $query_id = shift;
    my $results = shift;

    my $columns;
    my $headers;

    my $EXTVER_IS_1 = 0 ; # (scalar(keys(%query)) == 1);
#    print "EXTVER: $EXTVER_IS_1\n";
    my ($FPA_ID,$MJD_OBS,$filter,$obscode,$status);
    if ($EXTVER_IS_1 == 1) {
	# Specification of columns to write
	$columns = [
	    # matching rownum from detectability original request
	    { name => 'ROWNUM',   type => '20A', writetype => TSTRING }, 
	    # any errors that occurred during processing
	    { name => 'ERROR_CODE',   type => 'V', writetype => TULONG }, 
	    # number of pixels used in hypothetical PSF for the query detection
	    { name => 'DETECT_N', type => 'V',   writetype => TULONG },
	    # detectibility, indicating the fraction of PSF pixels detetable by IPP
	    { name => 'DETECT_F', type => 'D',   writetype => TDOUBLE },
	    # flux of the target source
	    { name => 'TARGET_FLUX', type => 'D', writetype => TDOUBLE },
	    # error in the flux of the target source
	    { name => 'TARGET_FLUX_SIG', type => 'D', writetype => TDOUBLE },
	    ];
	
	# Header translation table
	$headers = {
	    'QUERY_ID' => { name => 'QUERY_ID', type => TSTRING, 
			    comment => 'MOPS Query ID for this batch query' },
	    'FPA_ID' => { name => 'FPA_ID',   type => TSTRING, 
			  comment => 'original FPA_ID used at ingest' },
# 	    'MJD-OBS' => { name => 'FPA_ID',   type => TSTRING, 
# 			  comment => 'original FPA_ID used at ingest' },
# 	    'FILTER' => { name => 'FPA_ID',   type => TSTRING, 
# 			  comment => 'original FPA_ID used at ingest' },
# 	    'OBSCODE' => { name => 'FPA_ID',   type => TSTRING, 
# 			  comment => 'original FPA_ID used at ingest' },
	};
    }
    else {
	# Specification of columns to write
	$columns = [
	    # matching rownum from detectability original request
	    { name => 'ROWNUM',   type => '20A', writetype => TSTRING}, 
	    # any errors that occurred during processing
	    { name => 'ERROR_CODE',   type => 'V', writetype => TULONG }, 
	    # number of pixels used in hypothetical PSF for the query detection
	    { name => 'DETECT_N', type => 'V',   writetype => TULONG },
	    # detectibility, indicating the fraction of PSF pixels detetable by IPP
	    { name => 'DETECT_F', type => 'D',   writetype => TDOUBLE },
	    # flux of the target source
	    { name => 'TARGET_FLUX', type => 'D', writetype => TDOUBLE },
	    # error in the flux of the target source
	    { name => 'TARGET_FLUX_SIG', type => 'D', writetype => TDOUBLE },
	    # The FPA That would be in the header if it were to be there.
	    { name => 'FPA_ID',   type => '20A', writetype => TSTRING },
	    ];
	
	# Header translation table
	$headers = {
	    'QUERY_ID' => { name => 'QUERY_ID', type => TSTRING, 
			    comment => 'MOPS Query ID for this batch query' },
	};
    }	

    # Parse the list of columns
    my @colNames;                   # Names of columns
    my @colTypes;                   # Types of columns
    my %colData;                    # Data for each column
    my @colWriteType;                 # type to use to write
    foreach my $colSpec ( @$columns) {
	push @colNames, $colSpec->{name};
	push @colTypes, $colSpec->{type};
	push @colWriteType, $colSpec->{writetype};
	$colData{$colSpec->{name}} = [];
    }

    my $inHeader = { };

    # Hack to force the data to match the detect_response_create formats

    $inHeader->{QUERY_ID}->{value} = $query_id;
    if ($EXTVER_IS_1 == 1) {
	my $fpa_id = $results->{FPA_ID}->[0];
	$inHeader->{FPA_ID}->{value} = $fpa_id;
    }
    
    # Fill the table columns with the data, making sure the flux is defined

    @{$colData{'ROWNUM'}} = @{ $results->{ROWNUM} };
    @{$colData{'ERROR_CODE'}} = @{ $results->{PROC_ERROR} };
    @{$colData{'DETECT_N'}} = @{ $results->{NPIX} };
    @{$colData{'DETECT_F'}} = @{ $results->{QFACTOR} };
    @{$colData{'TARGET_FLUX'}} = @{ $results->{FLUX} };
    @{$colData{'TARGET_FLUX_SIG'}} = @{ $results->{FLUX_SIG} };
    @{$colData{'FPA_ID'}} = @{ $results->{FPA_ID} };

    my $numRows = 0;
    # Back to detect_response_create:
    $status = 0;
#    print "$output\n";
    if (-e $output) {
        unlink $output or die "failed to unlink existing response file $output\n";
    }
    my $outFits = Astro::FITS::CFITSIO::create_file( $output, $status );
    check_fitsio( $status );
    $outFits->create_img( 16, 0, undef, $status );
    check_fitsio( $status );
    my $EXTNAME = 'MOPS_DETECTABILITY_RESPONSE';
    
    $outFits->create_tbl( BINARY_TBL(), $numRows, scalar @colNames, \@colNames, \@colTypes, undef, $EXTNAME, $status);
    check_fitsio( $status );

    # TODO: get the extension version number from somewhere common
    $outFits->write_key( TINT, 'EXTVER', 2, 'Version of this Extension', $status );
    check_fitsio( $status );

    # Write the Extension keywords
    foreach my $keyword ( keys %$headers ) {
	my $value = $inHeader->{$keyword}->{value}; # Header keyword value
	unless (defined $value) {
	    print "Can't find header keyword $keyword\n";
	    next;
	}
	$value =~ s/\'//g;
	my $name    = $headers->{$keyword}->{name}; # New name
	my $type    = $headers->{$keyword}->{type}; # Type
	my $comment = $headers->{$keyword}->{comment}; # Comment
	$outFits->write_key( $type, $name, $value, $comment, $status );
	check_fitsio( $status );
    }

    for (my $i = 0; $i < scalar @colNames; $i++) {
	my $colName = $colNames[$i];# Column name
	my $writeType = $colWriteType[$i];
	my $numRows = scalar(@{$colData{$colName}});
	unless(defined($writeType)) {
	    print "write type undefined for $colName\n";
	}
	unless(defined($numRows)) {
	    print "num Rows undefined for $colName\n";
	}
	unless(defined($status)) {
	    print "status undefined for $colName\n";
	}
	unless(defined($colData{$colName})) {
	    print "col data undefined for $colName\n";
	}
	unless(defined($colName)) {
	    print "column name undefined for $i\n";
	}
	print STDERR "$writeType $i $numRows $colName $status @{ $colData{$colName} }\n";
	$outFits->write_col( $writeType, $i + 1, 1, 1, $numRows, $colData{$colName}, $status );
	print STDERR "$writeType $i $numRows $colName $status\n";
	check_fitsio( $status );
    }
    $outFits->close_file( $status );
    return($status);
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
sub update_request {
    my $req_id = shift;
    my $fault = shift;
    my $verbose = shift;
    
    my $command = "$pstamptool -updatereq -req_id $req_id";
    if ($fault) {
        $command .= " -set_fault $fault";
    } else {
        $command .= " -set_state stop";
    }
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
}

sub my_die {
    my $msg = shift;
    my $fault = shift;

    print STDERR $msg;

    update_request($req_id, $fault);

    exit $fault;
}
