#!/usr/bin/env perl

# this program injects a list of multi-file exposures into the db.
# the program takes a list of directory names (dir), and injects all
# dir/*.fits.  It takes the directory name as the exp_tag.  the user
# supplies a temporary telescope and instrument name.  these are used
# for informational purposes only until the registration step can
# determine the true telescope and instrument name from the image
# headers.

# this program should not fail because of the data format or the
# configuration, except for the very basic database setup.

use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use File::Spec;
use PS::IPP::Config;

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Parse the command-line arguments
my ($camera, $telescope, $workdir, $reduction, $dvo_db, $tess_id, $end_stage, $dbname, $help);
GetOptions('camera|c=s'    => \$camera,    # Camera used	      
           'telescope|t=s' => \$telescope, # Telescope used      
           'workdir|w=s'   => \$workdir,   # working directory for output files
	   'reduction=s'   => \$reduction, # user-supplied camera name
	   'dvodb=s'       => \$dvo_db,    # target dvo database 
	   'tess_id=s'     => \$tess_id,   # tessalation for warping
	   'end_stage=s'   => \$end_stage, # stop processing at this step
    	   'dbname|d=s'    => \$dbname, # Database name
	   'help'          => \$help # give help listing
) or pod2usage( 2 );

pod2usage( -msg => "inject one or many image exposures (directories) into the IPP pipeline database", 
	   -exitval => 2) if 
    defined $help;

pod2usage( -msg => "Usage: $0 --telescope (name) --camera (name) [--workdir path] [--reduction class] [--dvodb db] [--tess_id tess] [--end_stage stage] [--dbname name] (files)", 
	   -exitval => 2 ) if 
    scalar @ARGV == 0;

pod2usage(
	  -msg => "Required options: --camera --telescope",
	  -exitval => 3) unless 
    defined $telescope and
    defined $camera;

# Look for programs we need
my $pxinject = can_run('pxinject') or die "Can't find pxinject\n";

# if workdir is not defined, assign the current path
# we need to handle relative paths for workdir (not allowed)
if (! $workdir) {
    $workdir = File::Spec->rel2abs( "." );
}

my $num = 0;
foreach my $filedir ( @ARGV ) {
    # check for filedir existence
    if (! -e $filedir) { die "file dir $filedir not found\n"; }
    my $absfiledir = File::Spec->rel2abs( $filedir );
    inject($absfiledir, $workdir, $dbname, $telescope, $camera);
    $num ++;
}

sub inject
{
    my $absfiledir = shift;	# absolute path for this file
    my $workdir  = shift;	# absolute path for output directory
    my $dbname = shift;		# IPP database to use
    my $telescope = shift;	# user-specified telescope
    my $camera = shift;	# user-specified camera

    # XXX provide an option for an alternative extension
    my @files = <$absfiledir/*.fits>;
    my $Nfiles = scalar @files;

    print "absfiledir: $absfiledir\n";
    print "Nfiles: $Nfiles\n";

    # the absfiledir is of the form path://PATH/data/foo/expname
    my ( $vol, $path, $exp_name ) = File::Spec->splitpath( $absfiledir );

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
    $command_exp .= " -dbname $dbname"       if defined $dbname;

    my ( $success_exp, $error_code_exp, $full_buf_exp, $stdout_buf_exp, $stderr_buf_exp ) =
	run( command => $command_exp, verbose => 1 );
    die "Unable to inject $exp_name: $error_code_exp\n" if not $success_exp;
    
    my @line = split(/\s+/, $$stdout_buf_exp[0]); # The output line, containing the exposure tag
    my $exp_id = $line[2];	# The exposure tag

    foreach my $absfile (@files) {

	my $relfile = $ipprc->convert_filename_relative( $absfile );

	my ( $tmpvol, $tmppath, $filename ) = File::Spec->splitpath( $absfile );
	my ( $class_name ) = $filename =~ /(.*)\.fits/;

	# the class_id used here is temporary : register replaces it with the true class_id
	my $command_imfile = "$pxinject -newImfile";
	$command_imfile .= " -exp_id $exp_id";
	$command_imfile .= " -tmp_class_id $class_name";
	$command_imfile .= " -uri $relfile";
	$command_imfile .= " -dbname $dbname" if defined ($dbname);
	
	my ( $success_imfile, $error_code_imfile, $full_buf_imfile, $stdout_buf_imfile, $stderr_buf_imfile ) = run( command => $command_imfile, verbose => 1 );
	die "Unable to inject $exp_name imfile: $error_code_imfile\n" if not $success_imfile;
    }

    # the class_id used here is temporary : register replaces it with the true class_id
    my $command_update = "$pxinject -updatenewExp";
    $command_update .= " -exp_id $exp_id";
    $command_update .= " -state run";
    $command_update .= " -dbname $dbname" if defined ($dbname);
    
    my ( $success_update, $error_code_update, $full_buf_update, $stdout_buf_update, $stderr_buf_update ) = run( command => $command_update, verbose => 1 );
    die "Unable to update $exp_name: $error_code_update\n" if not $success_update;

    return 1;
}

__END__
