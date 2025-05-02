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

#
# Set up
################################################################################

my $missing_tools = 0;
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $dqstatstool = can_run('dqstatstool') or (warn "Can't find dqstatstool" and $missing_tools = 1);
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $stacktool= can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $magicdstool = can_run('magicdstool') or (warn "Can't find magicdstool" and $missing_tools = 1);
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $mkBTpcontrol = can_run('make_burntool_pcontrol.pl') or (warn "Can't find make_burntool_pcontrol.pl" and $missing_tools = 1);
my $moondata = can_run('moondata') or (warn "Can't find moondata" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $db;
my $debug = 0;
my %metadata_out = ();
$metadata_out{nsState} = 'UNDEFINEDERROR';
my $loghead;
chomp($loghead = `date`);
print STDERR 'Starting: ' . $loghead . ' ' . $0 . ' ' . (join ' ', @ARGV) . "\n";

# Grab options
my ( $date, $camera, $dbname, $logfile, $verbose, $manual);
my ( $help, $isburning, $force_stack_count, $test_mode, $this_target_only, $this_filter_only, $this_mode_only, $check_mode, $burntool_stats);
my ( $check_registration, $define_burntool, $queue_burntool, $check_chips, $queue_chips);
my ( $check_stacks, $queue_stacks, $check_sweetspot, $queue_sweetspot, $check_diffs, $queue_diffs, $clean_old);
my ( $check_detrends, $queue_detrends, $check_dqstats, $queue_dqstats);
my ( $confirm_stacks, $check_confirm_stacks );

GetOptions(
    'help|h'               => \$help,
    'date=s'               => \$date, # night to make stacks for
    'camera=s'             => \$camera, # camera
    'dbname=s'             => \$dbname, # Database name
    'logfile=s'            => \$logfile,
    'verbose'              => \$verbose,
    'debug'                => \$debug,
    'isburning'            => \$isburning,
    'force_stack_count'    => \$force_stack_count,
    'check'                => \$check_mode,
    'test_mode'            => \$test_mode,
    'burntool_stats'       => \$burntool_stats,
    'this_target_only=s'   => \$this_target_only,
    'this_filter_only=s'   => \$this_filter_only,
    'this_mode_only=s'     => \$this_mode_only,
    'check_registration'   => \$check_registration,
    'define_burntool'      => \$define_burntool,
    'queue_burntool'       => \$queue_burntool,
    'check_chips'          => \$check_chips,
    'queue_chips'          => \$queue_chips,
    'check_stacks'         => \$check_stacks,
    'queue_stacks'         => \$queue_stacks,
    'confirm_stacks'       => \$confirm_stacks,
    'check_confirm_stacks' => \$check_confirm_stacks,
    'check_sweetspot'      => \$check_sweetspot,
    'queue_sweetspot'      => \$queue_sweetspot,
    'check_detrends'       => \$check_detrends,
    'queue_detrends'       => \$queue_detrends,
    'check_dqstats'        => \$check_dqstats,
    'queue_dqstats'        => \$queue_dqstats,
    'check_diffs'          => \$check_stacks,
    'queue_diffs'          => \$queue_stacks,
    'clean_old'            => \$clean_old,
    ) or pod2usage ( 2 );
pod2usage( -msg =>
"USAGE: automate_stacks.pl <options...> <mode>
        Options:
           --help                 This help.
           --date YYYY-MM-DD      Work on this date (defaults to today GMT).
           --camera <camera>      Default GPC1.
           --dbname <db>          Default gpc1.
           --verbose
           --isburning            Signal that we are currently burntooling.
           --force_stack_count    Force the chip/warp counts.
           --this_target_only     Process only a single target.
           --this_filter_only     Process only a single filter.
           --this_mode_only       Process only a single clean mode.
           --burntool_stats       Display Nexp Nimfile Nburntooled Nqueued for check_chips.
        Modes:
           --check_registration   Confirm the data downloaded correctly.
           --define_burntool      Determine date ranges for burntool.
           --queue_burntool       Issue commands to burntool data.
           --check_chips          Confirm that chips can be built.
           --queue_chips          Issue chiptool commands to queue chips.
           --check_stacks         Confirm that stacks can be built.
           --queue_stacks         Issue stacktool commands to queue stacks.
           --check_sweetspot      See if we should queue SweetSpot stacks.
           --queue_sweetspot      Issue stacktool commands to queue SweetSpot stacks.
           --check_detrends       Confirm that detrend verify runs can be built.
           --queue_detrends       Issue dettool commands to queue detrend verify runs.
           --check_dqstats        Confirm that dqstats tables can be built.
           --queue_dqstats        Issue dqstatstool commands to queue dqstat tables.
           --check_diffs          Confirm that diffs can be done.
           --queue_diffs          Issue difftool commands to queue diffs.
           --clean_old            Mark old data 'goto_cleanup'.\n",
           -exitval => 2, ) if (defined($help));
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Choose a mode: --check_registration --check_burntool --queue_chips --queue_stacks --dbname",
          -exitval => 3,
          ) unless
    defined $check_registration or defined $define_burntool or defined $queue_burntool or
    defined $queue_chips or defined $queue_stacks or $queue_sweetspot or $queue_detrends or $queue_dqstats or 
    defined $check_chips or defined $check_stacks or $check_sweetspot or $check_detrends or $check_dqstats or
    defined $test_mode or defined $clean_old or defined $check_mode or
    defined $confirm_stacks or defined $burntool_stats or defined $dbname;

