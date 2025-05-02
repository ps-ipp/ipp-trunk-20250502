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
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use File::Temp qw( tempfile );
use File::Spec;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $dvoImageOverlaps = can_run('dvoImageOverlaps') or (warn "Can't find dvoImageOverlaps" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

my ($warp_id, $camera, $tess_dir, $dbname, $verbose, $no_update, $no_op, $logfile, $save_temps);
GetOptions(
    'warp_id|i=s'       => \$warp_id, # Warp identifier
    'camera|c=s'        => \$camera, # Camera name
    'tess_dir=s'        => \$tess_dir, # Tessellation directory
    'dbname|d=s'        => \$dbname, # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op, # Don't do any operations
    'logfile=s'         => \$logfile,
    'save-temps'        => \$save_temps, # Save temporary files?
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --warp_id --camera --tess_dir",
    -exitval => 3,
) unless defined $warp_id
    and defined $camera
    and defined $tess_dir;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $warp_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$ipprc->redirect_to_logfile($logfile) or my_die( "Unable to redirect output", $warp_id, $PS_EXIT_SYS_ERROR ) if $logfile;
print "FULL COMMAND: $0 @ARGS\n\n";

&my_die("Tessellation identifier not provided: $tess_dir", $warp_id, $PS_EXIT_SYS_ERROR) unless $tess_dir ne "NULL";

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Get list of component imfiles for exposure
my $imfiles;
{
    my $command = "$warptool -imfile -warp_id $warp_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform warptool -imfile: $error_code", $warp_id, $error_code);
    }

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $warp_id, $PS_EXIT_PROG_ERROR);
    $imfiles = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $warp_id, $PS_EXIT_PROG_ERROR);
}

# Where do we get the astrometry source from? Do we limit by astrometry error?
my $astromSource;               # The astrometry source filerule (eg, PSASTRO.OUTPUT, PSASTRO.OUTPUT.MEF)
my $astromAccept;               # Accept the astrometry unconditionally?
my $maxCerror = 0.0;
{
    my $command = "$ppConfigDump -camera $camera -dump-recipe PSWARP -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $warp_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $warp_id, $PS_EXIT_PROG_ERROR);
    $astromSource = metadataLookupStr($metadata, 'ASTROM.SOURCE');
    $astromAccept = metadataLookupBool($metadata, 'ASTROM.ACCEPT');
    $maxCerror    = metadataLookupF32($metadata, 'MAX.CERROR');
}

# Determine the imfile/skycell overlaps
my @overlaps = ();
unless ($no_op) {
    # Calculate the overlaps between imfiles and skycells

    # tess_dir is the DVO db holding the tessalations
    # this may be an abstract name in site.config:TESSELLATIONS, a URI, or an absolute path.
    # convert this to an absolute path
    my $tess_dir_abs = $ipprc->tessellation_catdir( $tess_dir ); # Tessellation catdir for DVO
    $tess_dir_abs = $ipprc->convert_filename_absolute( $tess_dir_abs );

    my %unique_skycells = (); # Identified skycells (all unique by virtue of hash property)

    # astrometry is always determined at the camera stage; we have a MEF astrometry file from psastro
    my $imfile = $imfiles->[0];
    my $camRoot = $imfile->{cam_path_base};
    my $astromFile = $ipprc->filename($astromSource, $camRoot); # Astrometry file
    if (!$astromFile) {
        &my_die("Unable to determine the astrometry source", $warp_id, $PS_EXIT_DATA_ERROR);
    }
    $astromFile = $ipprc->file_resolve($astromFile);
    if (!$astromFile) {
        &my_die("Unable to resolve real astrometry source filename", $warp_id, $PS_EXIT_DATA_ERROR);
    }

    my @matchlist = get_overlaps($astromFile, $tess_dir_abs, $astromAccept,$maxCerror); # List of overlaps
    if (! @matchlist) {
        # OLD: &my_die("Unable to perform dvoImageOverlaps: missing astrometry", $warp_id, $PS_EXIT_DATA_ERROR);
	warn("no overlaps found (bad astrometry); setting warpRun state to 'drop'\n");

	# Add the processed file to the database
	unless ($no_update) {
	    my $command = "$warptool -updaterun -set_state drop -warp_id $warp_id"; # Command to run warptool
	    $command .= " -dbname $dbname" if defined $dbname;
	    
	    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		run(command => $command, verbose => $verbose);
	    unless ($success) {
		$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		warn("Unable to perform warptool -updaterun -set_state fail: $error_code\n");
		exit($error_code);
	    }
	}
	exit(0);
    }
    # Match each of the imfiles to this list (the input images may be split, but the astrometry is not)
    # tess_dir (not tess_dir_abs) is supplied here so it may be written to the db
    foreach my $imfile (@$imfiles) {
        extract_overlaps(\@matchlist, $imfile, $astromFile, $tess_dir, \@overlaps, \%unique_skycells);
    }
} else {
    # create an overlap with an entry for each skycell:imfile match
    foreach my $imfile (@$imfiles) {
        my %overlap = ();
        $overlap{skycell_id} = 'default';
        $overlap{tess_dir}   = 'default';
        $overlap{cam_id}     = $imfile->{cam_id};
        $overlap{class_id}   = $imfile->{class_id};
        $overlap{fault}      = $imfile->{fault};
        push @overlaps, \%overlap;
    }
}

