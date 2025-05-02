#! /usr/bin/env perl

use warnings;
use strict;
use Carp;
use IPC::Cmd 0.36 qw( can_run run);
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use File::Temp qw( tempfile );

my $db;

my ($stage, $stage_id, $outroot, $camera, $dbname, $logfile, $verbose, $save_temps);
my $missing_tools;
my ($help,$masks);
my ($tempFile,$tempName,$maskFile,$maskName);
$verbose = 0;
my $ppSkycell = can_run('ppSkycell') or (warn "Can't find ppSkycell" and $missing_tools = 1);
my $warptool  = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $stacktool = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $difftool  = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
GetOptions(
    'help|h'          => \$help,
    'stage=s'         => \$stage,
    'stage_id=s'      => \$stage_id,
    'outroot=s'       => \$outroot,
    'dbname=s'        => \$dbname,
    'camera=s'        => \$camera,
    'logfile=s'       => \$logfile,
    'verbose'         => \$verbose,
    'save_temps'      => \$save_temps,
    'masks'           => \$masks,
#    'no-op'           => \$no_op,
#    'no-update'       => \$no_update,
    ) or pod2usage ( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2) if @ARGV;
pod2usage(
    -msg => "Required options: --stage --stage_id --outroot\nHelpful options: --camera --dbname",
    -exitval => 3,
    ) unless
    defined $stage and defined $stage_id and defined $outroot;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die("Unable to set up", $stage_id, $PS_EXIT_CONFIG_ERROR); # this is used for PATH, NEB filename conversions

my %stages = ('warp' => 1, 'stack' => 1, 'diff' => 1);