# Configurable parameters from our config file.
my @target_list = ();
my @filter_list = ();
my %distribution_list = ();
my %tessID_list = ();
my %obsmode_list = ();
my %object_list = ();
my %comment_list= ();
my %cleanmods_list = ();
my %stackable_list = ();
my %extra_processing = ();
my %reduction_class = ();
my %macro_formats = ();
my @unrecoverable_quality = ();
my @detrend_list = ();
my %dettype_list = ();
my %exptype_list = ();
my %refID_list = ();
my %refIter_list = ();
my %detfilter_list = ();
my %detmax_list = ();
my @mode_list = ();
my %clean_commands = ();
my %clean_retention = ();
my %noclean_list = ();
my %clean_alternate = ();
# Grab the configuration data.
my $conf_cmd = "$ppConfigDump -dump-recipe NIGHTLY_SCIENCE -";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $conf_cmd, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform ppConfigDump: $error_code", $date, $PS_EXIT_SYS_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new;
my $metadata = $mdcParser->parse(join "", @$stdout_buf);
foreach my $entry (@{ $metadata }) {
    if (@{ $entry }{name} eq 'CLEAN_MODES') {
	my @mode_data = @{ ${ $entry }{value} };
	my $this_mode = '';
	foreach my $mentry (@mode_data) {
	    if (${ $mentry }{name} eq 'MODE') {
		$this_mode = ${ $mentry }{value};
		push @mode_list, $this_mode;
	    }
	    elsif (${ $mentry }{name} eq 'COMMAND') {
		$clean_commands{$this_mode} = ${ $mentry }{value};
	    }
	    elsif (${ $mentry }{name} eq 'RETENTION_TIME') {
		$clean_retention{$this_mode} = ${ $mentry }{value};
	    }
	    elsif (${ $mentry }{name} eq 'ALTERNATE_CMD') {
		$clean_alternate{$this_mode} = ${ $mentry }{value};
	    }
	}
    }
    elsif (${ $entry }{name} eq 'FILTERS') {
        push @filter_list, ${ $entry }{value};
    }
    elsif (${ $entry }{name} eq 'UNRECOVERABLE_QUALITY') {
        push @unrecoverable_quality, ${ $entry }{value};
    }
    elsif (${ $entry }{name} eq 'MACRO_DEFINITIONS') {
	my @macro_data = @{ ${ $entry }{value} };
	my $this_mode = '';
	foreach my $mentry (@macro_data) {
	    if (${ $mentry }{name} eq 'PROC_MODE') {
		$this_mode = ${ $mentry }{value};
	    }
	    elsif (${ $mentry }{name} eq 'MACRO') {
		$macro_formats{$this_mode} = ${ $mentry }{value};
	    }
	}
    }
    elsif (${ $entry }{name} eq 'TARGETS') {
        my @target_data = @{ ${ $entry }{value} };
        my $this_target = '';
        foreach my $tentry (@target_data) {
            if (${ $tentry }{name} eq 'NAME') {
                $this_target = ${ $tentry }{value};
                push @target_list, $this_target;
		$distribution_list{$this_target} = $this_target;
            }
            elsif (${ $tentry }{name} eq 'DISTRIBUTION') {
                $distribution_list{$this_target} = ${ $tentry }{value};
            }
            elsif (${ $tentry }{name} eq 'TESS') {
                $tessID_list{$this_target} = ${ $tentry }{value};
            }
            elsif (${ $tentry }{name} eq 'OBSMODE') {
                $obsmode_list{$this_target} = ${ $tentry }{value};
            }
            elsif (${ $tentry }{name} eq 'OBJECT') {
                $object_list{$this_target} = ${ $tentry }{value};
            }
            elsif (${ $tentry }{name} eq 'COMMENT') {
                $comment_list{$this_target} = ${ $tentry }{value};
            }
            elsif (${ $tentry }{name} eq 'STACKABLE') {
                $stackable_list{$this_target} = ${ $tentry }{value};
            }
	    elsif (${ $tentry }{name} eq 'EXTRA_PROCESSING') {
		$extra_processing{$this_target} = ${ $tentry }{value};
	    }
	    elsif (${ $tentry }{name} eq 'REDUCTION') {
		$reduction_class{$this_target} = ${ $tentry }{value};
	    }
	    elsif (${ $tentry }{name} eq 'NOCLEAN') {
		$noclean_list{$this_target} = ${ $tentry }{value};
	    }
	    else {
		if (exists($clean_commands{ ${ $tentry }{name} })) {
		    $cleanmods_list{$this_target}{${ $tentry }{name} } = ${ $tentry }{value};
		}
	    }
        }
    }
    elsif (${ $entry }{name} eq 'DETRENDS') {
	my @detrend_data = @{ ${ $entry }{value} };
	my $this_detrend = '';
	foreach my $dentry (@detrend_data) {
	    if (${ $dentry }{name} eq 'NAME') {
		$this_detrend = ${ $dentry }{value};
		push @detrend_list, $this_detrend;
	    }
	    elsif (${ $dentry }{name} eq 'DETTYPE') {
		$dettype_list{$this_detrend} = ${ $dentry }{value};
	    }
	    elsif (${ $dentry }{name} eq 'EXPTYPE') {
		$exptype_list{$this_detrend} = ${ $dentry }{value};
	    }
	    elsif (${ $dentry }{name} eq 'REF_ID') {
		$refID_list{$this_detrend} = ${ $dentry }{value};
	    }
	    elsif (${ $dentry }{name} eq 'REF_ITER') {
		$refIter_list{$this_detrend} = ${ $dentry }{value};
	    }
	    elsif (${ $dentry }{name} eq 'FILTER') {
		$detfilter_list{$this_detrend} = ${ $dentry }{value};
	    }
	    elsif (${ $dentry }{name} eq 'MAX') {
		$detmax_list{$this_detrend} = ${ $dentry }{value};
	    }
	}
    }		
}

unless(defined($date)) {
    my @trash = gmtime;
    $trash[5] += 1900;
    $trash[4] += 1;
    $date = sprintf("%4d-%02d-%02d",$trash[5],$trash[4],$trash[3]);
}
unless(defined($camera)) {
    $camera = 'GPC1';
}

