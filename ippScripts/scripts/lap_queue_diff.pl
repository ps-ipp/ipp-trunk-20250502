#!/usr/bin/env perl

use warnings;
use strict;
use Carp;
use IPC::Cmd 0.36 qw( can_run run);
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use DateTime;


my $missing_tools = 0;

my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $laptool  = can_run('laptool') or (warn "Can't find laptool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}


my ( $help, $verbose, $debug, $do_nothing);
my ( $camera, $dbname);
my ( $lap_id, $data_group );
my ( $queue_list );

GetOptions(
    'help|h'       => \$help,
    'verbose'      => \$verbose,
    'debug'        => \$debug,
    'do_nothing'   => \$do_nothing,

    'camera=s'     => \$camera,
    'dbname=s'     => \$dbname,
    
    'lap_id=s'     => \$lap_id,
    'data_group=s' => \$data_group,
    'queue_list=s' => \$queue_list,
    ) or pod2usage ( 2 );
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "--lap_id --data_group --queue_list are required",
    -exitval => 3,
    ) unless
    defined $lap_id and
    defined $queue_list and
    defined $data_group
;

unless (defined $dbname) {
    $dbname = 'gpc1';
}

my $mdcParser = PS::IPP::Metadata::Config->new;

log_message("Startup");

my %lapRunInfo = get_lapRun_info($lap_id);
unless (($lapRunInfo{state} eq 'full')||
	($lapRunInfo{state} eq 'done')) {
    &my_die("Cannot run diff_mode if lapRun != full!", $lap_id);
}
my $label = $lapRunInfo{label};
log_message("Have lapRun info");

my %projcell_done = ();
&get_completed_projcells($label);
log_message("Have completed projcells");

my %projcell_todo = ();
&get_projcell_set();
log_message("Have projcells todo");

my $Nexp = 0;
my $Ndiff= 0;
my $lapExp = get_lapExp_info($lap_id);
foreach my $exp (@{ $lapExp }) {
    log_message("  Exp " . $exp->{exp_id});
    $Nexp++;
    if ($exp->{data_state} eq 'full') {
	my $warp_id = get_warp_id($exp->{exp_id},$exp->{chip_id});
	my @warp_proj_cells = get_warp_projcells($warp_id);
	my $can_diff = check_diff_existance($warp_id,$label);
	if ($can_diff) {
	    foreach my $pc (@warp_proj_cells) {
		if (exists($projcell_todo{$pc})) { # We plan on doing this projection cell
		    unless (exists($projcell_done{$pc})) { # But we haven't done it?
			$can_diff = 0;
		    }
		}
	    }
	    if ($can_diff == 1) {
		# This needs a check to ensure it hasn't been done yet.
		my $diff_id = &launch_diff($warp_id,$label,$data_group);
		print "Made diff for $warp_id => $diff_id\n";
		$Ndiff++;
	    }
	}
	log_message("  Diff? $can_diff");
    }
}
print "LAP_ID: $lap_id\n";
print "NEXP:   $Nexp\n";
print "NDIFF:  $Ndiff\n";



sub log_message {
    my $message = shift;
    if ($verbose) {
	print "$message\n";
    }
}

sub get_lapRun_info {
    my $lap_id = shift;
    my $command = "$laptool -pendingrun -lap_id $lap_id";
    $command .= " -dbname $dbname " if defined $dbname;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform laptool -pendingrun: $error_code", $lap_id);
    }
    my $Runs = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata from laptool -pendingrun", $lap_id);
    # There should be only one.
    my $Run = ${ $Runs }[0];
    my %info = %{ $Run };
    return(%info);
}

sub get_lapExp_info {
    my $lap_id = shift;
    my $command = "$laptool -exposures -lap_id $lap_id";
    $command .= " -dbname $dbname" if defined $dbname;

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform laptool -exposures: $error_code", $lap_id);
    }
    
    if (@$stdout_buf == 0) {
        # Nothing to do. However, this is likely an error.
        return(0);
    }
    
    my $exposures = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata from laptool -exposures", $lap_id);
    return($exposures); # This is a pointer to an array of hashes.
}

sub get_completed_projcells {
    my $label = shift;
    my $command = "$laptool -listrun -label ${label} -state done ";
    $command .= " -dbname $dbname " if defined $dbname;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform laptool -pendingrun: $error_code", $lap_id);
    }
    my $Runs = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata from laptool -pendingrun", $lap_id);
    foreach my $r (@{ $Runs }) {
	$projcell_done{$r->{projection_cell}} = 1;
    }
}

