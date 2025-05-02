#!/usr/bin/env perl

# this program injects a list of single-file exposures into the db,
# taking the filename (without .fits) as the exp_tag.  the user
# supplies a temporary telescope and camera name.  these are used
# for informational purposes only until the registration step can
# determine the true telescope and camera name from the image
# headers.

# this program should not fail because of the data format or the
# configuration, except for the very basic database setup.

use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use File::Spec;
use PS::IPP::Config;

my $ipprc = PS::IPP::Config->new(); # this is used for PATH, NEB filename conversions

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Parse the command-line arguments
my ($camera, $telescope, $workdir, $reduction, $dvo_db, $tess_id, $end_stage, $label, $dbname, $no_op, $help);
GetOptions('camera|i=s'     => \$camera,    # user-supplied camera name
	   'telescope|t=s'  => \$telescope, # user-supplied telescope name
	   'workdir|w=s'    => \$workdir,   # working directory for output files
	   'reduction=s'    => \$reduction, # user-supplied camera name
	   'dvodb=s'        => \$dvo_db,    # target dvo database 
	   'tess_id=s'      => \$tess_id,   # tessalation for warping
	   'end_stage=s'    => \$end_stage, # stop processing at this step
	   'label=s'        => \$label,     # set chip label
	   'dbname|d=s'     => \$dbname,    # Database name
	   'no-op'          => \$no_op,     # pretend but don't actually inject
	   'help'           => \$help       # give help listing
) or pod2usage( 2 );

pod2usage( -msg => "inject one or many files into the IPP pipeline database", 
	   -exitval => 2) if 
    defined $help;

pod2usage( -msg => "Usage: $0 --telescope (name) --camera (name) [--workdir path] [--reduction class] [--dvodb db] [--tess_id tess] [--end_stage stage] [--label label] [--dbname dbname] (files)", 
	   -exitval => 2 ) if 
    scalar @ARGV == 0;

pod2usage( -msg => "Required options: --telescope (name) --camera (name)",
	   -exitval => 3) unless
    defined $telescope and
    defined $camera;

my $pxinject = can_run('pxinject') or die "Can't find pxinject\n";

# if workdir is not defined, assign the current path
# XXX we need to handle relative paths for workdir (not allowed)
if (! $workdir) {
    $workdir = File::Spec->rel2abs( "." );
}

my $num = 0;
foreach my $file ( @ARGV ) {
    # check for file existence
    if (! -e $file) { die "file $file not found\n"; }
    my $absfile = File::Spec->rel2abs( $file );
    inject($absfile, $workdir, $dbname, $telescope, $camera);
    $num ++;
}

print "$num files injected.\n";

sub inject
{
    my $absfile = shift;	# absolute path for this file
    my $workdir  = shift;	# absolute path for output directory
    my $dbname = shift;		# IPP database to use
    my $telescope = shift;	# user-specified telescope
    my $camera = shift;	# user-specified camera

    # XXX provide an option for an alternative extension
    my ( $vol, $path, $name ) = File::Spec->splitpath( $absfile );
    my ( $exp_name ) = $name =~ /(.*)\.(fits|fit|fts)(|.gz)/;
    $exp_name =~ s|\s|_|g;

    my $relfile = $ipprc->convert_filename_relative( $absfile );

    # the telescope, instrument, and exp_name used here are temporary : register replaces them with the true values
    my $command_exp = "$pxinject -newExp";
    $command_exp .= " -tmp_exp_name $exp_name";
    $command_exp .= " -tmp_inst $camera";
    $command_exp .= " -tmp_telescope $telescope";
    $command_exp .= " -workdir $workdir";
    $command_exp .= " -reduction $reduction" if defined $reduction;
    $command_exp .= " -dvodb $dvo_db"       if defined $dvo_db;
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
    
    # the class_id used here is temporary : register replaces it with the true class_id
    my $command_imfile = "$pxinject -newImfile";
    $command_imfile .= " -exp_id $exp_id";
    $command_imfile .= " -tmp_class_id fpa";
    $command_imfile .= " -uri '$relfile'";
    $command_imfile .= " -dbname $dbname" if defined ($dbname);
    
    unless ($no_op) {
	my ( $success_imfile, $error_code_imfile, $full_buf_imfile, $stdout_buf_imfile, $stderr_buf_imfile ) = run( command => $command_imfile, verbose => 1 );
	die "Unable to inject $exp_name imfile: $error_code_imfile\n" if not $success_imfile;
    } else {
	print "skipping command: $command_imfile\n";
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

    return 1;
}


__END__
