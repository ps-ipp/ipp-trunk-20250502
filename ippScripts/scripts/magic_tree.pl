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

use Astro::FITS::CFITSIO qw( :constants );
Astro::FITS::CFITSIO::PerlyUnpacking(1);

use Math::Trig;
use File::Temp qw( tempfile );
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

use constant MAX_FIELDS => 4;   # Maximum number of fields to be in a node

# Look for programs we need
my $missing_tools;
my $magictool = can_run('magictool') or (warn "Can't find magictool" and $missing_tools = 1);
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($magic_id, $warp_id, $tess_id, $camera, $ra0, $dec0, $dbname, $outroot,
    $save_temps, $verbose, $no_update, $no_op, $logfile);
GetOptions(
           'magic_id=s'      => \$magic_id,   # Magic identifier
           'warp_id=s'       => \$warp_id,    # id for corresponding warps
           'tess_id=s'       => \$tess_id,    # Tessellation identifier
           'camera=s'        => \$camera,     # Camera name
           'ra=f'            => \$ra0,        # Boresight right ascension, radians
           'dec=f'           => \$dec0,       # Boresight declination, radians
           'dbname=s'        => \$dbname,     # Database name
           'outroot=s'       => \$outroot,    # Output root name
           'save-temps'      => \$save_temps, # Save temporary files?
           'verbose'         => \$verbose,    # Print stuff?
           'no-update'       => \$no_update,  # Don't update the database?
           'no-op'           => \$no_op,      # Don't do any operations?
           'logfile=s'       => \$logfile,   # Redirect output?
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --magic_id --camera --tess_id --ra --dec --outroot",
           -exitval => 3) unless
    defined $magic_id and
    defined $tess_id and
    defined $ra0 and
    defined $dec0 and
    defined $camera and
    defined $outroot;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $magic_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $magic_id, $PS_EXIT_SYS_ERROR ) if $logfile;

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

### Get a list of skycells
my @skycells;                   # List of skycells
{
    my $command = "$magictool -inputskyfile -magic_id $magic_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform magictool -inputskyfile: $error_code", $magic_id, $error_code);
    }

    my $magictool_output = join "", @$stdout_buf;
    # if there is no output from magictool that means that there are no
    # diffSkyfiles with non-zero quality. Set fault to a special value so
    # that these magicRuns can be recognized and dropped.
    &my_die("magictool -inputskyfile returned no output. Inputs are probably all bad quality.", $magic_id, 42)
        if !$magictool_output;
    my $metadata = $mdcParser->parse($magictool_output) or
        &my_die("Unable to parse metadata config doc", $magic_id, $PS_EXIT_PROG_ERROR);

    my $inputs = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $magic_id, $PS_EXIT_PROG_ERROR);

    foreach my $input ( @$inputs ) {
        push @skycells, $input; # NB: Storing the skycell_id in magicInputSkyfile.node
    }
}

