#!/usr/bin/env perl
# this program is used to run 'burntool' on images for a single chip
# for all exposures for a given time period.
# USAGE: ipp_apply_burntool.pl --dbname gpc1 --class_id XY00 --dateobs_begin YYYY/MM/DD --dateobs_end YYYY/MM/DD

# XXX todo:
# - add filters to processedimfile
#

use warnings;
use strict;
use Carp;

use IPC::Cmd 0.36 qw( can_run run );
#use IPC::Run 0.36 qw( run );
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ( $class_id, $dateobs_begin, $dateobs_end, $convert, $camera, $dbname, $logfile, $verbose, $save_temps, $rerun);
GetOptions(
    'class_id=s'        => \$class_id, # chip identifier
    'dateobs_begin=s'   => \$dateobs_begin, # exposure date/time range start
    'dateobs_end=s'     => \$dateobs_end, # exposure date/time range stop
    'camera=s'          => \$camera,     # Camera
    'dbname|d=s'        => \$dbname, # Database name
    'logfile=s'         => \$logfile,
    'rerun'             => \$rerun,
    'verbose'           => \$verbose,   # Print to stdout
    'save-temps'        => \$save_temps, # Save temporary files?

    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --class_id --dateobs_begin --dateobs_end --dbname",
          -exitval => 3,
          ) unless
    defined $class_id and
    defined $dateobs_begin and
    defined $dateobs_end and
    defined $dbname;

unless (defined $camera) {
    $camera = "GPC1";
#    warn("No camera defined. Defaulting to $camera and warning.");
}

my $missing_tools;
my $regtool  = can_run('regtool')  or (warn "Can't find regtool" and $missing_tools = 1);
my $funpack  = can_run('funpack')  or (warn "Can't find funpack" and $missing_tools = 1);
my $burntool = can_run('burntool') or (warn "Can't find burntool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $nebXattr = can_run('neb-xattr') or (warn "Can't find neb-xattr" and $missing_tools = 1);
my $nebreplicate = can_run('neb-replicate') or (warn "Can't find neb-replicate" and $missing_tools = 1);
#my $fpack    = can_run('fpack')    or (warn "Can't find fpack" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# IPP configuration (including nebulous)
my $ipprc = PS::IPP::Config->new( $camera ) or my_die("Unable to set up");

$ipprc->redirect_output($logfile) if $logfile;

# Determine the value of a "good" burntool run.
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
my $outState = -1 * abs($burntoolStateGood);

print ">>$burntoolStateGood<<\n";

# Define list of images to examine.
my $command = "$regtool -processedimfile";
$command .= " -class_id $class_id";
$command .= " -dateobs_begin $dateobs_begin";
$command .= " -dateobs_end $dateobs_end";
$command .= " -ordered_by_date";
$command .= " -dbname $dbname" if defined $dbname;

( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run ( command => $command, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform regtool: $error_code", $class_id, $dateobs_begin, $dateobs_end, $PS_EXIT_SYS_ERROR);
}

my @whole = split /\n/, (join "", @$stdout_buf);
my @single = ();
my $files;
while ( scalar @whole > 0 ) {
    my $value = shift @whole;
    push @single, $value;
    if ($value =~ /^\s*END\s*$/) {
        push @single, "\n";

        my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) );
        &my_die("Unable to parse output from regtool", $class_id, $dateobs_begin, $dateobs_end, $PS_EXIT_PROG_ERROR) unless
            defined $list;
        push @{ $files }, @$list;
        @single = ();
    }
}

#my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
#    &my_die("Unable to parse metadata", $class_id, $dateobs_begin, $dateobs_end, $PS_EXIT_SYS_ERROR);
#
#my $files = parse_md_list($metadata) or
#    &my_die("Unable to parse metadata list", $class_id, $dateobs_begin, $dateobs_end, $PS_EXIT_SYS_ERROR);

my $REALRUN = 1;

# Process each file in turn, checking to see if it needs processing, and doing it all in a single pass
my $processNext = 0;
my $previousTable = '';

