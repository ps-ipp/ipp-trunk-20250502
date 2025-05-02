#!/usr/bin/env perl

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

use Data::Dumper;
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use PS::IPP::Metadata::Stats;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my $RECIPE_PPSTATS = 'CHIPSTATS'; # Recipe to use with ppStats

# Parse command-line arguments
my ($det_type, $filelevel, $inst, $telescope, $filter,
    $det_id1, $iter1, $det_id2, $iter2, $operation, $mask,
    $workdir, $dbname, $no_update);
GetOptions(
           'det_type=s'    => \$det_type, # Detrend type for new detrend
           'filelevel=s'   => \$filelevel, # File level for new detrend
           'inst=s'        => \$inst, # Instrument for new detrend
           'telescope=s'   => \$telescope, # Telescope for new detrend
           'filter=s'      => \$filter, # Filter name for new detrend
           'det_id1=s'     => \$det_id1, # Detrend id for detrend 1
           'iteration1=s'  => \$iter1, # Iteration for detrend 1
           'det_id2=s'     => \$det_id2, # Detrend id for detrend 2
           'iteration2=s'  => \$iter2, # Iteration for detrend 2
           'operation=s'   => \$operation, # Operation to perform on files
           'mask'          => \$mask, # Operation is on a mask
           'workdir=s'     => \$workdir, # Working directory for output files
           'dbname=s'      => \$dbname, # Database name
           'no-update'     => \$no_update, # Don't update the database
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options --det_type --filelevel --inst --telescope --det_id1 --iteration1 --det_id2 --iteration2 --workdir",
           -exitval => 3,
           )
    unless defined $det_type
    and defined $filelevel
    and defined $inst
    and defined $telescope
    and defined $det_id1
    and defined $iter1
    and defined $det_id2
    and defined $iter2
    and defined $operation
    and defined $workdir;

$ipprc->define_camera($inst);

my $STATS =
   [
       #          PPSTATS KEYWORD         STATISTIC          CHIPTOOL FLAG
       { name => "ROBUST_MEDIAN",  type => "mean",  flag => "-bg",             dtype => "float" },
       { name => "ROBUST_MEDIAN",  type => "stdev", flag => "-bg_mean_stdev",  dtype => "float" },
       { name => "ROBUST_STDEV",   type => "rms",   flag => "-bg_stdev",       dtype => "float" },
   ];

# Look for programs we need
my $missing_tools;
my $detselect = can_run('detselect') or (warn "Can't find detselect" and $missing_tools = 1);
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
my $ppArith = can_run('ppArith') or (warn "Can't find ppArith" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Get the list of inputs
my $files1 = filelist($det_id1, $iter1); # Hash of input files for detrend 1
my $files2 = filelist($det_id2, $iter2); # Hash of input files for detrend 2
die("File lists for detrends have differing lengths") unless scalar keys %$files1 == scalar keys %$files2;

my ($det_id, $iter);          # Detrend identifier for the new detrend
unless ($no_update) {
    my $command = "$dettool -register_detrend -det_type $det_type -filelevel $filelevel -workdir $workdir " .
        "-inst $inst -telescope $telescope"; # Command to run
    $command .= " -filter $filter" if defined $filter;
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 1);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        die("Unable to run dettool -register_detrend: $error_code");
    }

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or die("Unable to parse metadata config doc\n");
    my $md = parse_md_list($metadata) or die("Unable to parse metadata list\n");

    $det_id = $$md[0]->{det_id};
    $iter = $$md[0]->{iteration};

    die("Unable to get det_id and iteration for new detrend.\n") unless defined $det_id and defined $iter;
} else {
    $det_id = 'DUMMY_DET_ID';
    $iter = 'DUMMY_ITER';
}

my $outRoot = caturi($workdir, "$inst.$det_id.$iter"); # Output root name
my $filerule = (defined $mask ? "PPARITH.OUTPUT.MASK" : "PPARITH.OUTPUT.IMAGE"); # File rule for ppArith

foreach my $class_id ( keys %$files1 ) {
    my $md1 = $$files1{$class_id};
    my $md2 = $$files2{$class_id};
    die("Class_id=$class_id not defined for det_id=$det_id2") unless defined $md2;

    my $uri1 = $$md1[0]->{uri};
    my $uri2 = $$md2[0]->{uri};

    die("Unable to find input file $uri1\n") unless $ipprc->file_exists($uri1);
    die("Unable to find input file $uri2\n") unless $ipprc->file_exists($uri2);

    my $outName = $ipprc->filename($filerule, $outRoot, $class_id);
    my $outStats = $outRoot . '.stats';

    my $command = "$ppArith -file1 $uri1 -op \'$operation\' -file2 $uri2 $outRoot"; # Command to run
    $command .= " -stats $outStats -recipe PPSTATS $RECIPE_PPSTATS";
    $command .= ' -mask' if defined $mask;
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 1);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        die("Unable to run ppArith: $error_code");
    }

    die("Unable to find ppArith product: $outName\n") unless $ipprc->file_exists($outName);
    die("Unable to find ppArith product: $outStats\n") unless $ipprc->file_exists($outStats);

    # Get the statistics on the processed image
    my $stats = PS::IPP::Metadata::Stats->new($STATS); # Stats parser
    {
        my $statsFile;          # File handle
        open $statsFile, $ipprc->file_resolve($outStats) or die("Can't open stats file $outStats: $!");
        my @contents = <$statsFile>; # Contents of file
        close $statsFile;

        my $metadata = $mdcParser->parse(join "", @contents) or die("Unable to parse metadata config doc");

        unless ($stats->parse($metadata)) {
            &my_die("Failure extracting metadata from the statistics output file.\n");
        }
    }

    # Register the imfile
    unless ($no_update) {
        my $command = "$dettool -register_detrend_imfile -det_id $det_id "; # Command to run
        $command .= " -class_id $class_id -uri $outName -path_base $outRoot";
        $command .= $stats->cmdflags();
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => 1);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            die("Unable to run dettool -register_detrend_imfile: $error_code");
        }
    }
}


### Pau.


# Get a list of files for the given detrend
sub filelist
{
    my $det_id = shift;         # Detrend identifier
    my $iter = shift;           # Iteration

    my $command = "$detselect -select -det_id $det_id -iteration $iter"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 1);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        die("Unable to run detselect: $error_code");
    }

    # Because of the length, need to split into individual metadatas --- it parses SO much quicker!
    my %files;

    my $md = $mdcParser->parse( join( "", @$stdout_buf ) ); # Parsed metadata
    my $list = parse_md_list( $md );

    foreach my $item ( @$list ) {
        my $class_id = $item->{class_id};
        die("Multiple definitions of class_id=$class_id found for det_id=$det_id, iteration=$iter\n") if
            defined $files{$class_id};
        $files{$class_id} = parse_md_list($md);
    }

    return \%files;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