sub get_projcell_set {
    open(Q,$queue_list) or &my_die("Cannot open queue list $queue_list",$lap_id);
    while(<Q>) {
	chomp;
	my $cmd = $_;
	my $projection_cell = $cmd;
	$projection_cell =~ s/.*?-projection_cell (skycell.\w+?) .*/$1/;
	$projcell_todo{$projection_cell} = 1;
    }
    close(Q);
}

sub get_warp_id {
    my $exp_id = shift;
    my $chip_id = shift;

    my $warptool_info_cmd = "$warptool -listrun -exp_id $exp_id -chip_id $chip_id";
    $warptool_info_cmd   .= " -dbname $dbname " if defined $dbname;
    
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $warptool_info_cmd, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform warptool -listrun: $error_code", $exp_id);
    }
    my $warps = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from warptool -listrun", $exp_id);
    # There should be only one.
    my $warp = ${ $warps }[0];
    my $warp_id = 0;
    if ($warp) {
	$warp_id = $warp->{warp_id};
    }
    if ($warp_id == 0) {
	&my_die("Unable to find warp_id",$exp_id);
    }
    return($warp_id);
}


sub get_warp_projcells {
    my $warp_id = shift;
    my %proj_cell_list = ();
    my $command = "$warptool -warped -warp_id ${warp_id}";
    $command .= " -dbname $dbname " if defined $dbname;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform laptool -pendingrun: $error_code", $lap_id);
    }
    my $Runs = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata from laptool -pendingrun", $lap_id);
    foreach my $r (@{ $Runs }) {
	my $projcell = $r->{skycell_id};
	$projcell =~ s/\.\d\d\d$//;
	$proj_cell_list{$projcell} = 1;
    }
    return(keys(%proj_cell_list));
}
    
sub check_diff_existance {
    my $warp_id = shift;
    my $label   = shift;
    my $command = "$difftool -listrun -warp_id ${warp_id} -label ${label}";
    $command .= " -dbname $dbname " if defined $dbname;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform laptool -pendingrun: $error_code", $lap_id);
    }
    my $Runs = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata from laptool -pendingrun", $lap_id);
    my $N = $#{ $Runs } + 1;
    if ($N == 0) {
	return(1);
    }
    else {
	return(0);
    }
}

sub launch_diff {
    my $warp_id = shift;
    my $label = shift;
    my $data_group = shift;
    my $command = "$difftool -definewarpstack -available -bothways -good_frac 0.2 ";
    $command   .= " -warp_label $label -stack_label $label -set_label $label ";
    $command   .= " -set_data_group $data_group ";
    $command   .= " -set_workdir neb://\@HOST\@.0/gpc1/${label}/${data_group}/ ";
    $command   .= " -set_reduction WARPSTACK_PV3 ";
    $command   .= " -warp_id ${warp_id} ";
    $command .= " -dbname $dbname " if defined $dbname;
    
    if (($debug)||($do_nothing)) {
	print "DEBUG! WOULD RUN: >> $command\n";
	return(99);
    }
    else {
	my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform laptool -pendingrun: $error_code", $lap_id);
	}
	my $Runs = $mdcParser->parse_list(join "", @$stdout_buf) or
	    &my_die("Unable to parse metadata from laptool -pendingrun", $lap_id);
	if ($#{ $Runs } != 0) {
	    print "Something has gone seriously wrong, and we've made more than one diff for warp_id $warp_id\n";
	}
	my $Run = ${ $Runs }[0];
	my $diff_id = $Run->{diff_id};
	return($diff_id);
    }
}

#
# Utilities
################################################################################

sub my_die {
    my $msg = shift; # Warning message on die
    my $lap_id = shift; # identifier
    my $optional = shift;
    my $exit_code = shift;
    unless (defined $exit_code) {
        $exit_code = $PS_EXIT_PROG_ERROR;
    }
    carp($msg);
    exit $exit_code;
}

# Check to see if a 64bit integer is NULL or not.  This is probably fragile, but I don't know how to get a S64 NULL out.
sub S64_IS_NOT_NULL {
    if ($_[0] == 9223372036854775807) {
        return(0);
    }
    else {
        return(1);
    }
}