foreach my $file (@$files) {
    my $exp_id = $file->{exp_id};
    my $state = $file->{burntool_state};

    my $rawImfile = $file->{uri};
    my $rawImfileReal = $ipprc->file_resolve($rawImfile, 0);

    my $outTable  = $file->{uri};
    
    if($burntoolStateGood == 15) {
        $outTable =~ s/fits$/burn.v15.tbl/;	    
    } else {
        $outTable =~ s/fits$/burn.tbl/;  	    
    }
    my $outTableReal = $ipprc->file_resolve($outTable, 1);

    my $process = 0;
    my $previousTableStyle = 0;

    if ($state == 0) {
        $process = 1;
    }
    elsif ($state == -1) {
        $process = 1;
    }
    elsif ($state == -3) {
        $process = 1;
    }
    elsif ($state == -2) {
        &my_die("Aborting as $rawImfile has modified pixel data!");
    }
    elsif ($state == $burntoolStateGood) {
        $process = 0;
        $previousTableStyle = 1;
    }
    elsif ($state == -1 * $burntoolStateGood) {
        $process = 0;
#       $previousTable = "in=$outTableReal";
        $previousTableStyle = -1;
    }
    else {
        $process = 1;
    }
    if ($rerun) {
	$process = 1;
    }
    if ($REALRUN == 0) {
        print "##Pretending to process as instructed!\n";
        $process = 1;
    }
    if (($process == 1)||($processNext == 1)) {
        $processNext = 1;
        # Set state to processing
        my $status = vsystem ("$regtool -dbname $dbname -updateprocessedimfile -exp_id $exp_id -class_id $class_id -burntool_state -1", $REALRUN);
        if ($status) {
            &my_die("failed to update imfile");
        }
        $file->{burntool_state} = -1;

        my $tempfile = new File::Temp ( TEMPLATE => "$file->{exp_name}.XXXX",
                                        DIR => '/tmp/',
                                        UNLINK => !$save_temps,
                                        SUFFIX => '.fits');
        my $tempPixels = $tempfile->filename;

        # Run burntool
        $status = vsystem ("$funpack -S $rawImfileReal > $tempPixels", $REALRUN);
        if ($status) {
            &my_die("failed on funpack",$exp_id,$class_id);
        }
        if ($previousTable ne '') {
            $status = vsystem ("$burntool $tempPixels $previousTable out=$outTableReal tableonly=t persist=t", $REALRUN);
        }
        else {
            $status = vsystem ("$burntool $tempPixels out=$outTableReal tableonly=t persist=t", $REALRUN);
        }
        if ($status) {
            &my_die("failed on burntool",$exp_id,$class_id);
        }
        &my_die("Unable to find output file: $outTableReal", $exp_id, $class_id) unless $ipprc->file_exists($outTableReal);

        $status = vsystem ("$nebXattr --write $outTable user.copies:2",$REALRUN);
	$status = vsystem ("$nebreplicate --set_copies 2 $outTable",$REALRUN);
        $status = vsystem ("$regtool -dbname $dbname -updateprocessedimfile -exp_id $exp_id -class_id $class_id -burntool_state $outState", $REALRUN);
        if ($status) {
            &my_die("failed to update imfile");
        }

        $previousTableStyle = -1;

        print "\n";
    }

    if ($previousTableStyle == 1) {
        $previousTable = "infits=$rawImfileReal";
    }
    elsif ($previousTableStyle == -1) {
        $previousTable = "in=$outTableReal";
    }
    else {
        die "previousTableStyle undefined!\n";
    }

}

exit 0;

sub vsystem {
    my $command = shift;
    my $realrun = shift;

    print "$command\n";

    my $status = 0;
    if ($realrun) {
        $status = system ($command);
    }

    return $status;
}

sub my_die {
    my $message = shift;
    if ($#_ != -1) {
        my $exp_id = shift;
        my $class_id = shift;
        vsystem("$regtool -dbname $dbname -updateprocessedimfile -exp_id $exp_id -class_id $class_id -burntool_state -3",1);
    }
    printf STDERR "$message\n";
    exit 1;
}