# If no overlaps are found, the astrometry calibration was poor, but
# not bad enough for the camera-stage quality to be marked as bad.
# we set the warpRun state to 'fail' since no warpSkyCellMap can be generated.
if (scalar @overlaps == 0) {
    warn("no overlaps found (bad astrometry); setting warpRun state to 'drop'\n");

    # Add the processed file to the database
    unless ($no_update) {
	my $command = "$warptool -updaterun -set_state drop -warp_id $warp_id"; # Command to run warptool
	$command .= " -dbname $dbname" if defined $dbname;
	
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    warn("Unable to perform warptool -updaterun -set_state fail: $error_code\n");
	    exit($error_code);
	}
    }
    exit(0);
}

# Generate a MDC file with the overlaps
my ($overlapFile, $overlapName) = tempfile( "/tmp/overlaps.wrp.$warp_id.mdc.XXXX", UNLINK => !$save_temps );
print $overlapFile "warpSkyCellMap MULTI\n\n";
foreach my $overlap (@overlaps) {
    print $overlapFile "warpSkyCellMap   METADATA\n";
    print $overlapFile "  warp_id        S32    $warp_id\n";
    print $overlapFile "  skycell_id     STR    $overlap->{skycell_id}\n";
    # XXX convert tess_id here to tess_dir when db scheme is updated
    print $overlapFile "  tess_id        STR    $overlap->{tess_dir}\n";
    print $overlapFile "  cam_id         S32    $overlap->{cam_id}\n";
    print $overlapFile "  class_id       STR    $overlap->{class_id}\n";
    print $overlapFile "  fault          S16    $overlap->{fault}\n";
    print $overlapFile "END\n\n";
}
close $overlapFile;

system "cat $overlapName" if $verbose;

# Add the processed file to the database
unless ($no_update) {
    my $command = "$warptool -addoverlap -mapfile $overlapName"; # Command to run warptool
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform warptool -addoverlap: $error_code\n");
        exit($error_code);
    }
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $warp_id = shift;        # Warp identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $warp_id and not $no_update) {
        my $command = "$warptool -addoverlap";
        $command .= " -warp_id $warp_id";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