### For each skycell, project centre of skycell onto tangent plane of boresight
my @fields;
foreach my $input ( @skycells ) {
    # We use the WCS in the diff image
    my $name = "PPSUB.OUTPUT"; # Name of file
    my $skycell_id = $input->{node}; # Name of skycell
    my $skyfile = $ipprc->filename($name, $input->{path_base}, $skycell_id); # Filename for diff
    my $skyfileResolved = $ipprc->file_resolve( $skyfile ); # Resolved filename

    my ($header, $status) = (undef, 0);
    my $fits =  Astro::FITS::CFITSIO::open_file( $skyfileResolved, READONLY, $status );
    &my_die("failed to open skycell file: $skyfileResolved: $status", $magic_id, $PS_EXIT_SYS_ERROR) if $status;

    ($header, $status) = Astro::FITS::CFITSIO::fits_read_header( $fits );

    &my_die("Unable to read skycell header: $status", $magic_id, $PS_EXIT_SYS_ERROR) if $status;


    # Get the useful header keywords
    my $naxis1 = $$header{'NAXIS1'};
    my $naxis2;
    if ($naxis1) {
        $naxis2 = $$header{'NAXIS2'} or &my_die("Can't find NAXIS2", $magic_id, $PS_EXIT_SYS_ERROR);
    } else {
        # if the skyfile is compressed then the WCS won't be in the primary header, move to the
        # extension
        my $hdutype;
        $fits->movrel_hdu(1, $hdutype, $status);
        &my_die("Unable to movrel_hdu: $status", $magic_id, $PS_EXIT_SYS_ERROR) if $status;

        ($header, $status) = Astro::FITS::CFITSIO::fits_read_header( $fits );
        &my_die("Unable to read extension header: $status", $magic_id, $PS_EXIT_SYS_ERROR) if $status;
        my $xtension = $$header{'XTENSION'} or &my_die("Can't find XTENSION", $magic_id, $PS_EXIT_SYS_ERROR);
        &my_die("XTENSION found: $xtension", $magic_id, $PS_EXIT_SYS_ERROR) if $xtension ne "'BINTABLE'";
        $naxis1 = $$header{'ZNAXIS1'} or &my_die("Can't find ZNAXIS1", $magic_id, $PS_EXIT_SYS_ERROR);
        $naxis2 = $$header{'ZNAXIS2'} or &my_die("Can't find ZNAXIS2", $magic_id, $PS_EXIT_SYS_ERROR);
    }
    my $ctype1 = $$header{'CTYPE1'} or &my_die("Can't find CTYPE1 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $ctype2 = $$header{'CTYPE2'} or &my_die("Can't find CTYPE2 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $cdelt1 = $$header{'CDELT1'} or &my_die("Can't find CDELT1 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $cdelt2 = $$header{'CDELT2'} or &my_die("Can't find CDELT2 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $crval1 = $$header{'CRVAL1'} or &my_die("Can't find CRVAL1 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $crval2 = $$header{'CRVAL2'} or &my_die("Can't find CRVAL2 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $crpix1 = $$header{'CRPIX1'} or &my_die("Can't find CRPIX1 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $crpix2 = $$header{'CRPIX2'} or &my_die("Can't find CRPIX2 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $pc11 = $$header{'PC001001'} or &my_die("Can't find PC001001 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $pc12 = $$header{'PC001002'} or &my_die("Can't find PC001002 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $pc21 = $$header{'PC002001'} or &my_die("Can't find PC002001 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $pc22 = $$header{'PC002002'} or &my_die("Can't find PC002002 in $skyfile", $magic_id, $PS_EXIT_SYS_ERROR);
    my $crota1 = $$header{'CROTA1'};
    my $crota2 = $$header{'CROTA2'};

    &my_die("Unexpected projection: $ctype1 and $ctype2.", $magic_id, $PS_EXIT_SYS_ERROR) unless
        $ctype1 =~ /^\'RA---TAN\s*\'$/ and $ctype2 =~ /^\'DEC--TAN\s*\'$/;
    &my_die("Can't determine size of skycell ($naxis1,$naxis2)", $magic_id, $PS_EXIT_SYS_ERROR) unless
        $naxis1 > 0 and $naxis2 > 0;
    &my_die("Can't determine scale of skycell ($cdelt1,$cdelt2)", $magic_id, $PS_EXIT_SYS_ERROR) if
        not defined $cdelt1 or $cdelt1 == 0 or not defined $cdelt2 or $cdelt2 == 0;
    &my_die("We don't know how to handle rotations ($crota1,$crota2)", $magic_id, $PS_EXIT_SYS_ERROR)
        if defined $crota1 or defined $crota2;

    # Relative coordinates of centre of the field
    my $x = $naxis1/2 - $crpix1;
    my $y = $naxis2/2 - $crpix2;

    # Coordinates on tangent plane
    my $xi = $pc11 * ($x) + $pc12 * ($y);
    my $eta = $pc21 * ($x) + $pc22 * ($y);
    $xi *= $cdelt1;
    $eta *= $cdelt2;

    # Coordinates on rotated celestial sphere
    my ($phi, $theta);
    if ($xi == 0 and $eta == 0) {
        $phi = 0;
        $theta = 0;
    } else {
        $phi = atan2($eta,$xi) + pi/2;
        my $denominator = sqrt($xi**2 + $eta**2);
        &my_die("denominator is zero!!", $magic_id, $PS_EXIT_PROG_ERROR) if $denominator == 0;
        $theta = atan(180 / pi / $denominator);
    }

    # Coordinates on celestial sphere
    $crval1 = deg2rad($crval1);
    $crval2 = deg2rad($crval2);
    my $ra = $crval1 + atan2(cos($theta) * sin($phi),
                             sin($theta) * cos($crval2) + cos($theta) * sin($crval2) * cos($phi));
    my $dec = asin(sin($theta) * sin($crval2) - cos($theta) * cos($crval2) * cos($phi));

    # Rotate to boresight
    my $phi_new = atan2(cos($dec) * sin($ra - $ra0),
                        sin($dec) * cos($dec0) + cos($dec) * sin($dec0) * cos($ra - $ra0));
    my $theta_new = asin(sin($dec) * sin($dec0) - cos($dec) * cos($dec0) * cos($ra - $ra0));

    # Project
    my $rad = 180 / pi * cot($theta_new);
    my $xi_new = $rad * sin($phi);
    my $eta_new = - $rad * cos($phi);

    my $field = { id => $skycell_id,
                  xi => $xi_new,
                  eta => $eta_new,
              };

    push @fields, $field;
}

### Subdivide list of positions into kd-tree
my $root = {                    # Root node of tree
    contents => \@fields,       # Contents of node
    position => 'root',         # Position in tree
    children => {},             # Children of node
};
my @tasks = ( $root );
while (scalar @tasks) {
    my $node = shift @tasks;
    divide_node($node, \@tasks);
}

### Format tree for magictool
my $mdcTree = print_node($root); # The tree in MDC format
my ($treeFile, $treeName) = tempfile( "magictree.${magic_id}.XXXX", UNLINK => !$save_temps );
print $treeFile $mdcTree;
close $treeFile;

### Input tree into database
if (!$no_update) {
    my $command = "$magictool -inputtree";
    $command   .= " -magic_id $magic_id";
    $command   .= " -dep_file $treeName";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform magictool -inputtree: $error_code", $magic_id, $error_code);
        }
    } else {
        print "Skipping command: $command\n";
    }
}

### Pau.

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $magic_id = shift;       # Magic identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $magic_id and not $no_update) {
        my $command = "$magictool -inputtree";
        $command .= " -magic_id $magic_id";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;
        system($command);
    }
    exit $exit_code;
}

