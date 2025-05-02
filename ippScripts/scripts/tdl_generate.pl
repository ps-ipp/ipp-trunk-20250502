#!/usr/bin/env perl

# generate the transient detctions list files for a difference run
# using ipp_mops_translate.pl
# Note: The functionality in this script should probably just be incorporated
# into diff_skycell.pl since all the information needed is aviailable there

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
#print "\n\n";
#print "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Parse the command-line arguments
my ($diff_id, $skycell_id, $outroot, $camera, $dbname, $verbose, $save_temps);

GetOptions(
           'diff_id=s'       => \$diff_id,    # Magic identifier
           'skycell_id=s'    => \$skycell_id, # skycell_id name
           'camera=s'        => \$camera,     # Camera name
           'dbname=s'        => \$dbname,     # Database name
           'outroot=s'       => \$outroot,    # Output root name
           'save-temps'      => \$save_temps, # Save temporary files?
           'verbose'         => \$verbose,    # Print stuff?
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --diff_id --camera --outroot",
           -exitval => 3) unless
    defined $diff_id and
    defined $camera and
    defined $outroot;

$skycell_id = "" if !defined($skycell_id);

$ipprc->define_camera($camera);

# resolve any path:// or file:// in outroot
$outroot = $ipprc->file_resolve($outroot);

# Look for programs we need
my $missing_tools;
my $difftool      = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $ipp_mops_translate = can_run('ipp_mops_translate.pl') or (warn "Can't find ipp_mops_translate.pl" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

### Get a list of inputs
my $inputs;                     # List of inputs
{
    my $command = "$difftool -diffskyfile -diff_id $diff_id"; # Command to run
    $command .= " -skycell_id $skycell_id" if $skycell_id ne "";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform difftool -diffskyfile: $error_code", $diff_id, $skycell_id, $error_code);
    }

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $diff_id, $skycell_id, $PS_EXIT_PROG_ERROR);

    $inputs = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $diff_id, $skycell_id, $PS_EXIT_PROG_ERROR);
}

foreach my $skyfile (@$inputs) {
    next if $skyfile->{fault};
    my $path_base = $skyfile->{path_base};
    my $cmf = "${path_base}.cmf";
    my $diff_image_id = $skyfile->{diff_image_id};
    my $basename = basename($path_base);
    my $output = "$outroot/${basename}.tdl";
    $skycell_id = $skyfile->{skycell_id};
    my $command = "$ipp_mops_translate --input $cmf --output $output --extname SkyChip.psf --skycell $skyfile->{uri} --diff_image_id $diff_image_id";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $ipp_mops_translate: $error_code", $diff_id, $skycell_id, $error_code);
    }

}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $diff_id = shift;       # Magic identifier
    my $skycell_id = shift;           # Node name
    my $exit_code = shift;      # Exit code to add

    carp($msg);
    exit $exit_code;
}

__END__