# Run dvoImageOverlaps to get the overlaps; return the output
sub get_overlaps
{
    my $filename = shift;       # Filename on which to run dvoImageOverlaps
    my $tess_dir_abs = shift;   # Tessellation directory
    my $accept = shift;         # Do we use the -accept-astrom flag?
    my $maxCerror = shift;	# maximum allowed astrometric error

    my $command = "$dvoImageOverlaps -D CATDIR $tess_dir_abs " . $filename;
    $command .= " -accept-astrom" if $accept;
    $command .= " -D OVERLAPS_MAX_CERROR $maxCerror"; # skip poorly calibrated images from warps
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    if (!$success) {
        print "Unable to calculate overlaps from $filename\n";
        print "STDOUT:\n";
        foreach my $line (@$stdout_buf) {
            print ">>>$line";
        }
        print "STDERR:\n";
        foreach my $line (@$stderr_buf) {
            print ">>>$line";
        }
        return 0;
    }
    return split ('\n', (join "", @$stdout_buf));
}

# Extract a list of overlaps for an imfile
#
# The command "dvoImageOverlaps FILENAME" returns:
# FILENAME[HDU] : SKYCELL.ID
# eg: 729534pa.cmf[ccd00.hdr]  :  skycell.051.fits
# for the case of split data, we need to identify the input imfiles based on this information
sub extract_overlaps
{
    my $matches = shift;         # Reference to list of skycells from dvoImageOverlaps
    my $imfile = shift;          # Imfile information
    my $filename = shift;        # Filename used with dvoImageOverlaps
    my $tess_dir = shift;        # Tessellation identifier
    my $overlaps = shift;        # Reference to list of overlaps
    my $unique_skycells = shift; # Reference to hash of found skycells

    # Get rid of the path
    my @dirlist = File::Spec->splitdir( $filename ); # The elements of the full path
    my $filenamecut = pop @dirlist;
    my @splitlist = split /:/, $filenamecut;
    $filename = pop @splitlist;

    # Work out how to identify this imfile in the output
    my $fileLevel = $imfile->{filelevel};
    my $entry;  # How to identify this imfile in the dvoImageOverlaps output

    if (lc($fileLevel) eq "chip") {
        # in the case of SPLIT images, all CLASSes are included in the output list
        # we need to pull out the single CLASS_ID for this imfile from the full list
        my $class_id = $imfile->{class_id};
        my $chipRoot = $imfile->{chip_path_base};
        my $extname = $ipprc->extname_rule("CMF.HEAD", $class_id); # MEF psastro output

        $entry = $filename . '\[' . $extname . '\]';
        print STDERR "entry: $entry, class: $class_id, extname: $extname, chiproot: $chipRoot\n" if $verbose;
    } else {
        # in the case of MEF or SINGLE images, there is only a single CLASS in the output list
        $entry = $filename;
        print STDERR "entry: $entry\n" if $verbose;
    }

    my @skycells = &select_skycells($entry, @$matches); # Matching skycells
    my $Nskycells = @skycells;
    printf STDERR "Nskycells: $Nskycells\n" if $verbose;
    foreach my $skycell (@skycells) {
        my %overlap = ();       # Overlap information for warptool
        $overlap{skycell_id} = $skycell;
        $overlap{tess_dir}   = $tess_dir;
        $overlap{cam_id}     = $imfile->{cam_id};
        $overlap{class_id}   = $imfile->{class_id};
        $overlap{fault}      = $imfile->{fault};
        push @$overlaps, \%overlap;

        printf STDERR "overlap: %s : %s , %s\n", $skycell, $imfile->{cam_id}, $imfile->{class_id} if $verbose;

        $unique_skycells->{$skycell} = 1;
    }

    return;
}

# Find skycells in the list that come from a particular entry
sub select_skycells
{
    my $entry = shift;          # File+Ext to search for
    my @list = @_;              # List of "File+Ext : skycell"

    my @skycells = ();
    my %unique = ();            # Ensure we only return unique skycells for this entry

    foreach my $line (@list) {
        if ($line =~ m|$entry|) {
            my ($skycell) = $line =~ m|$entry\S*\s+:\s+(\S+)|;
            if (not defined $unique{$skycell}) {
                push @skycells, $skycell;
                $unique{$skycell} = 1;
            }
        }
    }
    return @skycells;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