unless (exists($stages{$stage})) {
    my_die("Unknown stage '$stage'",$stage_id,$PS_EXIT_UNKNOWN_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

if ($stage eq 'warp') {
    my $imfiles;
    my $command = "$warptool -warped -warp_id $stage_id";
    if (defined($dbname)) {
	$command .= " -dbname $dbname";
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("unable to perform warptool: $error_code", $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
	# no results found;
    }
    
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("unable to parse metadata config doc", $stage_id, $error_code);

    my %tangents = ();
    
    foreach my $imfile (@$imfiles) {
	my $skycell_id = $imfile->{skycell_id};
	my $path_base  = $imfile->{path_base};
	my $quality    = $imfile->{quality};
	my $state      = $imfile->{data_state};
	my $projection_cell = $skycell_id;
	if ($quality != 0 or $state ne 'full') {
	    next;
	}

	$projection_cell =~ s/^(.*)\..*$/$1/;
	
	unless (exists($tangents{$projection_cell})) {
	    # Make a temp file and fill, but be sure to save 
	    ($tempFile, $tempName) = tempfile("/tmp/skycell.$projection_cell.XXXX",
						 UNLINK => !$save_temps);
	    $tangents{$projection_cell}{FILE} = $tempFile;
	    $tangents{$projection_cell}{NAME} = $tempName;
	    if ($masks) {
		my ($maskFile, $maskName) = tempfile("/tmp/skycell.$projection_cell.masks.XXXX",
						     UNLINK => !$save_temps);
		$tangents{$projection_cell}{MFILE} = $maskFile;
		$tangents{$projection_cell}{MNAME} = $maskName;
	    }		
	}
#	print "$skycell_id $projection_cell\n";	
	my $file = $ipprc->filename("PSWARP.OUTPUT", $path_base, $skycell_id);
#	print "$file $state $quality\n";
	my $f_fh = $tangents{$projection_cell}{FILE};
	print $f_fh "$file\n";
	if ($masks) {
	    my $mask = $ipprc->filename("PSWARP.OUTPUT.MASK", $path_base, $skycell_id);
#	    print "$mask\n";
	    my $m_fh = $tangents{$projection_cell}{MFILE};
	    print $m_fh "$mask\n";
	}
    }
    foreach my $projection_cell (keys %tangents) {
	$command = "$ppSkycell -images $tangents{$projection_cell}{NAME}";
	if ($masks) {
	    $command .= " -masks $tangents{$projection_cell}{MNAME} ";
	}
	$command .= " ${outroot}.${projection_cell} ";
#	print "$command\n";
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to perform ppSkycell: $error_code", $stage_id, $error_code);
	}
	# Update database:
	$command = "$warptool -addsummary -warp_id $stage_id -projection_cell $projection_cell -path_base $outroot";
	if (defined($dbname)) {
	    $command .= " -dbname $dbname";
	}
	
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to update warp summary: $error_code", $stage_id, $error_code);
	}
    }
    if (scalar (keys %tangents) == 0) {
	my $projection_cell = 'NULL';
	$command = "$warptool -addsummary -warp_id $stage_id -projection_cell $projection_cell -path_base $outroot";
	if (defined($dbname)) {
	    $command .= " -dbname $dbname";
	}
	
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to update diff summary: $error_code", $stage_id, $error_code);
	}
    }	
} #end warp stage
if ($stage eq 'diff') {
    my $imfiles;
    my $command = "$difftool -diffskyfile -diff_id $stage_id";
    if (defined($dbname)) {
	$command .= " -dbname $dbname";
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("unable to perform difftool: $error_code", $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
	# no results found;
    }
    
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("unable to parse metadata config doc", $stage_id, $error_code);

    my %tangents = ();
    
    foreach my $imfile (@$imfiles) {
	my $skycell_id = $imfile->{skycell_id};
	my $path_base  = $imfile->{path_base};
	my $quality    = $imfile->{quality};
	my $state      = $imfile->{data_state};
	my $projection_cell = $skycell_id;
	if ($quality != 0 or $state ne 'full') {
	    next;
	}

	$projection_cell =~ s/^(.*)\..*$/$1/;
	
	unless (exists($tangents{$projection_cell})) {
	    # Make a temp file and fill, but be sure to save 
	    ($tempFile, $tempName) = tempfile("/tmp/skycell.$projection_cell.XXXX",
						 UNLINK => !$save_temps);
	    $tangents{$projection_cell}{FILE} = $tempFile;
	    $tangents{$projection_cell}{NAME} = $tempName;
	    if ($masks) {
		my ($maskFile, $maskName) = tempfile("/tmp/skycell.$projection_cell.masks.XXXX",
						     UNLINK => !$save_temps);
		$tangents{$projection_cell}{MFILE} = $maskFile;
		$tangents{$projection_cell}{MNAME} = $maskName;
	    }		
	}
#	print "$skycell_id $projection_cell\n";	
	my $file = $ipprc->filename("PPSUB.OUTPUT", $path_base, $skycell_id);
#	print "$file $state $quality\n";
	my $f_fh = $tangents{$projection_cell}{FILE};
	print $f_fh "$file\n";
	if ($masks) {
	    my $mask = $ipprc->filename("PPSUB.OUTPUT.MASK", $path_base, $skycell_id);
#	    print "$mask\n";
	    my $m_fh = $tangents{$projection_cell}{MFILE};
	    print $m_fh "$mask\n";
	}
    }
    foreach my $projection_cell (keys %tangents) {
	$command = "$ppSkycell -images $tangents{$projection_cell}{NAME}";
	if ($masks) {
	    $command .= " -masks $tangents{$projection_cell}{MNAME} ";
	}
	$command .= " ${outroot}.${projection_cell} ";
#	print "$command\n";
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to perform ppSkycell: $error_code", $stage_id, $error_code);
	}
	# Update database:
	$command = "$difftool -addsummary -diff_id $stage_id -projection_cell $projection_cell -path_base $outroot";
	if (defined($dbname)) {
	    $command .= " -dbname $dbname";
	}
	
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to update diff summary: $error_code", $stage_id, $error_code);
	}
    }
    if (scalar (keys %tangents) == 0) {
	my $projection_cell = 'NULL';
	$command = "$difftool -addsummary -diff_id $stage_id -projection_cell $projection_cell -path_base $outroot";
	if (defined($dbname)) {
	    $command .= " -dbname $dbname";
	}
	
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to update diff summary: $error_code", $stage_id, $error_code);
	}
    }	
} #end diff stage
if ($stage eq 'stack') {
    my $imfiles;
    my $command = "$stacktool -sassskyfile -sass_id $stage_id";
    if (defined($dbname)) {
	$command .= " -dbname $dbname";
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("unable to perform stacktool: $error_code", $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
	# no results found;
    }
    
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("unable to parse metadata config doc", $stage_id, $error_code);

    my %tangents = ();

    my %products = ('image' => "PPSTACK.UNCONV",
#		    'mask'  => "PPSTACK.UNCONV.MASK",
		    'variance' => "PPSTACK.UNCONV.VARIANCE",
		    'exp'   => "PPSTACK.UNCONV.EXP",
		    'num'   => "PPSTACK.UNCONV.EXPNUM",
#		    'bkg'   => "PPSTACK.OUTPUT.BKGMODEL"
	);
    
    foreach my $imfile (@$imfiles) {
	my $skycell_id = $imfile->{skycell_id};
	my $path_base  = $imfile->{path_base};
	my $quality    = $imfile->{quality};
	my $state      = $imfile->{state};
	my $fault      = $imfile->{fault};
	my $projection_cell = $skycell_id;
	if ($quality != 0 or $state ne 'full') {
	    next;
	}

	$projection_cell =~ s/^(.*)\..*$/$1/;

	unless (exists($tangents{$projection_cell})) {
	    # Make a temp file and fill, but be sure to save 
	    foreach my $key (keys %products) {
		($tempFile, $tempName) = tempfile("/tmp/skycell.$projection_cell.$key.XXXX",
						  UNLINK => !$save_temps);
		$tangents{$projection_cell}{$key}{FILE} = $tempFile;
		$tangents{$projection_cell}{$key}{NAME} = $tempName;
		$tangents{$projection_cell}{$key}{N}    = 0;
	    }
	}
	foreach my $key (keys %products) {
#	    print "$skycell_id $projection_cell\n";	
	    my $file = $ipprc->filename($products{$key}, $path_base, $skycell_id);
#	    print "$file $state $quality\n";
	    my $f_fh = $tangents{$projection_cell}{$key}{FILE};
	    if ($ipprc->file_exists($file)) {
		print $f_fh "$file\n";
		$tangents{$projection_cell}{$key}{N} ++;
	    }
	}
    }
    foreach my $projection_cell (keys %tangents) {
	## Loop over results here.
	# Images
	# Masks
	# Variances
	# Nexptime
	# Nexp
	# Backgrounds
	
	foreach my $key (keys %products) {
	    $command = "$ppSkycell -images $tangents{$projection_cell}{$key}{NAME}";
	    $command .= " ${outroot}.${projection_cell}.${key} ";
	    if ($key eq 'bkg') {
		$command .= " -Di BIN1 1 -Di BIN2 1 ";
	    }
	    elsif ($key eq 'image') {
		$command .= " -exptimeOrder 1 ";
	    }
	    elsif ($key eq 'variance') { 
		$command .= " -exptimeOrder 2 ";
	    }
	    # Append the image list to other objects, in case the WCS information is unpopulated
	    $command .= " -wcsref $tangents{$projection_cell}{image}{NAME} ";

	    if ($tangents{$projection_cell}{$key}{N} > 0) {
		print "$command\n";
		( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
		unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
#		    print "Was unable to perform ppSkycell $stage_id $error_code $key\n";
		    &my_die("unable to perform ppSkycell: $error_code", $stage_id, $error_code);
		}
	    }
	}

	# Update database:
	$command = "$stacktool -addsummary -sass_id $stage_id -projection_cell $projection_cell -path_base $outroot";
	if (defined($dbname)) {
	    $command .= " -dbname $dbname";
	}
	
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to update stack summary: $error_code", $stage_id, $error_code);
	}
    }
    if (scalar (keys %tangents) == 0) {
	my $projection_cell = 'NULL';
	$command = "$stacktool -addsummary -sass_id $stage_id -projection_cell $projection_cell -path_base $outroot";
	if (defined($dbname)) {
	    $command .= " -dbname $dbname";
	}
#	print "$command\n";
	( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("unable to update stack summary: $error_code", $stage_id, $error_code);
	}
    }	

} #end stack stage


sub my_die
{
    my $msg = shift; # Warning message on die
    my $stage = shift; # stage name
    my $stage_id = shift; #  identifier
    my $exit_code = shift; # Exit code
    # outputImage and path_base are globals

    carp($msg);
    exit $exit_code;
}
