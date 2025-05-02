#!/usr/bin/env perl

# this program injects a set of files the db, assuming the list of
# files all belong to a single exposure the user supplies a temporary
# telescope and camera name.  these are used for informational
# purposes only until the registration step can determine the true
# telescope and camera name from the image headers.

# this program should not fail because of the data format or the
# configuration, except for the very basic database setup.

use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use File::Spec;
use PS::IPP::Config 1.01 qw( :standard );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

my $ipprc = PS::IPP::Config->new(); # this is used for PATH, NEB filename conversions

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Parse the command-line arguments
my ($camera, $telescope, $workdir, $reduction, $dvo_db, $tess_id, $end_stage, $label, $from_db, $dbname, $no_op, $verbose, $help);
GetOptions('camera|i=s'     => \$camera,    # user-supplied camera name
	   'telescope|t=s'  => \$telescope, # user-supplied telescope name
	   'workdir|w=s'    => \$workdir,   # working directory for output files
	   'reduction=s'    => \$reduction, # user-supplied camera name
	   'dvodb=s'        => \$dvo_db,    # target dvo database 
	   'tess_id=s'      => \$tess_id,   # tessalation for warping
	   'end_stage=s'    => \$end_stage, # stop processing at this step
	   'label=s'        => \$label,     # set chip label
	   'from=s'         => \$from_db,   # Source Database name
	   'dbname|d=s'     => \$dbname,    # Database name
           'verbose'        => \$verbose,   # Print to stdout
	   'no-op'          => \$no_op,     # pretend but don't actually inject
	   'help'           => \$help       # give help listing
	   ) or pod2usage( 2 );

pod2usage( -msg => "inject files based on query to another IPP db", 
	   -exitval => 2) if 
    defined $help;

pod2usage( -msg => "Usage: $0 [--from dbname] [--workdir path] [--reduction class] [--dvodb db] [--tess_id tess] [--end_stage stage] [--label label] [--dbname dbname] (exp_name)", 
	   -exitval => 2 ) if 
    scalar @ARGV == 0;

pod2usage(
          -msg => "Required options: --exp_tag --cam_id --camera --outroot",
          -exitval => 3,
          ) unless
    defined $from_db;

my $exp_name = $ARGV[0];

my $pxinject = can_run('pxinject') or die "Can't find pxinject\n";
my $regtool = can_run('regtool') or die "Can't find regtool\n";

# tess_id needs to be an absolute path, or it must start with file://, path://, or neb://
if ($tess_id) {
    $tess_id = &fixpath ($tess_id);
}

# dvodb needs to be an absolute path, or it must start with file://, path://, or neb://
if ($dvo_db) {
    $dvo_db = &fixpath ($dvo_db);
}

# workdir needs to be an absolute path, or it must start with file://, path://, or neb://
if ($workdir) {
    $workdir = &fixpath ($workdir);
}

# if workdir is not defined, assign the current path
if (! $workdir) {
    $workdir = File::Spec->rel2abs( "." );
}

if (! $telescope) {
    $telescope = "UNKNOWN";
}

if (! $camera) {
    $camera = "UNKNOWN";
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# query the from_db for the desired imfiles:
# regtool -dbname $from_db -processedimfile -exp_name $exp_name

# Get list of component files
my $files;                      # Array of component files
{
    my $command = "$regtool -dbname $from_db -processedimfile -exp_name $exp_name"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        die "Unable to perform regtool for $exp_name: $error_code";
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        die "Unable to parse metadata config doc $exp_name: $error_code";

    # extract the metadata for the files into a hash list
    $files = parse_md_list($metadata) or
        die "Unable to parse metadata list: $exp_name: $error_code";
}

## inject the exposure

# the telescope, instrument, and exp_name used here are temporary : register replaces them with the true values
my $command_exp = "$pxinject -newExp";
$command_exp .= " -tmp_exp_name $exp_name";
$command_exp .= " -tmp_inst $camera";
$command_exp .= " -tmp_telescope $telescope";
$command_exp .= " -workdir $workdir";
$command_exp .= " -reduction $reduction" if defined $reduction;
$command_exp .= " -dvodb $dvo_db"        if defined $dvo_db;
$command_exp .= " -tess_id $tess_id"     if defined $tess_id;
$command_exp .= " -end_stage $end_stage" if defined $end_stage;
$command_exp .= " -label $label"         if defined $label;
$command_exp .= " -dbname $dbname"       if defined $dbname;

my $exp_id = 0;
unless ($no_op) {
    my ( $success_exp, $error_code_exp, $full_buf_exp, $stdout_buf_exp, $stderr_buf_exp ) =
	run( command => $command_exp, verbose => 1 );
    die "Unable to inject $exp_name: $error_code_exp\n" if not $success_exp;

    my @line = split(/\s+/, $$stdout_buf_exp[0]); # The output line, containing the exposure tag
    $exp_id = $line[2];	# The exposure tag
} else {
    print "skipping command: $command_exp\n";
}

# now inject the imfiles one at a time
foreach my $file (@$files) {

    my $uri = $file->{uri};
    my $class_id = $file->{class_id};

    # the class_id used here is temporary : register replaces it with the true class_id
    my $command_imfile = "$pxinject -newImfile";
    $command_imfile .= " -exp_id $exp_id";
    $command_imfile .= " -tmp_class_id $class_id";
    $command_imfile .= " -uri $uri";
    $command_imfile .= " -dbname $dbname" if defined ($dbname);
    
    unless ($no_op) {
	my ( $success_imfile, $error_code_imfile, $full_buf_imfile, $stdout_buf_imfile, $stderr_buf_imfile ) = run( command => $command_imfile, verbose => 1 );
	die "Unable to inject $exp_name imfile: $error_code_imfile\n" if not $success_imfile;
    } else {
	print "skipping command: $command_imfile\n";
    }
}

# the class_id used here is temporary : register replaces it with the true class_id
my $command_update = "$pxinject -updatenewExp";
$command_update .= " -exp_id $exp_id";
$command_update .= " -state run";
$command_update .= " -dbname $dbname" if defined ($dbname);

unless ($no_op) {
    my ( $success_update, $error_code_update, $full_buf_update, $stdout_buf_update, $stderr_buf_update ) = run( command => $command_update, verbose => 1 );
    die "Unable to update $exp_name: $error_code_update\n" if not $success_update;
} else {
    print "skipping command: $command_update\n";
}

exit 0;

sub fixpath {
    my $path = shift;
    my $valid = 0;
    $valid |= ($path =~ m|^file://|);
    $valid |= ($path =~ m|^path://|);
    $valid |= ($path =~ m|^neb://|);
    $valid |= ($path =~ m|^/|);

    if (!$valid) {
	$path = File::Spec->rel2abs( $path );
    }
    return $path;
}

__END__