if (defined($this_target_only)) {
    foreach my $t (@target_list) {
        if ($t eq $this_target_only) {
            @target_list = ($this_target_only);
            last;
        }
    }
    die("$this_target_only is invalid.") if ($#target_list != 0);
}

if (defined($this_filter_only)) {
    foreach my $t (@filter_list) {
        if ($t eq $this_filter_only) {
            @filter_list = ($this_filter_only);
            last;
        }
    }
    die("$this_filter_only is invalid.") if ($#filter_list != 0);
}

if (defined($this_mode_only)) {
    foreach my $t (@mode_list) {
        if ($t eq $this_mode_only) {
            @mode_list = ($this_mode_only);
            last;
        }
    }
    die("$this_mode_only is invalid.") if ($#mode_list != 0);
}

#
# Mode selection
################################################################################
if (defined($test_mode)) {
    $debug = 1;
    my $z;
    foreach $z (@target_list) {
	print "TARGET: $z $tessID_list{$z} $obsmode_list{$z} $object_list{$z} $comment_list{$z} $stackable_list{$z} $reduction_class{$z}\n";
    }
    foreach $z (@filter_list) {
	print "FILTER: $z\n";
    }
    foreach $z (keys (%clean_commands)) {
	print "CLEAN: $z $clean_commands{$z} $clean_retention{$z}\n";
    }
    foreach $z (keys (%macro_formats)) {
	print "MACROS: $z $macro_formats{$z}\n";
    }
    
}

if (defined($check_registration) || defined($test_mode) || defined($check_mode)) {
    $metadata_out{nsState} = 'NEW';
    my ($Nsummit_exp,$Nfaults) = check_summit_copy($date);
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}

if (defined($check_dqstats) || defined($test_mode)) {
    $metadata_out{nsState} = 'CHECKDQSTATS';
    &execute_dqstats($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}
if (defined($queue_dqstats) || defined($test_mode)) {
    $metadata_out{nsState} = 'QUEUEDQSTATS';
    &execute_dqstats($date);
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}

if (defined($check_detrends) || defined($test_mode) || defined($check_mode)) {
    $metadata_out{nsState} = 'CHECKDETRENDS';
    &execute_detrends($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}
if (defined($queue_detrends)) {
    $metadata_out{nsState} = 'QUEUEDETRENDS';
    &execute_detrends($date);
    return_metadata($date);
    exit(0);
}

if (defined($define_burntool) || defined($test_mode)) {
    $metadata_out{nsState} = 'QUEUEBURNING';
    &find_burntool_ranges($date);
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}
if (defined($queue_burntool) || defined($test_mode)) {
    $metadata_out{nsState} = 'BURNING';
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}

if (defined($check_chips) || defined($test_mode) || defined($check_mode)) {
    $metadata_out{nsState} = 'QUEUECHIPS';
    &execute_chips($date,"pretend");
    if (($metadata_out{nsState} eq 'NEEDSBURNING')&&(defined($isburning))) {
        $metadata_out{nsState} = 'BURNING';
    }
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}
if (defined($queue_chips)) {
    $metadata_out{nsState} = 'TOWARP';
    &execute_chips($date);
    return_metadata($date);
    exit(0);
}

if (defined($check_stacks) || defined($test_mode) || defined($check_mode)) {
    $metadata_out{nsState} = 'TOWARP';
    &execute_stacks($date,"pretend");
    if ($metadata_out{nsState} eq 'FORCETOWARP') {
        $metadata_out{nsState} = 'TOWARP';
    }
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}
if (defined($queue_stacks)) {
    $metadata_out{nsState} = 'STACKING';
    &execute_stacks($date);
    if ($metadata_out{nsState} eq 'QUEUESTACKING') {
	$metadata_out{nsState} = 'STACKING_POSSIBLE_ERROR';
    }
    return_metadata($date);
    exit(0);
}

if (defined($check_confirm_stacks) || defined($test_mode) || defined($check_mode)) {
    $metadata_out{nsState} = 'CONFIRM_STACKING';
    &confirm_stacks($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}
if (defined($confirm_stacks)) {
    $metadata_out{nsState} = 'CONFIRM_STACKING';
    &confirm_stacks($date);
    return_metadata($date);
    exit(0);
}

if (defined($check_sweetspot) || defined($test_mode) || defined($check_mode)) {
    $metadata_out{nsState} = 'CHECKSWEETSPOT';
    &execute_sweetspot($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}
if (defined($queue_sweetspot)) {
    $metadata_out{nsState} = 'CHECKSWEETSPOT';
    &execute_sweetspot($date);
    return_metadata($date);
    exit(0);
}

if (defined($check_diffs) || defined($queue_diffs)) {
    die("Diffs are currently not implemented.");
}

if (defined($clean_old) || defined($test_mode)) {
    if (defined($test_mode)) {
        &execute_cleans($date,"pretend");
    }
    else {
        &execute_cleans($date);
    }
    unless (defined($test_mode)) { exit(0); }
}
exit(0);
#
# Registration
################################################################################

sub check_summit_copy {
    my $date = shift;
    my $db = init_gpc_db();

    # largely stolen from Bill's checkexp program.

    my $sth = " SELECT exp_name, summitExp.dateobs AS registered, rawExp.dateobs, summitExp.imfiles, ";
    $sth .= " summitExp.fault AS summit_fault, pzDownloadExp.state AS download_state, ";
    $sth .= " count(pzDownloadImfile.class_id) AS download_count, newExp.state AS newExp_state, newExp.exp_id, summitExp.exp_type";
    $sth .= " FROM summitExp LEFT JOIN pzDownloadExp USING(exp_name) LEFT JOIN pzDownloadImfile USING(exp_name) ";
    $sth .= " LEFT JOIN newExp ON exp_name = tmp_exp_name LEFT JOIN rawExp USING(exp_id, exp_name) ";
    $sth .= " WHERE date(summitExp.dateobs) >= '${date}T00:00:00' AND date(summitExp.dateobs) <= '${date}T23:59:59' ";
    $sth .= " GROUP BY exp_name ORDER BY summitExp.dateobs ";

    my $data_ref = $db->selectall_arrayref( $sth );

    my $Nsummit_exps = 0;
    my $Nsummit_faults = 0;
    my $Ndownload_faults = 0;
    my $Nregister_faults = 0;

#    my_trace($sth,$data_ref,$#{ $data_ref });

    foreach my $row_ref (@{ $data_ref }) {
        my ($exp_name,$registered,$dateobs,$imfiles,$summit_fault,
            $download_state,$download_count,$new_state,$exp_id,$exp_type) = @{ $row_ref };
        $Nsummit_exps++;
        if ($summit_fault) {
	    print STDERR "check_summit_copy: $date $exp_name has summit_fault $summit_fault";
            if (($exp_type ne 'OBJECT')||($exp_name =~ /^c.*/)) {
                print STDERR " (but I don't care).\n";
            }
            else {
                print STDERR "\n";
                $Nsummit_faults++;
            }
        }
        elsif (!$download_state or $download_state eq 'run') {
            print STDERR "check_summit_copy: $date $exp_name has download_state $download_state";
            if (($exp_type ne 'OBJECT')||($exp_name =~ /^c.*/)) {
                print STDERR " (but I don't care).\n";
            }
            else {
                print STDERR "\n";
                $Ndownload_faults++;
            }
        }
        elsif (!$new_state or $new_state eq 'run' ) {
            print STDERR "check_summit_copy: $date $exp_name has new_state $new_state";
            if (($exp_type ne 'OBJECT')||($exp_name =~ /^c.*/)) {
                print STDERR " (but I don't care).\n";
            }
            else {
                print STDERR "\n";
                $Nregister_faults++;
            }
        }
    }

    my $Nfaults = $Nsummit_faults + $Ndownload_faults + $Nregister_faults;
    if ($Nsummit_exps == 0) {
        print STDERR "No exposures were found on the summit for $date.\n";
        $metadata_out{nsState} = 'DROP';
    }
    elsif ($Nfaults != 0) {
        print STDERR "There were faults while downloading the exposures for $date.\n";
        $metadata_out{nsState} = 'NEW';
    }
    else {
        print STDERR "Summit copy and Registration have succeeded for $date.\n";
        $metadata_out{nsState} = 'REGISTERED';
    }

    return($Nsummit_exps,$Nfaults);
}

#
# Burntool
################################################################################

sub get_goodBTvalue {
    my $mdcParser = PS::IPP::Metadata::Config->new;

    my $config_cmd = "$ppConfigDump -camera $camera -get-key BURNTOOL.STATE.GOOD";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $config_cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", 0, 0, 0, $PS_EXIT_SYS_ERROR);
    }
    my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", 0, 0, 0, $PS_EXIT_SYS_ERROR);
    my $burntoolStateGood = 999;
    foreach my $cfg (@$recipeData) {
        if ($cfg->{name} eq 'BURNTOOL.STATE.GOOD') {
            $burntoolStateGood = $cfg->{value};
        }
    }
    if ($burntoolStateGood == 999) {
        &my_die("Failed to determine BURNTOOL.STATE.GOOD", $burntoolStateGood, 0, 0, $PS_EXIT_SYS_ERROR);
    }
    return($burntoolStateGood);
}

sub verify_burntool {
    my $exp_id = shift;
    my $burntoolStateGood = shift;

    my $db = init_gpc_db();

    my $sth = "SELECT exp_id,exp_name,obs_mode,dateobs,class_id,burntool_state,comment FROM rawImfile WHERE exp_id = $exp_id";
    my $data_ref = $db->selectall_arrayref( $sth );

    my $Nimfiles = 0;
    my $Nburntooled = 0;

    foreach my $row_ref (@{ $data_ref }) {
        my ($exp_id,$exp_name, $obs_mode,$dateobs,$class_id,$burntool_state,$comment) = @{ $row_ref };
        $Nimfiles++;
        if (abs($burntool_state) == $burntoolStateGood) {
            $Nburntooled++;
        }
    }
    return($Nimfiles,$Nburntooled);
}

sub find_burntool_ranges {
    my $date = shift;
    # Much cleaner than reimplementing that mess here
    my $command = "$mkBTpcontrol -d $date -b";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }

#    if ($#{ $stderr_buf } > -1) {
#        $metadata_out{nsState} = 'ERROR';
#        return(1);
#    }

    my $N = 0;
    my @burntool_entries = split /\n/, (join '', @$stdout_buf);
    foreach my $row (@burntool_entries) {
        my ($trash,$start,$end);
        (undef,$trash,$start,$end) = (split /\s+/,$row);
	if ($trash ne 'burntool') {
	    $metadata_out{nsState} = 'ERROR';
	    return(1);
	}
        for (my $class_counter = 0; $class_counter < 60; $class_counter++) {
            $metadata_out{"bt${N}Begin"} = $start;
            $metadata_out{"bt${N}End"} = $end;
            $metadata_out{"bt${N}Class"} = $class_counter;
            $metadata_out{"bt${N}Status"} = 'NEW';
            $N++;
        }
        print STDERR "define_burntool: $row\n";
    }
    $metadata_out{btN} = $N - 1;
    $metadata_out{btNCounter} = 0;
    add_to_macro_list('define_burntool',1,$date);
    return(0);
}

#
# Chips
################################################################################


sub construct_chiptool_cmd {
    my $date = shift;
    my $target = shift;

    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $select =  "-dateobs_begin ${date}T00:00:00 -dateobs_end ${date}T23:59:59 ";
    $date =~ s/-//g;

    my $cmd = "$chiptool";
    $cmd .= " -simple -dbname $dbname -definebyquery -set_end_stage warp ";
    $cmd .= " -set_label $label ";
    $cmd .= " -set_workdir $workdir -set_dist_group $dist_group ";
    $cmd .= " -set_tess_id $tess_id -set_data_group $data_group ";
    if (defined($obs_mode)) {
        $cmd .= " -obs_mode '$obs_mode' ";
    }
    if (defined($object)) {
        $cmd .= " -object '$object' ";
    }
    if (defined($comment)) {
        $cmd .= " -comment '$comment' ";
    }
    if (defined($reduction)) {
	$cmd .= " -set_reduction $reduction ";
    }
    $cmd .= " $select ";
    if ($debug == 1) {
        $cmd .= " -pretend ";
    }
    print STDERR "$cmd\n";
    return($cmd);
}

sub verify_uniqueness_chip {
    my $exp_id = shift;
    my $date = shift;
    my $target = shift;

    my $db = init_gpc_db();
    $date =~ s/-//g;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $sth = "SELECT exp_id from chipRun where data_group = '$data_group' AND exp_id = $exp_id";
    my $data_ref = $db->selectall_arrayref( $sth );

    return($#{ $data_ref } + 1);
}

sub pre_chip_queue {
    my $date = shift;
    my $target = shift;

    my $command = construct_chiptool_cmd($date,$target) . ' -pretend ';
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform chiptool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }

    my $burntoolStateGood = get_goodBTvalue();
    my $Nimfiles = 0;
    my $Nburntooled = 0;
    my $Nalready = 0;
    my @input_exposures = split /\n/, (join '', @$stdout_buf);
    foreach my $entry (@input_exposures) {
        my ($exp_id, @trash) = split /\s+/, $entry;
        @trash = verify_burntool($exp_id,$burntoolStateGood);
        $Nimfiles += $trash[0];
        $Nburntooled += $trash[1];

        @trash = verify_uniqueness_chip($exp_id,$date,$target);
        $Nalready += $trash[0];
    }
    return($#input_exposures + 1,$Nimfiles,$Nburntooled,$Nalready);
}

sub chip_queue {
    my $date = shift;
    my $target = shift;

    my $command = construct_chiptool_cmd($date,$target);
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform chiptool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }
    $metadata_out{nsState} = 'TOWARP';

    return(0);
}

sub execute_chips {
    my $date = shift;
    my $pretend = shift;
    my $exposures = 0;

    foreach my $target (@target_list) {
        my ($Nexposures,$Nimfiles,$Nburntooled,$Nalready) = pre_chip_queue($date,$target);
	if (defined($burntool_stats)) {
	    print "BTSTATS: $date $target $Nexposures $Nimfiles $Nburntooled $Nalready\n";
	}

        if ($Nexposures == 0) {
	    print STDERR "execute_chips: Target $target on $date had no exposures.\n";
	    next;
        }
        if ($Nalready != 0) {
	    print STDERR "execute_chips: Not queueing $target on $date due to already existing exposures.\n";
            next;
        }
        if ($Nimfiles != $Nburntooled) {
	    print STDERR "execute_chips: Target $target on $date is not fully burntooled.\n";
            $metadata_out{nsState} = 'NEEDSBURNING';
	    $exposures++;
            next;
        }
	$exposures++;
        unless(defined($pretend)) {
            chip_queue($date,$target);
        }
	if (defined($pretend)) {
	    add_to_macro_list('check_chips',$stackable_list{$target},$date,$target);
	}
	else {
	    add_to_macro_list('queue_chips',$stackable_list{$target},$date,$target);
	}
		    
    }
    if ($exposures == 0) {
	$metadata_out{nsState} = 'DROP';
    }
}


#
# DQstats
################################################################################

sub construct_dqstats_cmd {
    my $date = shift;

    my $select = "-dateobs_end ${date}T23:59:59 ";

    my $cmd = "$dqstatstool";
    $cmd .= " -simple -dbname $dbname -definebyquery ";
    $cmd .= " $select ";
    $cmd .= " -label %.nightlyscience ";
    $cmd .= " -set_label dqstats.nightlyscience ";
    if ($debug == 1) {
	$cmd .= ' -pretend ';
    }
    print STDERR "$cmd\n";
    return($cmd);
}

sub pre_dqstats_queue {
    my $date = shift;
    
    my $command = construct_dqstats_cmd($date) . ' -pretend ';
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dqstatstool: $error_code",1,1,,$date, $PS_EXIT_SYS_ERROR);
    }
    
    my @input_exposures = split /\n/, (join '', @$stdout_buf);

    return($#input_exposures + 1,1,1); 
}
 
sub dqstats_queue {
    my $date = shift;

    my $command = construct_dqstats_cmd($date);
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform dqstatstool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }
    return(0);

}
sub execute_dqstats {
    my $date = shift;
    my $pretend = shift;
    my ($Nexposures,$Nchips,$Ncams) = pre_dqstats_queue($date);
    if ($Nexposures == 0) {
	print STDERR "execute_dqstats: No exposures on date $date.\n";
    }
    elsif ($Ncams != $Nchips) {
	print STDERR "execute_dqstats: Not done processing data through camera stage.\n";
    }
    else {
	unless(defined($pretend)) {
	    dqstats_queue($date);
	}
	if (defined($pretend)) {
	    add_to_macro_list('check_dqstats',1,$date);
	}
	else {
	    add_to_macro_list('queue_dqstats',1,$date);
	}
    }
}   
#
# Detrend verification
################################################################################

sub construct_dettool_cmd {
    my $date = shift;
    my $target = shift;

    my ($label,$workdir,$filter,$exp_type,$det_type,$ref_det_id,$ref_iter,$maxN) = get_dettool_parameters($date,$target);
    
    my $select = "-select_dateobs_begin ${date}T00:00:00 -select_dateobs_end ${date}T23:59:59 ";
    my $use_limits = " -use_begin ${date}T00:00:00 -use_end ${date}T23:59:59 ";
    $date =~ s/-//g;

    my $cmd = "$dettool";
#    $cmd .= " -pretend ";
    $cmd .= " -simple -dbname $dbname -definebyquery -det_type $det_type ";
    $cmd .= " -mode verify -ref_det_id $ref_det_id -ref_iter $ref_iter ";
    $cmd .= " $select ";
    $cmd .= " -inst $camera ";
    $cmd .= " -select_exp_type $exp_type ";
    $cmd .= " -select_filter $filter " if defined($filter);
    $cmd .= " -workdir $workdir ";
    $cmd .= " -label $label ";
    $cmd .= " $use_limits ";
    if ($maxN > 0) {
	$cmd .= " -random_subset -random_limit $maxN ";
    }
    if ($debug == 1) {
	$cmd .= ' -pretend ';
    }
    print STDERR "$cmd\n";
    return($cmd);
}    

sub verify_uniqueness_detverify {
    my $date = shift;
    my $target = shift;

    my ($label,$workdir,$filter,$exp_type,$det_type,$ref_det_id,$ref_iter,$maxN) = get_dettool_parameters($date,$target);
    
    my $db = init_gpc_db();
    my $sth = "SELECT * FROM detRun WHERE workdir = '$workdir' AND ref_det_id = $ref_det_id AND ref_iter = $ref_iter";
    my $data_ref = $db->selectall_arrayref( $sth );

    return($#{ $data_ref } + 1);
}

sub pre_detrend_queue {
    my $date = shift;
    my $target = shift;
    
    my $command = construct_dettool_cmd($date,$target) . ' -pretend ';
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run ( command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform dettool: $error_code", 0,0, $date, $PS_EXIT_SYS_ERROR);
    }
    
    my @input_exposures = split /\n/, (join '', @$stdout_buf);
    return($#input_exposures + 1);
}

sub detrend_queue {
    my $date = shift;
    my $target = shift;
    
    my $command = construct_dettool_cmd($date,$target);
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run ( command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform chiptool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }
    $metadata_out{nsState} = 'DETREND_QUEUED';
    return(0);

}
sub execute_detrends {
    my $date = shift;
    my $pretend = shift;
    my $exposures = 0;
    foreach my $target (@detrend_list) {
	my ($Nexposures) = pre_detrend_queue($date,$target);
	if ($Nexposures == 0) {
	    print STDERR "execute_detrends: Target $target on $date had no exposures.\n";
	    next;
	}
	$exposures++;
	unless(defined($pretend)) {
	    detrend_queue($date,$target);
	}
	if (defined($pretend)) {
	    add_to_macro_list('check_detrends',1,$date,$target,"dettool");
	}
	else {
	    add_to_macro_list('queue_detrends',1,$date,$target,"dettool");
	}
    }
    if ($exposures == 0) {
	$metadata_out{nsState} = 'DETREND_DROP';
    }
    if (($metadata_out{nsState} eq 'CHECKDETRENDS') && ($exposures > 0)) {
	$metadata_out{nsState} eq 'QUEUEDETRENDS';
    }
}

#
# SweetSpot Stacking
################################################################################
sub construct_sweetspot_cmd {
    my $date = shift;
    my $target = 'SweetSpot';
    my $filter = 'w.00000';

    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    # Dateobs begin end?
    my ($dateobs_begin,$dateobs_end) = get_lunation_extent($date);
    my $select = "-select_dateobs_begin ${dateobs_begin}T00:00:00 -select_dateobs_end ${dateobs_end}T00:00:00";
    my $cmd = "$stacktool";
    $cmd .= " -simple -dbname $dbname -definebyquery ";
    $cmd .= " -set_label SweetSpot.refstack -select_label $label ";
    $cmd .= " -set_workdir $workdir -set_dist_group $dist_group ";
    $cmd .= " -select_filter $filter -set_data_group $data_group ";
    $cmd .= " -select_good_frac_min 0.1 -select_fwhm_major_max 8.0 ";
    $cmd .= " -min_num 7 -min_new 4";
    $cmd .= " $select ";
    if ($debug == 1) {
	$cmd .= ' -pretend ';
    }
    print STDERR "$cmd\n";
    return($cmd);
}

sub get_lunation_extent {
    my $date = shift;
    my ($year,$month,$day) = split /-/,$date;
    my $dateobs_begin;
    my $dateobs_end;

    my $dt = DateTime->new(year => $year, month => $month, day => $day,
			   hour => 0, minute => 0, second => 0, nanosecond => 0,
			   time_zone => 'Pacific/Honolulu');
    do {
	$dt->subtract(days => 1);
	my $ymd = $dt->ymd;
	my $md_cmd = "moondata $ymd 0 0";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run ( command => $md_cmd, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform moondata: $error_code", 0,0, $date, $PS_EXIT_SYS_ERROR);
	}
	my @result = split /\s+/,(join "\n", @$stdout_buf);
	if (abs($result[6]) <= 0.5) {
	    $dateobs_end = $ymd;
	}
    } while (!defined($dateobs_end));
    
    do {
	$dt->subtract(days => 1);
	my $ymd = $dt->ymd;
	my $md_cmd = "moondata $ymd 0 0";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run ( command => $md_cmd, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform moondata: $error_code", 0,0, $date, $PS_EXIT_SYS_ERROR);
	}
	my @result = split /\s+/,(join "", @$stdout_buf);
	if (abs($result[6]) <= 0.5) {
	    $dateobs_begin = $ymd;
	}
    } while (!defined($dateobs_begin));
	
    return($dateobs_begin,$dateobs_end);
}

sub pre_sweetspot_queue { 
    my $date = shift;
    my $command = construct_sweetspot_cmd($date) . ' -pretend ';
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform sweetspot stacktool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }
    my @stacks = split /\n/, (join '', @$stdout_buf);
    my $Nstacks = $#stacks + 1;
        
    return(1,$Nstacks);
}

sub sweetspot_queue {
    my $date = shift;
    my $command = construct_sweetspot_cmd($date);
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform sweetspot stacktool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }
    my @stacks = split /\n/, (join '', @$stdout_buf);
    my $Nstacks = $#stacks + 1;
    
    return(1,$Nstacks);
}

sub execute_sweetspot {    
    my $date = shift;
    my $pretend = shift;
    
    my ($is_lunation_date,$Nstacks) = pre_sweetspot_queue($date);
    if ($Nstacks == 0) {
	print STDERR "execute_sweetspot: No new stacks to make ($Nstacks)\n";
	$metadata_out{nsState} = 'SS_EMPTY';
	return();
    }
    if ($Nstacks < 10) {
	print STDERR "execute_sweetspot: Too few new stacks to make ($Nstacks)\n";
	$metadata_out{nsState} = 'SS_FEW';
	return();
    }
    if ($is_lunation_date == 0) {
	print STDERR "execute_sweetspot: Invalid lunation date. ($date $is_lunation_date)\n";
	$metadata_out{nsState} = 'SS_ERROR';
	return();
    }
    $metadata_out{nsState} = 'SS_QUEUE';
    $metadata_out{nsSweetSpot} = $Nstacks;
    unless(defined($pretend)) {
	$metadata_out{nsState} = 'SS_DONE';
	sweetspot_queue($date);
    }
    if (defined($pretend)) {
	add_to_macro_list('check_sweetspot',1,$date);
    }
    else {
	add_to_macro_list('queue_sweetspot',1,$date);
    }
}

#
# Stacking
################################################################################

sub construct_stacktool_cmd {
    my $date = shift;
    my $target = shift;
    my $filter = shift;

    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $select =  "-select_dateobs_begin ${date}T00:00:00 -select_dateobs_end ${date}T23:59:59 ";
    $date =~ s/-//g;

    my $cmd = "$stacktool";
#    $cmd .= ' -pretend -simple -dbname gpc1 -definebyquery -min_new 4 ';  # Probably silly, but I want to be safe and not overwrite
    $cmd .= " -simple -dbname $dbname -definebyquery ";
    $cmd .= " -set_label $label -select_label $label ";
    $cmd .= " -set_workdir $workdir -set_dist_group $dist_group ";
    $cmd .= " -select_filter $filter -set_data_group $data_group ";
    $cmd .= " -min_num 4";
    $cmd .= " -select_good_frac_min 0.05";
    $cmd .= " $select ";
    if ($debug == 1) {
        $cmd .= ' -pretend ';
    }
    print STDERR "$cmd\n";
    return($cmd);
}

sub verify_uniqueness_stack {
    my $skycell = shift;
    my $date = shift;
    my $target = shift;
    my $filter = shift;

    my $db = init_gpc_db();
    $date =~ s/-//g;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $sth = "SELECT skycell_id from stackRun where data_group = '$data_group' AND skycell_id = '$skycell' AND filter = '$filter' AND tess_id = '$tess_id'";
    my $data_ref = $db->selectall_arrayref( $sth );

    return($#{ $data_ref } + 1);
}

sub pre_stack_queue {
    my $date = shift;
    my $target = shift;
    my $filter = shift;

    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);
    # check warp stage == chip stage
    my $db = init_gpc_db();

    my $trunc_date = $date; $trunc_date =~ s/-//g;

    my $where = " label = '$label' AND data_group = '$data_group' ";

    my $where_possibly_faulted = $where . " AND ( " ;
    foreach my $acceptable_quality (@unrecoverable_quality) {
	$where_possibly_faulted .= " quality = $acceptable_quality OR ";
    }
    $where_possibly_faulted .= " 0 )";

    my $chip_sth = "SELECT * from chipRun WHERE (state = 'full' OR state = 'new') AND $where ";
    my $cam_sth  = "SELECT * from camRun JOIN camProcessedExp USING(cam_id) WHERE state = 'full' AND $where_possibly_faulted ";
    my $warp_sth = "SELECT * from warpRun WHERE state = 'full' AND $where ";

    my $chip_ref = $db->selectall_arrayref( $chip_sth );
    my $cam_ref  = $db->selectall_arrayref( $cam_sth );
    my $warp_ref = $db->selectall_arrayref( $warp_sth );

    # check that we will be able to queue them up
    my $command = construct_stacktool_cmd($date,$target,$filter) . ' -pretend ';
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform stacktool: $error_code", $#{ $chip_ref },$#{ $warp_ref },$date, $PS_EXIT_SYS_ERROR);
    }

    my $Nalready = 0;

    my @input_exposures = split /\n/, (join '', @$stdout_buf);
    foreach my $entry (@input_exposures) {
        my ($warp_tess_id,$skycell, @trash) = split /\s+/, $entry;
        @trash = verify_uniqueness_stack($skycell,$date,$target,$filter);
        $Nalready += $trash[0];
    }

    return($#input_exposures  + 1, $#{ $chip_ref } + 1, $#{ $cam_ref } + $#{ $warp_ref } + 2, $Nalready);
}

sub stack_queue {
    my $date = shift;
    my $target = shift;
    my $filter = shift;

    my $command = construct_stacktool_cmd($date,$target,$filter);
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $command, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform stacktool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }
    
    
    return(0);
}

sub execute_stacks {
    my $date = shift;
    my $pretend = shift;
    foreach my $target (@target_list) {
	foreach my $filter (@filter_list) {
	    if (exists($extra_processing{$target})) {
                my ($Nexposures,$NprocChips,$NprocWarps,$Nalready) = pre_stack_queue($date,$target,$filter);
                if ((!defined($force_stack_count))&&($NprocChips != $NprocWarps)) { # This makes me sad. :(
		    next;
		}
		if ($Nexposures == 0) {
		    if ($debug == 1) {
			print STDERR "execute_stacks: Target $target in filter $filter on $date has no exposures.\n";
		    }
                    next;
		}		    
		else {
		    do_extra_processing($date,$target,$filter,$pretend);
		}

	    }
	    if ($stackable_list{$target} == 1) {
                my ($Nexposures,$NprocChips,$NprocWarps,$Nalready) = pre_stack_queue($date,$target,$filter);
                if ((!defined($force_stack_count))&&($NprocChips != $NprocWarps)) { # This makes me sad. :(
		    if ($debug == 1) {
			print STDERR "execute_stacks: Target $target on $date is not fully processed. ($NprocChips $NprocWarps)\n";
		    }
                    $metadata_out{nsState} = 'FORCETOWARP';
                    next;
                }
                if ($Nexposures == 0) {
		    if ($debug == 1) {
			print STDERR "execute_stacks: Target $target in filter $filter on $date has no exposures.\n";
		    }
                    next;
                }
                if ($Nalready != 0) {
		    if ($debug == 1) {
			print STDERR "execute_stacks: Not queueing $target in filter $filter on $date due to already existing stacks.\n";
		    }
		    unless ($metadata_out{nsState} eq 'FORCETOWARP') {
			$metadata_out{nsState} = 'STACKING';
		    }
                    next;
                }
                unless (($metadata_out{nsState} eq 'FORCETOWARP')||($metadata_out{nsState} eq 'STACKING')) {
                    $metadata_out{nsState} = 'QUEUESTACKS';
                }
                unless(defined($pretend)) {
		    if ($debug == 1) {
			print STDERR "execute_stacks: Target $target in filter $filter on $date has exposures and will be queued.\n";
		    }
                    stack_queue($date,$target,$filter);
                }
		if (defined($pretend)) {
		    add_to_macro_list('check_stacks',$stackable_list{$target},$date,$target,$filter);
		}
		else {
		    add_to_macro_list('queue_stacks',$stackable_list{$target},$date,$target,$filter);
		}
            }
	    else {
		# print STDERR "execute_stacks: Target $target is not auto-stackable.\n";
	    }
        }
    }
}

sub post_stack_queue {
    my $date = shift;
    my $target = shift;

    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);
    # check warp stage == chip stage
    my $db = init_gpc_db();

    my $trunc_date = $date; $trunc_date =~ s/-//g;

    my $where = " label = '$label' AND data_group = '$data_group' ";

    my $stack_full_sth = "SELECT * from stackRun where $where ";
    my $stack_done_sth = "SELECT * from stackRun LEFT JOIN stackSumSkyfile USING(stack_id) WHERE $where ";
    $stack_done_sth .= " AND ((state = 'full') OR (state = 'new' && fault != 0)) ";
    if ($debug == 1) {
	print STDERR "post_stack_queue: database queries:\n";
	print STDERR "$stack_full_sth\n";
	print STDERR "$stack_done_sth\n";
    }
    my $stack_full_ref = $db->selectall_arrayref( $stack_full_sth );
    my $stack_done_ref = $db->selectall_arrayref( $stack_done_sth );

    return($#{ $stack_full_ref } + 1, $#{ $stack_done_ref } + 1);
}

sub confirm_stacks {
    my $date = shift;
    my $pretend = shift;
    
    foreach my $target (@target_list) {
        if ($stackable_list{$target} == 1) {
	    my ($Nstacks, $Nattempted) = post_stack_queue($date,$target);
	    if ($debug == 1) {
		print STDERR "confirm_stacks: Target $target on $date has $Nattempted attempts on $Nstacks.\n";
	    }
	    if ($Nstacks != $Nattempted) {
		if ($debug == 1) {
		    print STDERR "confirm_stacks: Target $target on $date is not done stacking. $Nstacks $Nattempted\n";
		}
		if ($metadata_out{nsState} eq 'CONFIRM_STACKING') {
		    $metadata_out{nsState} = 'STACKING';
		}
		next;
	    }
	    if ($Nstacks == 0) {
		if ($debug == 1) {
		    print STDERR "confirm_stacks: Target $target on $date has no stacks. Skipping.\n";
		}
		next;
	    }
	    if ($metadata_out{nsState} eq 'CONFIRM_STACKING') {
		if (defined($pretend)) {
		    add_to_macro_list('check_confirm_stacks',$stackable_list{$target},$date,$target);
		}
		else {
		    add_to_macro_list('confirm_stacks',$stackable_list{$target},$date,$target);
		}
	    }
	}
    }
}	    

#
# Extra processing
################################################################################

sub verify_uniqueness_diff {
    my $warp_id_1 = shift;
    my $warp_id_2 = shift;
    my $date = shift;
    my $target = shift;

    my $db = init_gpc_db();
    $date =~ s/-//g;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $sth = "SELECT diff_id from diffRun JOIN diffInputSkyfile USING(diff_id) where data_group = '$data_group' AND warp1 = $warp_id_1 AND warp2 = $warp_id_2";
    my $data_ref = $db->selectall_arrayref( $sth );

    return($#{ $data_ref } + 1);
}

sub do_extra_processing {
    my $date = shift;
    my $target = shift;
    my $filter = shift;
    my $pretend = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    if (($target eq 'OSS')||($target eq 'SweetSpot')) {
	my $db = init_gpc_db();

	my $obj_sth = "select DISTINCT rawExp.object from warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) ";
	$obj_sth .= " WHERE warpRun.state = 'full' AND warpRun.label = '$label' AND warpRun.data_group = '$data_group' AND rawExp.filter = '$filter' ORDER BY rawExp.object";
	print STDERR "$obj_sth\n";
	my $object_ref = $db->selectall_arrayref( $obj_sth );

	foreach my $object_row (@{ $object_ref }) {
	    my $this_object = shift @{ $object_row };
	    my $input_sth = "select exp_id,warp_id,dateobs from warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) ";
	    $input_sth .= " WHERE warpRun.state = 'full' AND warpRun.label = '$label' AND warpRun.data_group = '$data_group' AND rawExp.filter = '$filter' AND rawExp.object = '$this_object' ";
	    $input_sth .= " ORDER BY dateobs ";

	    my $warps = $db->selectall_arrayref( $input_sth );
	    
	    if (($#{ $warps } + 1) % 2 != 0) {
		print STDERR "Number of input warps to make OSS diffs is not even! $this_object $#{ $warps }\n";
#		last;
		next;
	    }
	    
	    while ($#{ $warps } > -1) {
		my $input_warp = shift @{ $warps };
		my $template_warp = shift @{ $warps };
		my $input_exp_id = ${ $input_warp }[0];
		my $template_exp_id = ${ $template_warp }[0];
		
		my $input_warp_id = ${ $input_warp }[1];
		my $template_warp_id = ${ $template_warp }[1];

		if (verify_uniqueness_diff($input_warp_id,$template_warp_id,$date,$target) != 0) {
		    print STDERR "Diffs already queued for this $date $target $input_exp_id $template_exp_id\n";
		    next;
		}

		my $cmd = "$difftool -dbname $dbname  -definewarpwarp ";
		$cmd .= "-input_label $label  -template_label $label ";
		$cmd .= "-backwards "; # Needed because difftool assumes a different date sorting.
		$cmd .= "-set_workdir $workdir  -set_dist_group $dist_group  -set_data_group $data_group ";
		$cmd .= " -simple  -set_label $label -exp_id $input_exp_id -template_exp_id $template_exp_id ";
#		$cmd .= " -pretend ";
		if (defined($pretend)) {
		    $cmd .= ' -pretend ';
		}
		if ($debug == 1) {
		    $cmd .= ' -pretend ';
		}
		print STDERR "EXTRA_PROCESSING: $cmd\n";
		if (($debug == 0)&&(!defined($pretend))) {
		    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
			run ( command => $cmd, verbose => $verbose );
		    unless ($success) {
			$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
			&my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
		    }
		}
	    }
	}
    }
}

	    


#
# Auto-Clean
################################################################################

sub construct_cleantool_args {
    my $date = shift;
    my $target = shift;
    my $mode = shift;

    my $command = $clean_commands{$mode};
    my $retention_time;
    if (exists($cleanmods_list{$target}{$mode})) {
	$retention_time = $cleanmods_list{$target}{$mode};
    }
    else {
	$retention_time = $clean_retention{$mode};
    }
    if ($retention_time <= 0) {
	return("no clean","true");
    }

    my ($year,$month,$day) = split /-/,$date;
    my $dt = DateTime->new(year => $year, month => $month, day => $day,
                               hour => 0, minute => 0, second => 0, nanosecond => 0,
                               time_zone => 'Pacific/Honolulu');
	
    $dt->subtract(days => $retention_time);
    my $cleaning_date = $dt->ymd;

    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($cleaning_date,$target);
    my $args = $command;
    if ((exists($clean_alternate{$mode})) && ($clean_alternate{$mode} eq 'A')) {
	$args .= " -dbname $dbname -updaterun -set_state goto_cleaned -full -set_label goto_cleaned -label $label -time_stamp_end $cleaning_date ";
    }
    elsif ((exists($clean_alternate{$mode})) && ($clean_alternate{$mode} eq 'B')) {
	$args .= " -dbname $dbname -updaterun -set_state goto_cleaned -state full -set_label goto_cleaned -label $label ";
    }
    else {
	$args .= " -dbname $dbname -updaterun -set_state goto_cleaned -state full -set_label goto_cleaned -label $label -data_group $data_group ";
    }
    if ($debug == 1) {
        $args .= ' -pretend ';
    }
    return($cleaning_date,$args);
}

sub execute_cleans {
    my $date = shift;
    my $pretend = shift;

    foreach my $mode (@mode_list) {
	foreach my $target (@target_list) {
	    if (exists($noclean_list{$target})) {
		next;
	    }
	    my ($cleaning_date,$command) = construct_cleantool_args($date,$target,$mode);
	    if ($cleaning_date eq 'no clean') {
		next;
	    }		
	    print STDERR "$command\n";
	    if (!(defined($pretend) || $debug == 1)) {
#           print STDERR "BEAR IS DRIVING!?\n";
		my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		    run ( command => $command, verbose => $verbose );
		unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		    &my_die("Unable to perform cleantool ($command): $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
		}
		add_to_macro_list('clean_old',1,$date,$target,$mode);
	    }
	}
    }
    return(0);
}

#
# Utilities
################################################################################

sub get_tool_parameters {
    my $date = shift;
    my $target = shift;
    my $workdir_date = $date; $workdir_date =~ s%-%/%g;
    my $trunc_date = $date; $trunc_date =~ s/-//g;

    my $label = "${target}.nightlyscience";
    my $workdir = "neb://\@HOST\@.0/${dbname}/${target}.nt/${workdir_date}";
    my $obs_mode = $obsmode_list{$target};
    my $object   = $object_list{$target};
    my $comment = $comment_list{$target};
    my $dist_group = $distribution_list{$target};
    my $data_group = "${target}.${trunc_date}";
    my $tess_id = $tessID_list{$target};
    my $reduction = $reduction_class{$target};
    return($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction);
}

sub get_dettool_parameters {
    my $date = shift;
    my $target = shift;
    my $workdir_date = $date; $workdir_date =~ s%-%/%g;
    my $trunc_date = $date; $trunc_date =~ s/-//g;

    my $exp_type = $exptype_list{$target};
    my $det_type = $dettype_list{$target};
    my $ref_det_id = $refID_list{$target};
    my $ref_iter = $refIter_list{$target};
    my $det_filter = $detfilter_list{$target};
    my $internal_filter;
    if (defined($det_filter)) {
	$internal_filter = $det_filter; $internal_filter =~ s/\..*//;
	$internal_filter = '.' . $internal_filter;
    }
    else {
	$internal_filter = '';
    }
    my $maxN = $detmax_list{$target};    
    
    my $lc_type = lc($exp_type);
    my $label = "${lc_type}${internal_filter}.$trunc_date";
    my $workdir = 'neb://@HOST@.0/' . $dbname . "/detverify.nt/${workdir_date}/${lc_type}${internal_filter}";
    return($label,$workdir,$det_filter,$exp_type,$det_type,$ref_det_id,$ref_iter,$maxN);
}

sub add_to_macro_list {
    my $proc_mode = shift;
    my $do_or_do_not = shift;
    my $date = shift;
    my $target = shift;
    my $mode = shift;

    if ((defined($macro_formats{$proc_mode}))&&($do_or_do_not)) {
	unless (defined($metadata_out{N_MACROS})) {
	    $metadata_out{N_MACROS} = 0;
	}
	my $N = $metadata_out{N_MACROS};
	$metadata_out{"ns${N}Macro"} = $macro_formats{$proc_mode};
	if ($debug == 1) {
	    print STDERR "WORKING ON A MACRO: ns${N}Macro $proc_mode $macro_formats{$proc_mode}\n";
	}
	if (defined($date)&&(defined($target))) {

	    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) 
		= get_tool_parameters($date,$target);
	    $metadata_out{"ns${N}Macro"} =~ s/\@LABEL\@/$label/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@WORKDIR\@/$workdir/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@OBS_MODE\@/$obs_mode/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@OBJECT\@/$object/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@COMMENT\@/$comment/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@TESS_ID\@/$tess_id/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@DIST_GROUP\@/$dist_group/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@DATA_GROUP\@/$data_group/;
	    $metadata_out{"ns${N}Macro"} =~ s/\@REDUCTION\@/$reduction/;
	}
	if (defined($mode)) {
	    $metadata_out{"ns${N}Macro"} =~ s/\@EXTRA\@/$mode/;
	}
	if (defined($date)) {
	    $metadata_out{"ns${N}Macro"} =~ s/\@DATE\@/$date/;
	}
	if ($debug == 1) {
	    print STDERR "DONE WITH A MACRO: ns${N}Macro $proc_mode $macro_formats{$proc_mode}\n";
	}
	$metadata_out{N_MACROS} ++;
    }
}

sub init_gpc_db {
    ## change to use the site.config setting now, however may want to use replicated scidbs instead
    my $ipprc =  PS::IPP::Config->new(); # IPP Configuration
    my $siteConfig = $ipprc->{_siteConfig};
    use constant DB_SOCKET => '/var/run/mysqld/mysqld.sock';
    # my $dbserver = 'ippdb01';
    my $dbuser = 'ippuser';
    my $dbpass = 'ippuser';
    my $dbserver = metadataLookupStr($siteConfig, 'DBSERVER');
    die "database configuration set up" unless defined($dbserver);
    $db = DBI->connect("DBI:mysql:database=${dbname};host=${dbserver};" .
                       "mysql_socket=" . DB_SOCKET(),
                       ${dbuser},${dbpass},
                       { RaiseError => 1, AutoCommit => 1}
        ) or die "Unable to connect to database $DBI::errstr\n";
    return($db);
}

sub return_metadata {
    my $date = shift;
    print STDOUT "autoStack MULTI\n\n";
    print STDOUT "autoStack METADATA\n";
    print STDOUT "   date                  STR          $date\n";
    foreach my $k (sort(keys %metadata_out)) {
        print STDOUT "   $k                STR          $metadata_out{$k}\n";
    }
    print STDOUT "END\n";
}

sub my_die {
    my $msg = shift; # Warning message on die
    my $stage = shift; # stage name
    my $stage_id = shift; #  identifier
    my $exit_code = shift; # Exit code
    # outputImage and path_base are globals

    carp($msg);
    exit $exit_code;
}

sub my_trace {
    if ($debug == 1) {
        foreach my $thing (@_) {
            carp($thing);
        }
    }
}