# Divide a list into two, returning the lower and upper parts
sub divide_list
{
    my $list = shift;           # List to divide
    my $index = shift;          # Name of index for sorting

    my @sorted = sort { $$a{$index} <=> $$b{$index} } @$list; # Sorted list
    my $median = int(scalar @sorted / 2); # Median point of list
    my @upper = splice(@sorted, $median); # Upper part of the sorted list

#    print " median $median\n";

    return (\@sorted, \@upper);
}

# Create a new node, add it to the parent, and add it to the task list if required
sub new_node
{
    my $parent = shift;         # The parent node
    my $contents = shift;       # Contents of the new node
    my $position = shift;       # Position description
    my $tasks = shift;          # Tasks to do

    my $node = {
        contents => $contents,
        position => $parent->{position} . '_' . $position,
        children => {},
    };

#    my $n = scalar @$contents;
#    print "new node: $node->{position}: $n\n";

    $parent->{children}->{$position} = $node;

    push @$tasks, $node if scalar @$contents > 4;

    return $node;
}

# Divide a node
sub divide_node
{
    my $node = shift;           # Node to divide
    my $tasks = shift;          # Tasks to do

    my $position = $node->{position};

    my $contents = $node->{contents} or die "Can't find contents of node."; # Contents of node

    if ($position eq 'root' and scalar @$contents <= 4) {
        # don't need to divide, but do we need to sort?
        return;
    }

    my ($lower, $upper) = divide_list($contents, 'xi');

    if (scalar @$lower > 4) {
        my ($ll, $lr) = divide_list($lower, 'eta');
        new_node($node, $ll, 'll', $tasks);
        new_node($node, $lr, 'lr', $tasks);
    } else {
        new_node($node, $lower, 'L', $tasks);
    }

    if (scalar @$upper > 4) {
        my ($ul, $ur) = divide_list($upper, 'eta');
        new_node($node, $ul, 'ul', $tasks);
        new_node($node, $ur, 'ur', $tasks);
    } else {
        new_node($node, $upper, 'U', $tasks);
    }

    $node->{contents} = undef;

    return $node;
}

# Print the contents of a node
sub print_node
{
    my $node = shift;           # Node to print

    my $position = $node->{position}; # Position of node

    my $output = "$position\t\tMULTI\n"; # Output text

    if (defined $node->{contents}) {
        foreach my $field ( @{$node->{contents}} ) {
            my $skycell_id = $field->{id};      # Skycell name
            $output .= "$position\t\tSTR\t$skycell_id\n";
            $output .= "$skycell_id\t\tSTR\tNULL\t\# $field->{xi},$field->{eta}\n";
        }
    } else {
        foreach my $div ( keys %{$node->{children}} ) {
            $output .= "$position\t\tSTR\t${position}_$div\n";
            $output .= print_node($node->{children}->{$div});
        }
    }

    return $output;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
