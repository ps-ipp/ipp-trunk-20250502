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
my $regtool = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
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

## for DB connection, want to use siteConfig DB settings
my $ipprc =  PS::IPP::Config->new(); # IPP Configuration
my $siteConfig = $ipprc->{_siteConfig};

my $db;
my $debug = 0;
my %metadata_out = ();
$metadata_out{nsState} = 'NIGHTLY_SCIENCE';
my $loghead;
chomp($loghead = `date`);
print STDERR 'Starting: ' . $loghead . ' ' . $0 . ' ' . (join ' ', @ARGV) . "\n";

# Grab options
my ( $date, $datetime, $now, $camera, $dbname, $logfile, $verbose, $manual,$desdiffdt);
my ( $help, $isburning, $force_stack_count, $force_diff_count, $force_registration, $test_mode, $this_target_only, $this_filter_only, $this_mode_only, $check_mode);
my ( $registration_status, $burntool_status, $observing_status, $old_date);
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
    'test_mode'            => \$test_mode,
    'force_stack_count'    => \$force_stack_count,
    'force_diff_count'     => \$force_diff_count,
    'force_registration'   => \$force_registration,
    'this_target_only=s'   => \$this_target_only,
    'this_filter_only=s'   => \$this_filter_only,
    'this_mode_only=s'     => \$this_mode_only,
    'registraion_status'   => \$registration_status,
    'burntool_status'      => \$burntool_status,
    'old_date=s'           => \$old_date,
    'check_stacks'         => \$check_stacks,
    'queue_stacks'         => \$queue_stacks,
    'confirm_stacks'       => \$confirm_stacks,
    'check_confirm_stacks' => \$check_confirm_stacks,
    'check_diffs'          => \$check_diffs,
    'queue_diffs'          => \$queue_diffs,
    'check_sweetspot'      => \$check_sweetspot,
    'queue_sweetspot'      => \$queue_sweetspot,
    'check_detrends'       => \$check_detrends,
    'queue_detrends'       => \$queue_detrends,
    'check_dqstats'        => \$check_dqstats,
    'queue_dqstats'        => \$queue_dqstats,
    'clean_old'            => \$clean_old,
    'desdiffdt=s'             => \$desdiffdt, # TdB20190529delay time for making desperate iddfs on the fly
    ) or pod2usage ( 2 );
pod2usage( -msg =>
"USAGE: nightlyscience.pl <options...> <mode>
        Options:
           --help                 This help.
           --date YYYY-MM-DD      Work on this date (defaults to today GMT).
           --camera <camera>      Default GPC1.
           --dbname <db>          Default gpc1.
           --verbose
           --force_stack_count    Force the chip/warp counts.
           --force_diff_count     Force the chip/warp counts.
           --force_registration   Force registration counts.
           --this_target_only     Process only a single target.
           --this_filter_only     Process only a single filter.
           --this_mode_only       Process only a single clean mode.
           --burntool_status      Display Nexp Nimfile Nburntooled Nqueued for check_chips.
           --desdiffdt <hrs>      Default 3hrs.
        Modes:
           --check_stacks         Confirm that stacks can be built.
           --queue_stacks         Issue stacktool commands to queue stacks.
           --check_diffs          Confirm that diffs can be done.
           --queue_diffs          Issue difftool commands to queue diffs.
           --check_sweetspot      See if we should queue SweetSpot stacks.
           --queue_sweetspot      Issue stacktool commands to queue SweetSpot stacks.
           --check_detrends       Confirm that detrend verify runs can be built.
           --queue_detrends       Issue dettool commands to queue detrend verify runs.
           --check_dqstats        Confirm that dqstats tables can be built.
           --queue_dqstats        Issue dqstatstool commands to queue dqstat tables.
           --clean_old            Mark old data 'goto_cleanup'.\n",
           -exitval => 2, ) if (defined($help));
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Choose a mode: --check_registration --check_burntool --queue_chips --queue_stacks",
          -exitval => 3,
          ) unless
    defined $check_registration or defined $define_burntool or defined $queue_burntool or
    defined $queue_diffs or defined $queue_stacks or $queue_sweetspot or $queue_detrends or $queue_dqstats or 
    defined $check_diffs or defined $check_stacks or $check_sweetspot or $check_detrends or $check_dqstats or
    defined $test_mode or defined $clean_old or defined $check_mode or
    defined $confirm_stacks or defined $burntool_status;
pod2usage(
          -msg => "Explicitly choose --dbname and --camera",
    -exitval => 3,
    ) unless (defined($camera) and defined($dbname));

# Configurable parameters from our config file.
my %science_config = ();
my %clean_config   = ();
my %detrend_config = ();
my %eon_config     = ();
my %cleanmods_list = ();
my %macro_config = ();
my @unrecoverable_quality = ();
my @filter_list = ();
my %unique_filter_hash = ();

#set up the camera config
my $ipprcam = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up camera", $PS_EXIT_CONFIG_ERROR ); # IPP camera configuration

# Grab the configuration data.
my $conf_cmd = "$ppConfigDump -camera $camera -dump-recipe NIGHTLY_SCIENCE -";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $conf_cmd, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform ppConfigDump: $error_code", 0, 0, $date, $PS_EXIT_SYS_ERROR);
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
            }
            elsif (${ $mentry }{name} eq 'COMMAND') {
                $clean_config{$this_mode}{COMMAND} = ${ $mentry }{value};
            }
            elsif (${ $mentry }{name} eq 'RETENTION_TIME') {
                $clean_config{$this_mode}{RETENTION_TIME} = ${ $mentry }{value};
            }
        }
    }
    elsif (${ $entry }{name} eq 'FILTERS') {
        $unique_filter_hash{ ${ $entry }{value} } = 1;
#        push @filter_list, ${ $entry }{value};
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
                $macro_config{$this_mode} = ${ $mentry }{value};
            }
        }
    }
    elsif (${ $entry }{name} eq 'TARGETS') {
        my @target_data = @{ ${ $entry }{value} };
        my $this_target = '';
        foreach my $tentry (@target_data) {
            if (${ $tentry }{name} eq 'NAME') {
                $this_target = ${ $tentry }{value};
                $science_config{$this_target}{DISTRIBUTION} = $this_target; # Set the default dist_group
            }
            else {
                $science_config{$this_target}{ ${ $tentry }{name} } = ${ $tentry }{value};
            }
        }
    }
    elsif (${ $entry }{name} eq 'DETRENDS') {
        my @detrend_data = @{ ${ $entry }{value} };
        my $this_detrend = '';
        foreach my $dentry (@detrend_data) {
            if (${ $dentry }{name} eq 'NAME') {
                $this_detrend = ${ $dentry }{value};
            }
            else {
                $detrend_config{$this_detrend}{ ${ $dentry }{name} } = ${ $dentry }{value};
            }
        }
    }           
    elsif (${ $entry }{name} eq 'END_OF_NIGHT') {
        my @eon_data = @{ ${ $entry }{value} };
        my $this_eon = '';
        foreach my $tentry (@eon_data) {
            if (${ $tentry }{name} eq 'NAME') {
                $this_eon = ${ $tentry }{value};
            }
            else {
                $eon_config{$this_eon}{ ${ $tentry }{name} } = ${ $tentry }{value};
            }
        }
    }
}
@filter_list = sort(keys(%unique_filter_hash));

if (defined($date)) {
    my $time;
    if ($date =~ / /) {
        ($date,$time) = split / /, $date;
    }
    elsif ($date =~ /T/) {
        ($date,$time) = split /T/, $date;
    }
    my ($year,$month,$day) = split /-/, $date;
    if (defined($time)) {
        my ($hour,$min,$sec) = split /\:/, $time; #/;
        $datetime = DateTime->new( year => $year,
                                   month => $month,
                                   day => $day,
                                   hour => $hour,
                                   min => $min,
                                   second => $sec,
                                   time_zone => 'UTC');
    }
    else {
        $datetime = DateTime->now(time_zone => 'UTC');
        $datetime->set_year($year);
        $datetime->set_day($day);
        $datetime->set_month($month);
    }
}   
else {
    $datetime = DateTime->now(time_zone => 'UTC'); # time_zone   => 'Pacific/Honolulu');
    $date = $datetime->ymd();
}
$now = DateTime->now(time_zone => 'UTC');

if (defined($this_target_only)) {
    foreach my $t (keys %science_config) {
        if ($t ne $this_target_only) {
            undef($science_config{$t});
        }
    }
    die("$this_target_only is invalid.") if (scalar keys %science_config < 1);
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
    foreach my $t (keys %clean_config) {
        if ($t ne $this_mode_only) {
            undef($clean_config{$t});
        }
    }
    die("$this_mode_only is invalid.") if (scalar keys %clean_config < 1);
}

$metadata_out{nsObservingState} = &get_observing_state($date);
$metadata_out{nsRegistrationState} = &get_registration_state($date);
if ($force_registration) {
    $metadata_out{nsRegistrationState} = 'REGISTERED';
}

if (defined($desdiffdt) == 0) {
        $desdiffdt = 1.5;
}

#Build in a safety for desdiffdt
if ((defined($desdiffdt) == 1) && ($desdiffdt <= 0.5)) {
        $desdiffdt = 0.5;
}

#
# Mode selection
################################################################################

if (defined($test_mode)) {
    $debug = 1;
    my $z;
    my $v;
    foreach $z (sort (keys %science_config)) {
        foreach $v (keys %{ $science_config{$z} }) {
            print "SCIENCE: $z $v $science_config{$z}{$v}\n";
        }
    }
    foreach $z (sort (keys %detrend_config)) {
        foreach $v (keys %{ $detrend_config{$z} }) {
            print "DETREND: $z $v $detrend_config{$z}{$v}\n";
        }
    }
    foreach $z (keys (%clean_config)) {
        foreach $v (keys %{ $clean_config{$z} }) {
            print "CLEAN: $z $v $clean_config{$z}{$v}\n";
        }
    }
    foreach $z (@filter_list) {
        print "FILTER: $z\n";
    }
    foreach $z (keys (%macro_config)) {
        print "MACROS: $z $macro_config{$z}\n";
    }
}
if (defined($observing_status) || defined($test_mode)) {
    $metadata_out{nsState} = 'CHECK_OBSERVING_STATUS';
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}

if (defined($registration_status) || defined($test_mode)) {
    &check_summit_copy($date);
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}

if (defined($burntool_status) || defined($test_mode)) {
    &burntool_status($date);
    unless (defined($test_mode)) { exit(0); }
}

if (defined($check_dqstats) || defined($test_mode)) {
    &execute_dqstats($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}
if (defined($queue_dqstats)) {
    &execute_dqstats($date);
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}

if (defined($check_detrends) || defined($test_mode)) {
    $metadata_out{nsDetState} = 'CHECKDETRENDS';
    &execute_detrends($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}
if (defined($queue_detrends)) {
    $metadata_out{nsDetState} = 'QUEUEDETRENDS';
    &execute_detrends($date);
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
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

if (defined($check_stacks) || defined($test_mode)) {
    $metadata_out{nsStackState} = 'TOWARP';
    &execute_stacks($date,$metadata_out{nsObservingState},"pretend");
    if ($metadata_out{nsStackState} eq 'FORCETOWARP') {
        $metadata_out{nsStackState} = 'TOWARP';
    }
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}
if (defined($queue_stacks)) {
    $metadata_out{nsStackState} = 'STACKING';
    &execute_stacks($date,$metadata_out{nsObservingState});
    if ($metadata_out{nsStackState} eq 'QUEUESTACKING') {
        $metadata_out{nsStackState} = 'STACKING_POSSIBLE_ERROR';
    }
    return_metadata($date);
    exit(0);
}

if (defined($check_diffs) || defined($test_mode)) {
    $metadata_out{nsDiffState} = 'TOWARP';
    &execute_diffs($date,$metadata_out{nsObservingState},"pretend");
    if ($metadata_out{nsDiffState} eq 'FORCETOWARP') {
        $metadata_out{nsDiffState} = 'TOWARP';
    }
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}
if (defined($queue_diffs)) {
    $metadata_out{nsDiffState} = 'DIFFING';
    &execute_diffs($date,$metadata_out{nsObservingState});
    if ($metadata_out{nsDiffState} eq 'QUEUESTACKING') {
        $metadata_out{nsDiffState} = 'STACKING_POSSIBLE_ERROR';
    }
    return_metadata($date);
    exit(0);
}

if (defined($check_confirm_stacks) || defined($test_mode)) {
    $metadata_out{nsStackState} = 'CONFIRM_STACKING';
    &confirm_stacks($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode)) { exit(0); }
}
if (defined($confirm_stacks)) {
    $metadata_out{nsStackState} = 'CONFIRM_STACKING';
    &confirm_stacks($date);
    return_metadata($date);
    exit(0);
}
exit(10);
if (defined($check_sweetspot) || defined($test_mode) || defined($check_mode)) {
    $metadata_out{nsSSState} = 'CHECKSWEETSPOT';
    &execute_sweetspot($date,"pretend");
    return_metadata($date);
    unless (defined($test_mode) || defined($check_mode)) { exit(0); }
}
if (defined($queue_sweetspot)) {
    $metadata_out{nsSSState} = 'CHECKSWEETSPOT';
    &execute_sweetspot($date);
    return_metadata($date);
    exit(0);
}

exit(0);
#
# Registration
################################################################################
# This isn't used, but might be useful to keep.

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
    my $Ndownload_imfiles = 0;
    my $Nsummit_imfiles = 0;
#    my_trace($sth,$data_ref,$#{ $data_ref });

    foreach my $row_ref (@{ $data_ref }) {
        my ($exp_name,$registered,$dateobs,$imfiles,$summit_fault,
            $download_state,$download_count,$new_state,$exp_id,$exp_type) = @{ $row_ref };
        $Nsummit_imfiles += $imfiles;
        $Ndownload_imfiles += $download_count;
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
        $metadata_out{nsRegState} = 'DROP';
    }
    elsif ($Nfaults != 0) {
        print STDERR "There were faults while downloading the exposures for $date.\n";
        $metadata_out{nsRegState} = 'NEW';
    }
    elsif ($Ndownload_imfiles != $Nsummit_imfiles) {
        print STDERR "Not done downloading from the summit for $date (Summit: $Nsummit_imfiles, Downloaded: $Ndownload_imfiles)\n";
        $metadata_out{nsRegState} = 'NEW';
    }
    else {
        print STDERR "Summit copy and Registration have succeeded for $date.\n";
        $metadata_out{nsRegState} = 'REGISTERED';
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

#
# Chips
################################################################################
# This no longer queues chips, it only checks the counts and returns that.
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

sub check_chip_status {
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

sub burntool_status {
    my $date = shift;
    my $exposures = 0;

    foreach my $target (sort (keys %science_config)) {
        my ($Nexposures,$Nimfiles,$Nburntooled,$Nalready) = check_chip_status($date,$target);
        print "BTSTATS: $date $target $Nexposures $Nimfiles $Nburntooled $Nalready\n";
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
    $metadata_out{nsDetState} = 'DETREND_QUEUED';
    return(0);

}
sub execute_detrends {
    my $date = shift;
    my $pretend = shift;
    my $exposures = 0;
    foreach my $target (sort (keys %detrend_config)) {
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
        $metadata_out{nsDetState} = 'DETREND_DROP';
    }
    if (($metadata_out{nsDetState} eq 'CHECKDETRENDS') && ($exposures > 0)) {
        $metadata_out{nsDetState} eq 'QUEUEDETRENDS';
    }
}

# #
# # SweetSpot Stacking
# ################################################################################
# sub construct_sweetspot_cmd {
#     my $date = shift;
#     my $target = 'SweetSpot';
#     my $filter = 'w.00000';

#     my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

#     # Dateobs begin end?
#     my ($dateobs_begin,$dateobs_end) = get_lunation_extent($date);
#     my $select = "-select_dateobs_begin ${dateobs_begin}T00:00:00 -select_dateobs_end ${dateobs_end}T00:00:00";
#     my $cmd = "$stacktool";
#     $cmd .= " -simple -dbname $dbname -definebyquery ";
#     $cmd .= " -set_label SweetSpot.refstack -select_label $label ";
#     $cmd .= " -set_workdir $workdir -set_dist_group $dist_group ";
#     $cmd .= " -select_filter $filter -set_data_group $data_group ";
#     $cmd .= " -select_good_frac_min 0.1 -select_fwhm_major_max 8.0 ";
#     $cmd .= " -min_num 7 -min_new 4";
#     $cmd .= " $select ";
#     if ($debug == 1) {
#       $cmd .= ' -pretend ';
#     }
#     print STDERR "$cmd\n";
#     return($cmd);
# }

sub get_lunation_extent {
    my $date = shift;
    my ($year,$month,$day) = split /-/,$date;
    my $dateobs_begin;
    my $dateobs_end;

    my $dt = DateTime->new(year => $year, month => $month, day => $day,
                           hour => 0, minute => 0, second => 0, nanosecond => 0,
                           time_zone => 'Pacific/Honolulu');
    do {
        $dt->add(days => 1);
        my $ymd = $dt->ymd;
        my $md_cmd = "moondata $ymd 0 0";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run ( command => $md_cmd, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform moondata: $error_code", 0,0, $date, $PS_EXIT_SYS_ERROR);
        }
        my @result = split /\s+/,(join "\n", @$stdout_buf);
        if (abs($result[6]) <= 0.6) {
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
        if (abs($result[6]) <= 0.6) {
            $dateobs_begin = $ymd;
        }
    } while (!defined($dateobs_begin));
       
    return($dateobs_begin,$dateobs_end);
}

# sub pre_sweetspot_queue {
#     my $date = shift;
#     my $command = construct_sweetspot_cmd($date) . ' -pretend ';
#     my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
#       run ( command => $command, verbose => $verbose );
#     unless ($success) {
#         $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
#         &my_die("Unable to perform sweetspot stacktool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
#     }
#     my @stacks = split /\n/, (join '', @$stdout_buf);
#     my $Nstacks = $#stacks + 1;
       
#     return(1,$Nstacks);
# }

# sub sweetspot_queue {
#     my $date = shift;
#     my $command = construct_sweetspot_cmd($date);
#     my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
#       run ( command => $command, verbose => $verbose );
#     unless ($success) {
#         $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
#         &my_die("Unable to perform sweetspot stacktool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
#     }
#     my @stacks = split /\n/, (join '', @$stdout_buf);
#     my $Nstacks = $#stacks + 1;
   
#     return(1,$Nstacks);
# }

# sub execute_sweetspot {   
#     my $date = shift;
#     my $pretend = shift;
   
#     my ($is_lunation_date,$Nstacks) = pre_sweetspot_queue($date);
#     if ($Nstacks == 0) {
#       print STDERR "execute_sweetspot: No new stacks to make ($Nstacks)\n";
#       $metadata_out{nsSSState} = 'SS_EMPTY';
#       return();
#     }
#     if ($Nstacks < 10) {
#       print STDERR "execute_sweetspot: Too few new stacks to make ($Nstacks)\n";
#       $metadata_out{nsSSState} = 'SS_FEW';
#       return();
#     }
#     if ($is_lunation_date == 0) {
#       print STDERR "execute_sweetspot: Invalid lunation date. ($date $is_lunation_date)\n";
#       $metadata_out{nsSSState} = 'SS_ERROR';
#       return();
#     }
#     $metadata_out{nsSSState} = 'SS_QUEUE';
#     $metadata_out{nsSweetSpot} = $Nstacks;
#     unless(defined($pretend)) {
#       $metadata_out{nsSSState} = 'SS_DONE';
#       sweetspot_queue($date);
#     }
#     if (defined($pretend)) {
#       add_to_macro_list('check_sweetspot',1,$date);
#     }
#     else {
#       add_to_macro_list('queue_sweetspot',1,$date);
#     }
# }

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
    if (defined($reduction)) {
        $cmd .= " -set_reduction $reduction ";
    }
    else {
        $cmd .= " -set_reduction NIGHTLY_STACK ";
    }
    if (defined($science_config{$target}{ADDITIONAL_STACK_LABEL})) {
        # Grab list of skycells
        my $skycell_select = '';
        {
            my %skycells = ();
            my $warpcmd = "$warptool -warped -label $label -data_group $data_group  -dbname $dbname";
            my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) = 
                run(command => $warpcmd, verbose => $verbose);
            unless($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unabel to perform warptool -warped to determine skycell_ids: $error_code",$target);
            }
            my $runs = $mdcParser->parse_list(join "", @$stdout_buf) or
                &my_die("Unabel to parse warptool -warped to determine skycell_ids: $error_code",$target);
            if ($#{ $runs } != -1) {
                for my $warp (@$runs) {
                    $skycells{$warp->{skycell_id}} = 1;
                }
                foreach my $skycell (sort (keys (%skycells))) {
                    $skycell_select .= " -select_skycell_id $skycell ";
                }
                $cmd .= " -select_label $science_config{$target}{ADDITIONAL_STACK_LABEL} $skycell_select ";
            }
        }
    }
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
    my $observing_state = shift;
    my $target = shift;
    my $filter = shift;

    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

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
   
    my $minimum_required_warp;
    if ($observing_state eq 'OBSERVING') {
        $minimum_required_warp = $science_config{$target}{MIN_STACK};
    }
    unless (defined($minimum_required_warp)) {
        $minimum_required_warp = 0;
    }

    return($#input_exposures  + 1, $#{ $chip_ref } + 1, $#{ $cam_ref } + $#{ $warp_ref } + 2, $minimum_required_warp, $Nalready);
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
    my $observing_state = shift;
    my $pretend = shift;

    my $Npotential = 0;
    my $Nqueued = 0;
    my $is_processing = 0;
    my $is_registering;
    if ($metadata_out{nsRegistrationState} eq 'REGISTERED') {
        $is_registering = 0;
    }
    else {
        $is_registering = 1;
    }

    foreach my $target (sort (keys %science_config)) {
        if ($science_config{$target}{STACKABLE} == 1) {
            foreach my $filter (@filter_list) {
                my ($Nexposures,$NprocChips,$NprocWarps,$NrequiredWarps,$Nalready) = pre_stack_queue($date,$observing_state,$target,$filter);
                if ((!defined($force_stack_count))&&($NprocChips != $NprocWarps)) { # This makes me sad. :(
                    if ($debug == 1) {
                        print STDERR "execute_stacks: Target $target on $date is not fully processed. ($NprocChips $NprocWarps)\n";
                    }
                    $is_processing = 1;
                    $metadata_out{nsStackState} = 'FORCETOWARP';
                    next;
                }
                if ($Nexposures == 0) {
                    if ($debug == 1) {
                        print STDERR "execute_stacks: Target $target in filter $filter on $date has no exposures.\n";
                    }
                    next;
                }
                if ($NprocWarps < $NrequiredWarps) {
                    if ($debug == 1) {
                        print STDERR "execute_stacks: Target $target in filter $filter on $date has too few warps to begin stacking. ($NprocWarps $NrequiredWarps)\n";
                    }
                    next;
                }
                $Npotential++;
                if ($Nalready != 0) {
                    $Nqueued++;
                    if ($debug == 1) {
                        print STDERR "execute_stacks: Not queueing $target in filter $filter on $date due to already existing stacks.\n";
                    }
                    unless ($metadata_out{nsStackState} eq 'FORCETOWARP') {
                        $metadata_out{nsStackState} = 'STACKING';
                    }
                    next;
                }
                unless (($metadata_out{nsStackState} eq 'FORCETOWARP')||($metadata_out{nsStackState} eq 'STACKING')) {
                    $metadata_out{nsStackState} = 'QUEUESTACKS';
                }
                unless(defined($pretend)) {
                    if ($debug == 1) {
                        print STDERR "execute_stacks: Target $target in filter $filter on $date has exposures and will be queued.\n";
                    }
                    stack_queue($date,$target,$filter);
                    $Nqueued ++;
                }
                if (defined($pretend)) {
                    add_to_macro_list('check_stacks',$science_config{$target}{STACKABLE},$date,$target,$filter);
                }
                else {
                    add_to_macro_list('queue_stacks',$science_config{$target}{STACKABLE},$date,$target,$filter);
                }
            }
        }
        else {
            if ($debug == 1) {
                print STDERR "execute_stacks: Target $target is not auto-stackable.\n";
            }
        }
    }
    $metadata_out{nsStackPotential} = $Npotential;
    $metadata_out{nsStackQueued}    = $Nqueued;
    if (($Npotential == $Nqueued)&&($metadata_out{nsObservingState} eq 'END_OF_NIGHT')&&($is_processing == 0)&&($is_registering == 0)) {
        $metadata_out{nsStackState} = 'FINISHED_STACKS';
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
   
    foreach my $target (sort (keys %science_config)) {
        if ($science_config{$target}{STACKABLE} == 1) {
            my ($Nstacks, $Nattempted) = post_stack_queue($date,$target);
            if ($debug == 1) {
                print STDERR "confirm_stacks: Target $target on $date has $Nattempted attempts on $Nstacks.\n";
            }
            if ($Nstacks != $Nattempted) {
                if ($debug == 1) {
                    print STDERR "confirm_stacks: Target $target on $date is not done stacking. $Nstacks $Nattempted\n";
                }
                if ($metadata_out{nsStackState} eq 'CONFIRM_STACKING') {
                    $metadata_out{nsStackState} = 'STACKING';
                }
                next;
            }
            if ($Nstacks == 0) {
                if ($debug == 1) {
                    print STDERR "confirm_stacks: Target $target on $date has no stacks. Skipping.\n";
                }
                next;
            }
            if ($metadata_out{nsStackState} eq 'CONFIRM_STACKING') {
                if (defined($pretend)) {
                    add_to_macro_list('check_confirm_stacks',$science_config{$target}{STACKABLE},$date,$target);
                }
                else {
                    add_to_macro_list('confirm_stacks',$science_config{$target}{STACKABLE},$date,$target);
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

    my $count = 0;
   
    my $sth = "SELECT DISTINCT diff_id from diffRun JOIN diffInputSkyfile USING(diff_id) where data_group = '$data_group' AND warp1 = $warp_id_1 AND warp2 = $warp_id_2";
    my $data_ref = $db->selectall_arrayref( $sth );
    $count += $#{ $data_ref } + 1;

    $sth = "SELECT DISTINCT diff_id from diffRun JOIN diffInputSkyfile USING(diff_id) where data_group = '$data_group' AND warp1 = $warp_id_2 AND warp2 = $warp_id_1";
    $data_ref = $db->selectall_arrayref( $sth );
    $count += $#{ $data_ref } + 1;

    return($count);
}

sub multi_date_verify_uniqueness_diff {
    my $warp_id_1 = shift;
    my $warp_id_2 = shift;
    my $date = shift;
    my $target = shift;

    my $db = init_gpc_db();
    $date =~ s/-//g;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $count = 0;
   
    my $sth = "SELECT DISTINCT diff_id from diffRun JOIN diffInputSkyfile USING(diff_id) where (label = '$label' OR label = 'goto_cleaned') AND warp1 = $warp_id_1 AND warp2 = $warp_id_2";
    my $data_ref = $db->selectall_arrayref( $sth );
    $count += $#{ $data_ref } + 1;

    $sth = "SELECT DISTINCT diff_id from diffRun JOIN diffInputSkyfile USING(diff_id) where (label = '$label' OR label = 'goto_cleaned') AND warp1 = $warp_id_2 AND warp2 = $warp_id_1";
    $data_ref = $db->selectall_arrayref( $sth );
    $count += $#{ $data_ref } + 1;

    return($count);
}

sub pre_diff_queue {
    my $date = shift;
    my $observing_state = shift;
    my $target = shift;
    my $filter = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);
   
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

    return($#{ $chip_ref } + 1, $#{ $cam_ref } + $#{ $warp_ref } + 2);
}

sub execute_diffs {
    my $date = shift;
    my $observing_state = shift;
    my $pretend = shift;

    my $Npotential = 0;
    my $Nnoexp     = 0;
    my $is_processing = 0;
    my $is_registering;
    if ($metadata_out{nsRegistrationState} eq 'REGISTERED') {
        $is_registering = 0;
    }
    else {
        $is_registering = 1;
    }

    foreach my $target (sort (keys %science_config)) {
        if ($science_config{$target}{DIFFABLE} == 1) {

            my $config_reduction = "DEFAULT";
            if($science_config{$target}{REDUCTION}) {$config_reduction=$science_config{$target}{REDUCTION};} 
            print STDERR "execute_diffs: Trying target $target on $date $camera $science_config{$target}{REDUCTION} $config_reduction\n";

            my $recipe_psastro = $ipprcam->reduction($config_reduction, 'PSASTRO'); # Recipe to use
            &my_die("Unrecognised PSASTRO recipe", $PS_EXIT_CONFIG_ERROR) unless defined $recipe_psastro;

            my $recipeData;
            {
                # Get the PSASTRO recipe
                my $command = "$ppConfigDump -camera $camera -recipe PSASTRO $recipe_psastro -dump-recipe PSASTRO -";
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform ppConfigDump: $error_code", $PS_EXIT_CONFIG_ERROR);
                }
                $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
                    &my_die("Unable to parse metadata config doc", $PS_EXIT_CONFIG_ERROR);
            }

            my $maxFWHM = metadataLookupF32($recipeData, 'PSASTRO.MAX.ALLOWED.FWHM');
            print STDERR "execute_diffs: Max FWHM= $maxFWHM for $target\n";

            foreach my $filter (@filter_list) {
                $Npotential ++;
                my ($NprocChips,$NprocWarps) = pre_diff_queue($date,$observing_state,$target,$filter);
                if ((!defined($force_diff_count))&&($NprocChips != $NprocWarps)) {
                    if ($debug == 1) {
                        print STDERR "execute_diffs: Target $target in filter $filter on $date is not fully processed. ($NprocChips $NprocWarps)\n";
                    }
                    $is_processing = 1;
                }
                if ($NprocChips == 0) {
                    $Nnoexp ++;
                    if ($debug == 1) {
                        print STDERR "execute_diffs: Target $target in filter $filter on $date has no exposures.\n";
                    }
                    next;
                }
                unless (($metadata_out{nsDiffState} eq 'FORCETOWARP')||($metadata_out{nsDiffState} eq 'DIFFING')) {
                    $metadata_out{nsDiffState} = 'QUEUEDIFFS';
                }
                if ($debug == 1) {
                    print STDERR "execute_diffs: Target $target in filter $filter on $date has exposures and will be queued.\n";
                }
                diff_queue($date,$target,$filter,$maxFWHM,$pretend);

                #Queue up desperate diffs if the conditions are right for them 
                if ((defined($science_config{$target}{DESPERATE_DIFFS})) && ($science_config{$target}{DESPERATE_DIFFS} == 1)) {
                    desperate_diff_singles($date,$target,$filter,$maxFWHM,$pretend);
                }

                if (defined($pretend)) {
                    add_to_macro_list('check_diffs',$science_config{$target}{DIFFABLE},$date,$target,$filter);
                }
                else {
                    add_to_macro_list('queue_diffs',$science_config{$target}{DIFFABLE},$date,$target,$filter);
                }
            }
        }
        else {
            if ($debug == 1) {
                print STDERR "execute_diffs: Target $target is not auto-diffable.\n";
            }
        }
    }
    if ($debug == 1) {
        print "$metadata_out{nsObservingState} $metadata_out{nsDiffPotential} $metadata_out{nsDiffQueued} $is_processing $is_registering\n";
    }

    if ($metadata_out{nsObservingState} eq 'END_OF_NIGHT') {
        if ($is_processing == 1) {
            $metadata_out{nsDiffState} = 'DIFFING';
        }
        elsif ($is_registering == 0) {
            if ($Npotential == $Nnoexp) {
                $metadata_out{nsDiffState} = 'FINISHED_DIFFS';
            }
            elsif ($metadata_out{nsDiffPotential} == $metadata_out{nsDiffQueued}) {
                $metadata_out{nsDiffState} = 'FINISHED_DIFFS';
            }
        }
        else {
            $metadata_out{nsDiffState} = 'DIFFING';
        }
    }
    else {
        $metadata_out{nsDiffState} = 'DIFFING';
    }
    if ($metadata_out{nsDiffState} eq 'FINISHED_DIFFS') {
        foreach my $target (sort (keys %science_config)) {

            my $config_reduction = "DEFAULT";
            if($science_config{$target}{REDUCTION}) {$config_reduction=$science_config{$target}{REDUCTION};} 
            print STDERR "execute_diffs: Trying target $target on $date $camera $science_config{$target}{REDUCTION} $config_reduction\n";

            my $recipe_psastro = $ipprcam->reduction($config_reduction, 'PSASTRO'); # Recipe to use
            &my_die("Unrecognised PSASTRO recipe", $PS_EXIT_CONFIG_ERROR) unless defined $recipe_psastro;

            my $recipeData;
            {
                # Get the PSASTRO recipe
                my $command = "$ppConfigDump -camera $camera -recipe PSASTRO $recipe_psastro -dump-recipe PSASTRO -";
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform ppConfigDump: $error_code", $PS_EXIT_CONFIG_ERROR);
                }
                $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
                    &my_die("Unable to parse metadata config doc", $PS_EXIT_CONFIG_ERROR);
            }

            my $maxFWHM = metadataLookupF32($recipeData, 'PSASTRO.MAX.ALLOWED.FWHM');

            # CZW: We should only arrive at this point once, so we can construct a diff report and mail it now.
            if ((defined($science_config{$target}{DESPERATE_DIFFS})) && ($science_config{$target}{DESPERATE_DIFFS} == 1)) {
                foreach my $filter (@filter_list) {
                    desperate_diff_queue($date,$target,$filter,$maxFWHM,$pretend);
                }
            }

            if ((defined($science_config{$target}{SELF_WSDIFFS})) && ($science_config{$target}{SELF_WSDIFFS} == 1)) {
                foreach my $filter (@filter_list) {
                    # This one needs to return a state to see if we need to wait on stacking before checking again.
                    $metadata_out{nsDiffState} = self_WS_diff_queue($date,$target,$filter,$pretend);
                }
            }
        }
    }

#     if (($Npotential == $Nnoexp)&&($metadata_out{nsObservingState} eq 'END_OF_NIGHT')&&($is_processing == 0)) {
#       $metadata_out{nsDiffState} = 'FINISHED_DIFFS';
#     }

#     if ($is_processing == 1) {
#       $metadata_out{nsDiffState} = 'DIFFING';
#     }
}

sub diff_queue {
    my $date = shift;
    my $target = shift;
    my $filter = shift;
    my $maxFWHM = shift;
    my $pretend = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $db = init_gpc_db();

    my $obj_sth = "select rawExp.object,substr(rawExp.comment, 1, position(' ' in rawExp.comment)) from warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) ";
    $obj_sth .= " WHERE warpRun.state = 'full' AND warpRun.label = '$label' AND warpRun.data_group = '$data_group' AND rawExp.filter = '$filter' GROUP BY rawExp.object,substr(rawExp.comment, 1, position(' ' in rawExp.comment))";

    my $object_ref = $db->selectall_arrayref( $obj_sth );

    my $Npotential = 0;
    my $Nqueued = 0;
   
    foreach my $object_row (@{ $object_ref }) {
        my $this_object = shift @{ $object_row };       
        my $this_chunk = shift @{ $object_row };
        my $input_sth = "select exp_id,warp_id,dateobs,rawExp.comment,warpRun.state AS warp_state,camProcessedExp.quality,camProcessedExp.fwhm_major,chipRun.state,camRun.state FROM ";
        $input_sth .=   " rawExp LEFT JOIN chipRun USING (exp_id) LEFT JOIN camRun USING (chip_id) LEFT JOIN camProcessedExp USING(cam_id) LEFT JOIN fakeRun USING (cam_id) LEFT JOIN warpRun USING (fake_id) ";
        $input_sth .=   " WHERE chipRun.label = '$label' AND chipRun.data_group = '$data_group' AND rawExp.filter = '$filter' AND rawExp.object = '$this_object' ";
        $input_sth .=   " AND substr(rawExp.comment, 1, position(' ' in rawExp.comment)) = '$this_chunk' ORDER BY rawExp.comment,dateobs ";

        my $warps = $db->selectall_arrayref( $input_sth );

        #consider which exposures should be used to be used to make warps (bad states and too high FWHM are not be allowed)
        # Each comment should only appear once. Therefore, if we see it more than once, we assume the first is extra.
        my %comment_hash = ();
        foreach my $this_warp (@{ $warps }) {
            my $this_comment = ${ $this_warp }[3];
            my $this_exp_id  = ${ $this_warp }[0];
            my $this_quality = ${ $this_warp }[5];
            my $this_state   = ${ $this_warp }[4];
            my $this_fwhm   = ${ $this_warp }[6];
	    
            if ( ($this_quality != 0) || ($this_state eq 'drop') || ($this_fwhm > $maxFWHM)) {
                print STDERR "diff_queue: excluding $this_exp_id for $this_object due to non-zero cam.quality $this_quality or state $this_state or FWHM $this_fwhm\n";
            }
            else {
                $comment_hash{$this_comment} = $this_exp_id;
            }
        }

        # Exclude any warps that are not stored in the comment_hash (overrides) and check their status.
        my @keep_warps = ();
        my $Nbad = 0;
        foreach my $this_warp (@{ $warps }) {
            my $this_comment = ${ $this_warp }[3];
            my $this_exp_id  = ${ $this_warp }[0];
            my $this_quality = ${ $this_warp }[5];
            my $this_warp_id  = ${ $this_warp }[1];
            my $chip_state   = ${ $this_warp }[7];
            my $cam_state   = ${ $this_warp }[8];
            if ((exists($comment_hash{$this_comment}))&&
                ($comment_hash{$this_comment} == $this_exp_id)) {
                push @keep_warps, $this_warp;

	        #do not continue if you encounter exposures that finished cam stage but have not yet continued to warp (i.e. stuck in between stages)
                if (($this_warp_id eq 'NULL') && (($this_quality == 0) || ($this_quality eq 'NULL')) ) {
                    print STDERR "diff_queue: exposure with exp_id $this_exp_id for $this_object has warp_id $this_warp_id and quality $this_quality and is therefore not fully processed\n";
            	    $Nbad += 1;
                }
	        #do not continue if you encounter exposures that have not been fully processed or have bad quality
                if (($chip_state ne 'full') || ($cam_state ne 'full') || ($this_quality != 0) ) {
                    print STDERR "diff_queue: exposure with exp_id $this_exp_id for $this_object has chip state $chip_state and cam state $cam_state and quality $this_quality and is therefore not fully processed\n";
            	    $Nbad += 1;
                }
            }
            else {
                print STDERR "diff_queue: excluding $this_exp_id for $this_object due to not being an accepted comment string $this_comment\n";
            }
        }
        @{ $warps } = @keep_warps;

        #kick object out of diff consideration if it has exposures not fully processed
        if ($Nbad > 0) {
            print STDERR "diff_queue: excluding $this_object from making diffs due to not being fully processed to warp stage\n";
            next;
        }

        # Exclude the last entry if we do not have an even number of warps.
        if (($#{ $warps } + 1) % 2 != 0) {
            print STDERR "diff_queue: Number of input warps to make diffs is not even for target $target and object $this_object! $#{ $warps } ";
            if ($#{ $warps} + 1 == 1) {
                print STDERR "diff_queue: I can do no diffs with only one exposure.\n";
                next;
            }
            else {
                my $rejected_warp = pop @{ $warps };
                my $rejected_exp_id = ${ $rejected_warp }[0];
                print STDERR "diff_queue: Rejecting ${rejected_exp_id} to force visit count.\n";
            }
        }
       
        while ($#{ $warps } > -1) {
            # The array is sorted in pairs of input/template.
            my $input_warp = shift @{ $warps };
            my $template_warp = shift @{ $warps };

            my $input_exp_id = ${ $input_warp }[0];
            my $input_comment = ${ $input_warp }[3];

            my $template_exp_id = ${ $template_warp }[0];
            my $template_comment = ${ $template_warp }[3];

            my $input_warp_id = ${ $input_warp }[1];
            my $template_warp_id = ${ $template_warp }[1];

            my $input_warp_state = ${ $input_warp }[4];
            my $template_warp_state = ${ $template_warp }[4];

            my $input_warp_camQuality = ${ $input_warp }[5];
            my $template_warp_camQuality = ${ $template_warp }[5];

            $Npotential++;

            unless (defined($input_warp_id) && defined($template_warp_id) &&
                    ($input_warp_state eq 'full')&&($template_warp_state eq 'full')) {
                print STDERR "diff_queue:Diff for this $date $target $input_exp_id $template_exp_id not fully processed ($input_warp_state $template_warp_state) ($input_warp_camQuality $template_warp_camQuality)\n";
                if (($input_warp_camQuality == 4007)||($template_warp_camQuality == 4007)) {
                    # This should now never be reached.
                    # CZW: Trigger backup plan here?  Or simply set up framework?
                    print STDERR "diff_queue: ...but this is due to a camera stage astrometry quality\n";
                    $Npotential--;
                }
                next;
            }

            if (verify_uniqueness_diff($input_warp_id,$template_warp_id,$date,$target) != 0) {
                $Nqueued++;
                print STDERR "diff_queue:Diffs already queued for this $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
                next;
            }
            else {
                print STDERR "diff_queue:Preparing to diff $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
            }
           
            my $cmd = "$difftool -dbname $dbname  -definewarpwarp  ";
            $cmd .= "-input_label $label  -template_label $label -good_frac 0.1 ";
            $cmd .= "-backwards "; # Needed because difftool assumes a different date sorting.
            $cmd .= "-set_workdir $workdir  -set_dist_group $dist_group  -set_data_group $data_group ";
            $cmd .= " -simple  -set_label $label -exp_id $input_exp_id -template_exp_id $template_exp_id ";
#               $cmd .= " -pretend ";
            if (defined($reduction)) {
                $cmd .= " -set_reduction $reduction ";
            }

            if (defined($pretend)) {
                $cmd .= ' -pretend ';
            }
            if ($debug == 1) {
                $cmd .= ' -pretend ';
                print STDERR "diff_queue: $cmd\n";
                print STDERR " $input_warp_id $template_warp_id\n";
            }
           
            if (($debug == 0)&&(!defined($pretend))) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run ( command => $cmd, verbose => $verbose );
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
                }
                $Nqueued++;
            }
        }
    }
    $metadata_out{nsDiffPotential} += $Npotential;
    $metadata_out{nsDiffQueued}    += $Nqueued;
#      if (($metadata_out{nsDiffPotential} == $metadata_out{nsDiffQueued})&&($metadata_out{nsObservingState} eq 'END_OF_NIGHT')) {
#       $metadata_out{nsDiffState} = 'FINISHED_DIFFS';
#      }       

}

sub desperate_diff_singles {
    my $date = shift;
    my $target = shift;
    my $filter = shift;
    my $maxFWHM = shift;
    my $pretend = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);
    my $this_date = undef;
    my $chunk_name = undef;
    my ($year,$month,$day,$hour,$min,$sec);

    my $db = init_gpc_db();

    my $obj_sth = "select rawExp.object,substr(rawExp.comment, 1, position(' ' in rawExp.comment)) from warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) ";
    $obj_sth .= " WHERE warpRun.state = 'full' AND warpRun.label = '$label' AND warpRun.data_group = '$data_group' AND rawExp.filter = '$filter' GROUP BY rawExp.object,substr(rawExp.comment, 1, position(' ' in rawExp.comment))";

    my $object_ref = $db->selectall_arrayref( $obj_sth );

    my $Npotential = 0;
    my $Nqueued = 0;

    #Before considering making diffs, we should check if there are no issues with summit download and/or registration.
    #summitExp has limited information, so see if the number of exps in rawExp and summitExp match up before most recent dateobs of chunk
    #be careful here, since dateobs is not quite completely the same in both tables. Therefore, allow numbers to be off by 1
    my $timenow=DateTime->now;
    $timenow->subtract(minutes => 15);
    $timenow->set_time_zone("UTC");

    my $dateinit = $timenow->ymd;
    my $raw_sth = "select count(dateobs),MAX(dateobs) FROM rawExp WHERE exp_name LIKE 'o%' AND dateobs > '$dateinit' AND dateobs <= '$timenow' ";
    my ($nrawexps,$rawdate) = $db->selectrow_array( $raw_sth );

    my $summit_sth = "select count(dateobs),MAX(dateobs) FROM summitExp WHERE exp_name LIKE 'o%' AND dateobs > '$dateinit' AND dateobs <= '$timenow' ";
    my ($nsummitexps,$summitdate) = $db->selectrow_array( $summit_sth );
 
    my $summittimediff = 9999;
    if(($rawdate)&&($summitdate)) {
        $rawdate=~/^(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2})$/;
        my $date1=DateTime->new(year=>$1,month=>$2,day=>$3,hour=>$4,minute=>$5,second=>$6,time_zone=>"UTC");
 
        $summitdate=~/^(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2})$/;
        my $date2=DateTime->new(year=>$1,month=>$2,day=>$3,hour=>$4,minute=>$5,second=>$6,time_zone=>"UTC");
       
        my $difference=$date2->delta_ms($date1);

        $summittimediff=(($difference->hours)*60.) + ($difference->minutes) + (($difference->seconds)/60.);
    }

    #Run some basic checks
    my $noff = abs($nsummitexps-$nrawexps);
    if (($summittimediff > 10.)||($noff >10)) {
        if ($debug == 1) {
        print STDERR "desp_diff_singles: No desperate diffs will be attempted, since the number of exps at the summit and in rawExp do not match ($nsummitexps $nrawexps), or the timestamps are off ($summittimediff)\n";
        }
    }
    else {   
        foreach my $object_row (@{ $object_ref }) {
            my $this_object = shift @{ $object_row };
            my $this_chunk = shift @{ $object_row };
       
            my $input_sth = "select exp_id,warp_id,dateobs,rawExp.comment,warpRun.state AS warp_state,camProcessedExp.quality,camProcessedExp.fwhm_major,chipRun.state,camRun.state FROM ";
            $input_sth .=   " rawExp LEFT JOIN chipRun USING (exp_id) LEFT JOIN camRun USING (chip_id) LEFT JOIN camProcessedExp USING(cam_id) LEFT JOIN fakeRun USING (cam_id) LEFT JOIN warpRun USING (fake_id) ";
            $input_sth .=   " WHERE chipRun.label = '$label' AND chipRun.data_group = '$data_group' AND rawExp.filter = '$filter' AND rawExp.object = '$this_object' ";
            $input_sth .=   " AND substr(rawExp.comment, 1, position(' ' in rawExp.comment)) = '$this_chunk' ORDER BY rawExp.comment,dateobs ";

            my $warps = $db->selectall_arrayref( $input_sth );
            my $warpsBQ = $db->selectall_arrayref( $input_sth );;

            #consider which exposures should be used to be used to make warps (bad states and too high FWHM are not be allowed)
            # Each comment should only appear once. Therefore, if we see it more than once, we assume the first is extra.
            my %comment_hash = ();
            my %comment_hash_good = ();
            foreach my $this_warp (@{ $warps }) {
                my $this_comment = ${ $this_warp }[3];
                my $this_exp_id  = ${ $this_warp }[0];
                my $this_quality = ${ $this_warp }[5];
                my $this_state   = ${ $this_warp }[4];
                my $this_fwhm   = ${ $this_warp }[6];
	    
                if (($this_quality != 0) || ($this_state eq 'drop') || ($this_fwhm > $maxFWHM)) {
                    print STDERR "desp_diff_singles: excluding $this_exp_id for $this_object due to non-zero cam.quality $this_quality or state $this_state or FWHM $this_fwhm\n";
                }
                else {
                    $comment_hash_good{$this_comment} = $this_exp_id;
                }
		
                #also save the entries which are ok, but not best quality
                if (($this_quality == 0) && ($this_state ne 'drop') ) {
                    $comment_hash{$this_comment} = $this_exp_id;
                }
		
            }

            # Each comment should only appear once. Therefore, if we see it more than once, we assume the first is extra.
            my %comment_hash = ();
            my %comment_hash_good = ();
            my $Nbad = 0;
            foreach my $this_warp (@{ $warps }) {
                my $this_comment = ${ $this_warp }[3];
                my $this_exp_id  = ${ $this_warp }[0];
                my $this_quality = ${ $this_warp }[5];
                my $this_state   = ${ $this_warp }[4];
                my $this_fwhm   = ${ $this_warp }[6];

                if (($this_quality != 0) || ($this_state eq 'drop') || ($this_fwhm > $maxFWHM)) {
                    print STDERR "desp_diff_singles: excluding $this_exp_id for $this_object due to non-zero cam.quality $this_quality or state $this_state or FWHM $this_fwhm\n";
                }
                else {
                    $comment_hash_good{$this_comment} = $this_exp_id;
                }
               
                #also save the entries which are ok, but not best quality
                if (($this_quality == 0) && ($this_state ne 'drop') ) {
                    $comment_hash{$this_comment} = $this_exp_id;
                }
            }

            # Exclude any warps that are not stored in the comment_hash (overrides) and check their status.
            my @keep_warps = ();
            my $Nbad = 0;
            foreach my $this_warp (@{ $warps }) {
                my $this_comment = ${ $this_warp }[3];
                my $this_exp_id  = ${ $this_warp }[0];
                my $this_quality = ${ $this_warp }[5];
                my $this_warp_id  = ${ $this_warp }[1];
                my $chip_state   = ${ $this_warp }[7];
                my $cam_state   = ${ $this_warp }[8];
                if ((exists($comment_hash_good{$this_comment}))&&
                    ($comment_hash_good{$this_comment} == $this_exp_id)) {
                    push @keep_warps, $this_warp;
                    $this_date  = ${ $this_warp }[2];
                    $chunk_name  = ${ $this_warp }[6];

	            #do not continue if you encounter exposures that finished cam stage but have not yet continued to warp (i.e. stuck in between stages)
                    if (($this_warp_id eq 'NULL') && (($this_quality == 0) || ($this_quality eq 'NULL')) ) {
                        print STDERR "desp_diff_singles: exposure with exp_id $this_exp_id for $this_object has warp_id $this_warp_id and quality $this_quality and is therefore not fully processed\n";
            	        $Nbad += 1;
                    }
	            #do not continue if you encounter exposures that have not been fully processed or have bad quality
                    if (($chip_state ne 'full') || ($cam_state ne 'full') || ($this_quality != 0) ) {
                        print STDERR "desp_diff_singles: exposure with exp_id $this_exp_id for $this_object has chip state $chip_state and cam state $cam_state and quality $this_quality and is therefore not fully processed\n";
            	        $Nbad += 1;
                    }
                }
                else {
                    print STDERR "desp_diff_singles: excluding $this_exp_id for $this_object due to not being an accepted comment string $this_comment\n";
                }
            }
            @{ $warps } = @keep_warps;
            my $nwarps = ($#{ $warps } + 1);

            #kick object out of diff consideration if it has exposures not fully processed
            if ($Nbad > 0) {
                print STDERR "desp_diff_singles: excluding $this_object from making diffs due to not being fully processed to warp stage\n";
                next;
            }
           
            #find the time of the most recent exposure in that chunk
            my $input_chunk = "select max(dateobs) FROM rawExp WHERE substr(comment, 1, position(' ' in comment)) = '$chunk_name' AND filter = '$filter' ";
            my $chunk = $db->selectall_arrayref( $input_chunk );
            foreach my $this_chunk (@{ $chunk }) {
                my $this_date = ${ $this_chunk }[0];
            } 
 
            #compute and store some stats for potential on-the-fly desperate diffs
            my $timediff = 0;
            my $now=DateTime->now;
            $now->set_time_zone("UTC");

            if($this_date) {
                if($this_date=~/^(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2})$/) {
                    my $dt=DateTime->new(year=>$1,month=>$2,day=>$3,hour=>$4,minute=>$5,second=>$6,time_zone=>"UTC");
                    my $difference=$now->delta_ms($dt);
                    $timediff=($difference->hours) + (($difference->minutes) + ($difference->seconds)/60.)/60.;
                }
            }
           
            ################################
            #We reach a junction here, where having more than 3 (but uneven) warps will result in desperate diffs
            #Conversely, less than 3 good quality warps might result in dirty diffs
            ################################
            if($nwarps >= 3) {     
                #Consider the special case of on-the-fly desperate diffs
                #The conditions are: uneven nr of warps greater than 3, and the most recent observations is more than $desdiffdt hours old
                if (($nwarps % 2 != 0) && ($nwarps >= 3) && ($timediff < $desdiffdt)) {
                    print STDERR "desp_diff_singles: There are potential desperate diffs to be done (nwarps = $nwarps) for $this_object, but the time criterium is not met ($timediff < $desdiffdt).\n";
                    next;
                }
                if (($nwarps % 2 == 0) || ($nwarps == 1) || ($timediff < $desdiffdt)) {
                    print STDERR "desp_diff_singles: No desperate diffs will be attempted for $this_object, since the number of warps is even, less than two ($nwarps) and/or the time criterium is not met ($timediff < $desdiffdt)\n";
                    next;
                }

                # We are attempting to do the missing diffs, so reverse the list of retained warps.
                # Good objects with all visits will be skipped due to the duplicate check.
                # Bad objects will have the earliest visit rejected, and the visits repaired in a way that will produce all desired pairs.
                @{ $warps } = reverse @keep_warps;

                # Exclude the last entry if we do not have an even number of warps.
                if (($#{ $warps } + 1) % 2 != 0) {
                    print STDERR "desp_diff_singles: Number of input warps to make diffs is not even for target $target and object $this_object!";
                    my $rejected_warp = pop @{ $warps };
                    my $rejected_exp_id = ${ $rejected_warp }[0];
                    print STDERR "desp_diff_singles: Rejecting ${rejected_exp_id} to force visit count.\n";
                }
       
                while ($#{ $warps } > -1) {
                    # The array is sorted in pairs of input/template.
                    my $template_warp = shift @{ $warps };
                    my $input_warp = shift @{ $warps };
   
                    my $input_exp_id = ${ $input_warp }[0];
                    my $input_comment = ${ $input_warp }[3];

                    my $template_exp_id = ${ $template_warp }[0];
                    my $template_comment = ${ $template_warp }[3];

                    my $input_warp_id = ${ $input_warp }[1];
                    my $template_warp_id = ${ $template_warp }[1];

                    my $input_warp_state = ${ $input_warp }[4];
                    my $template_warp_state = ${ $template_warp }[4];

                    my $input_warp_camQuality = ${ $input_warp }[5];
                    my $template_warp_camQuality = ${ $template_warp }[5];

                    $Npotential++;

                    unless (defined($input_warp_id) && defined($template_warp_id) &&
                        ($input_warp_state eq 'full')&&($template_warp_state eq 'full')) {
                        print STDERR "desp_diff_singles: Desp diff for this $date $target $input_exp_id $template_exp_id not fully processed ($input_warp_state $template_warp_state) ($input_warp_camQuality $template_warp_camQuality)\n";
                        if (($input_warp_camQuality == 4007)||($template_warp_camQuality == 4007)) {
                            # This should now never be reached.
                            # CZW: Trigger backup plan here?  Or simply set up framework?
                            print STDERR "desp_diff_singles: ...but this is due to a camera stage astrometry quality\n";
                            $Npotential--;
                        }
                        next;
                    }

                    if (verify_uniqueness_diff($input_warp_id,$template_warp_id,$date,$target) != 0) {
                        $Nqueued++;
                        print STDERR "desp_diff_singles: Desp diffs already queued for this $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
                        next;
                    }
                    else {
                        print STDERR "desp_diff_singles: Preparing to single desperate diff $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
                    }
           
                    my $cmd = "$difftool -dbname $dbname  -definewarpwarp  ";
                    $cmd .= "-input_label $label  -template_label $label -good_frac 0.1 ";
                    $cmd .= "-backwards "; # Needed because difftool assumes a different date sorting.
                    $cmd .= "-rerun "; # Needed because we may have some diffs that already use some of the exposures
                    $cmd .= "-set_workdir $workdir  -set_dist_group $dist_group  -set_data_group $data_group ";
                    $cmd .= " -simple  -set_label $label -exp_id $input_exp_id -template_exp_id $template_exp_id ";
                    if (defined($reduction)) {
                        $cmd .= " -set_reduction $reduction ";
                    }
       
                    if (defined($pretend)) {
                        $cmd .= ' -pretend ';
                    }
                    if ($debug == 1) {
                        $cmd .= ' -pretend ';
                        print STDERR "desp_diff_singles: $cmd\n";
                        print STDERR " $input_warp_id $template_warp_id\n";
                    }
           
                    if (($debug == 0)&&(!defined($pretend))) {
                        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                            run ( command => $cmd, verbose => $verbose );
                        unless ($success) {
                            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                            &my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
                        }
                        $Nqueued++;
                    }
                }           
           
            } else {       
                print STDERR "desp_diff_singles: There is a possibility for dirty diffs to be made for $this_object, but this is not implemeted at the moment\n";	    
                #Consider the case of on-the-fly dirty desperate diffs
                #The conditions are: uneven nr of bad quality warps greater than 3, and the most recent observations is more than $desdiffdt hours old

#		 # Exclude any warps that are not stored in the more general comment_hash.
#		 my @keep_warps = ();
#		 foreach my $this_warp (@{ $warpsBQ }) {
#		     my $this_comment = ${ $this_warp }[3];
#		     my $this_exp_id  = ${ $this_warp }[0];
#		     if ((exists($comment_hash{$this_comment}))&&
#			 ($comment_hash{$this_comment} == $this_exp_id)) {
#			 push @keep_warps, $this_warp;
#			 $this_date  = ${ $this_warp }[2];
#			 $chunk_name  = ${ $this_warp }[6];
#		     }
#		     else {
#			 print STDERR "desp_diff_singles: excluding $this_exp_id for $this_object due to being rejected $this_comment\n";
#		     }
#		 }
#		 @{ $warpsBQ } = @keep_warps;
#
#		 if ($nwarps >= 2) {
#		     #one regular diff has already been made. So, need only one more. Strip a good entry from the array 	    
#		     my $nrem = 0;
#		     my @keep_warps = ();
#		     foreach my $this_warp (@{ $warpsBQ }) {
#			 my $this_comment = ${ $this_warp }[3];
#			 my $this_exp_id  = ${ $this_warp }[0];
#			 my $this_fwhm   = ${ $this_warp }[7];
#			 if (($nrem <1) && ($this_fwhm <= $maxFWHM)) {
#			     $nrem++;
#			     print STDERR "desp_diff_singles: excluding $this_exp_id for $this_object from dirty desp diffs $this_comment\n";
#			 } else {
#			     push @keep_warps, $this_warp;			 
#			 }
#		     }
#		     @{ $warpsBQ } = @keep_warps;
#		 }
#
#		 # We are attempting to do the missing diffs, so reverse the list of retained warps.
#		 # Good objects with all visits will be skipped due to the duplicate check.
#		 # Bad objects will have the earliest visit rejected, and the visits repaired in a way that will produce all desired pairs.
#		 @{ $warpsBQ } = reverse @{ $warpsBQ };
#
#		 # Exclude the last entry if we do not have an even number of warps.
#		 if (($#{ $warpsBQ } + 1) % 2 != 0) {
#		     print STDERR "desp_diff_singles: Number of input warps to make dirty diffs is not even for target $target and object $this_object!";
#		     my $rejected_warp = pop @{ $warpsBQ };
#		     my $rejected_exp_id = ${ $rejected_warp }[0];
#		     print STDERR ": Rejecting ${rejected_exp_id} to force visit count.\n";
#		 }
#		 my $nwarpsBQ = ($#{$warpsBQ} + 1);
#	
#	
#		 if (($nwarpsBQ >= 2) && ($timediff < $desdiffdt)) {
#		     print STDERR "desp_diff_singles: There are potential dirty desperate diffs to be done, but the time criterium is not met.\n";
#		     next;
#		 }
#		 if (($nwarpsBQ <= 1) || ($timediff < $desdiffdt)) {
#		     next;
#		 }
#	
#		 while ($#{ $warpsBQ } > -1) {
#		     # The array is sorted in pairs of input/template.
#		     my $template_warp = shift @{ $warpsBQ };
#		     my $input_warp = shift @{ $warpsBQ };
#   
#		     my $input_exp_id = ${ $input_warp }[0];
#		     my $input_comment = ${ $input_warp }[3];
#
#		     my $template_exp_id = ${ $template_warp }[0];
#		     my $template_comment = ${ $template_warp }[3];
#
#		     my $input_warp_id = ${ $input_warp }[1];
#		     my $template_warp_id = ${ $template_warp }[1];
#
#		     my $input_warp_state = ${ $input_warp }[4];
#		     my $template_warp_state = ${ $template_warp }[4];
#
#		     my $input_warp_camQuality = ${ $input_warp }[5];
#		     my $template_warp_camQuality = ${ $template_warp }[5];
#
#		     $Npotential++;
#
#		     unless (defined($input_warp_id) && defined($template_warp_id) &&
#			 ($input_warp_state eq 'full')&&($template_warp_state eq 'full')) {
#			 print STDERR "Desp diff for this $date $target $input_exp_id $template_exp_id not fully processed ($input_warp_state $template_warp_state) ($input_warp_camQuality $template_warp_camQuality)\n";
#			 if (($input_warp_camQuality == 4007)||($template_warp_camQuality == 4007)) {
#			     # This should now never be reached.
#			     # CZW: Trigger backup plan here?  Or simply set up framework?
#			     print STDERR "  ...but this is due to a camera stage astrometry quality\n";
#			     $Npotential--;
#			 }
#			 next;
#		     }
#
#		     if (verify_uniqueness_diff($input_warp_id,$template_warp_id,$date,$target) != 0) {
#			 $Nqueued++;
#			 print STDERR "Desp diffs already queued for this $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
#			 next;
#		     }
#		     else {
#			 print STDERR "Preparing to dirty diff $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
#		     }
#	    
#		     my $cmd = "$difftool -dbname $dbname  -definewarpwarp  ";
#		     $cmd .= "-input_label $label  -template_label $label -good_frac 0.1 ";
#		     $cmd .= "-backwards "; # Needed because difftool assumes a different date sorting.
#		     $cmd .= "-rerun "; # Needed because we may have some diffs that already use some of the exposures
#		     $cmd .= "-set_workdir $workdir  -set_dist_group $dist_group  -set_data_group $data_group ";
#		     $cmd .= " -simple  -set_label $label -exp_id $input_exp_id -template_exp_id $template_exp_id ";
#		     if (defined($reduction)) {
#			 $cmd .= " -set_reduction $reduction ";
#		     }
#	
#		     if (defined($pretend)) {
#			 $cmd .= ' -pretend ';
#		     }
#		     if ($debug == 1) {
#			 $cmd .= ' -pretend ';
#			 print STDERR "desp_diff_singles: $cmd\n";
#			 print STDERR " $input_warp_id $template_warp_id\n";
#		     }
#	    
#		     if (($debug == 0)&&(!defined($pretend))) {
#			 my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
#			     run ( command => $cmd, verbose => $verbose );
#			 unless ($success) {
#			     $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
#			     &my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
#			 }
#			 $Nqueued++;
#		     }
#		 }
           
           
           
            }
        }
    }
    $metadata_out{nsDiffPotential} += $Npotential;
    $metadata_out{nsDiffQueued}    += $Nqueued;
}

sub desperate_diff_queue {
    my $date = shift;
    my $target = shift;
    my $filter = shift;
    my $maxFWHM = shift;
    my $pretend = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);
    my ($year,$month,$day,$hour,$min,$sec);
    my $this_date = undef;
    my $chunk_name = undef;

    my $db = init_gpc_db();

    my $obj_sth = "select rawExp.object,substr(rawExp.comment, 1, position(' ' in rawExp.comment)) from warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) ";
    $obj_sth .= " WHERE warpRun.state = 'full' AND warpRun.label = '$label' AND warpRun.data_group = '$data_group' AND rawExp.filter = '$filter' GROUP BY rawExp.object,substr(rawExp.comment, 1, position(' ' in rawExp.comment))";

    my $object_ref = $db->selectall_arrayref( $obj_sth );

    my $Npotential = 0;
    my $Nqueued = 0;

    #Before considering making diffs, we should check if there are no issues with summit download and/or registration.
    #summitExp has limited information, so see if the number of exps in rawExp and summitExp match up before most recent dateobs of chunk
    #be careful here, since dateobs is not quite completely the same in both tables. Therefore, allow numbers to be off by 1
    my $timenow=DateTime->now;
    $timenow->subtract(minutes => 15);
    $timenow->set_time_zone("UTC");

    my $dateinit = $timenow->ymd;
    my $raw_sth = "select count(dateobs),MAX(dateobs) FROM rawExp WHERE exp_name LIKE 'o%' AND dateobs > '$dateinit' AND dateobs <= '$timenow' ";
    my ($nrawexps,$rawdate) = $db->selectrow_array( $raw_sth );

    my $summit_sth = "select count(dateobs),MAX(dateobs) FROM summitExp WHERE exp_name LIKE 'o%' AND dateobs > '$dateinit' AND dateobs <= '$timenow' ";
    my ($nsummitexps,$summitdate) = $db->selectrow_array( $summit_sth );
 
    my $summittimediff = 9999;
    if(($rawdate)&&($summitdate)) {
        $rawdate=~/^(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2})$/;
        my $date1=DateTime->new(year=>$1,month=>$2,day=>$3,hour=>$4,minute=>$5,second=>$6,time_zone=>"UTC");
 
        $summitdate=~/^(\d{4})\-(\d{2})\-(\d{2}) (\d{2}):(\d{2}):(\d{2})$/;
        my $date2=DateTime->new(year=>$1,month=>$2,day=>$3,hour=>$4,minute=>$5,second=>$6,time_zone=>"UTC");
       
        my $difference=$date2->delta_ms($date1);

        $summittimediff=(($difference->hours)*60.) + ($difference->minutes) + (($difference->seconds)/60.);
    }

    #Run some basic download/registration checks
    my $noff = abs($nsummitexps-$nrawexps);
    if (($summittimediff > 10.)||($noff >10)) {
        if ($debug == 1) {
        print STDERR "desp_diff_queue: No desperate diffs will be attempted, since the number of exps at the summit and in rawExp do not match ($nsummitexps $nrawexps), or the timestamps are off ($summittimediff)\n";
        }
    }
    else {
        foreach my $object_row (@{ $object_ref }) {
            my $this_object = shift @{ $object_row };
            my $this_chunk = shift @{ $object_row };
       
            my $input_sth = "select exp_id,warp_id,dateobs,rawExp.comment,warpRun.state AS warp_state,camProcessedExp.quality,camProcessedExp.fwhm_major,chipRun.state,camRun.state FROM ";
            $input_sth .=   " rawExp LEFT JOIN chipRun USING (exp_id) LEFT JOIN camRun USING (chip_id) LEFT JOIN camProcessedExp USING(cam_id) LEFT JOIN fakeRun USING (cam_id) LEFT JOIN warpRun USING (fake_id) ";
            $input_sth .=   " WHERE chipRun.label = '$label' AND chipRun.data_group = '$data_group' AND rawExp.filter = '$filter' AND rawExp.object = '$this_object' ";
            $input_sth .=   " AND substr(rawExp.comment, 1, position(' ' in rawExp.comment)) = '$this_chunk' ORDER BY rawExp.comment,dateobs ";

            my $warps = $db->selectall_arrayref( $input_sth );
            my $warpsBQ = $db->selectall_arrayref( $input_sth );;








            # Each comment should only appear once. Therefore, if we see it more than once, we assume the first is extra.
            my %comment_hash = ();
            my %comment_hash_good = ();
            my $Nbad = 0;
            foreach my $this_warp (@{ $warps }) {
                my $this_comment = ${ $this_warp }[3];
                my $this_exp_id  = ${ $this_warp }[0];
                my $this_quality = ${ $this_warp }[5];
                my $this_state   = ${ $this_warp }[4];
                my $this_fwhm   = ${ $this_warp }[6];

                if (($this_quality != 0) || ($this_state eq 'drop') || ($this_fwhm > $maxFWHM)) {
                    print STDERR "desp_diff_queue: excluding $this_exp_id for $this_object due to non-zero cam.quality $this_quality or state $this_state or FWHM $this_fwhm\n";
                }
                else {
                    $comment_hash_good{$this_comment} = $this_exp_id;
                }
               
                #also save the entries which are ok, but not best quality
                if (($this_quality == 0) && ($this_state ne 'drop') ) {
                    $comment_hash{$this_comment} = $this_exp_id;
                }
            }

            # Exclude any warps that are not stored in the comment_hash (overrides) and check their status.
            my @keep_warps = ();
            my $Nbad = 0;
            foreach my $this_warp (@{ $warps }) {
                my $this_comment = ${ $this_warp }[3];
                my $this_exp_id  = ${ $this_warp }[0];
                my $this_quality = ${ $this_warp }[5];
                my $this_warp_id  = ${ $this_warp }[1];
                my $chip_state   = ${ $this_warp }[7];
                my $cam_state   = ${ $this_warp }[8];
                if ((exists($comment_hash_good{$this_comment}))&&
                    ($comment_hash_good{$this_comment} == $this_exp_id)) {
                    push @keep_warps, $this_warp;
                    $this_date  = ${ $this_warp }[2];
                    $chunk_name  = ${ $this_warp }[6];

	            #do not continue if you encounter exposures that finished cam stage but have not yet continued to warp (i.e. stuck in between stages)
                    if (($this_warp_id eq 'NULL') && (($this_quality == 0) || ($this_quality eq 'NULL')) ) {
                        print STDERR "desp_diff_queue: exposure with exp_id $this_exp_id for $this_object has warp_id $this_warp_id and quality $this_quality and is therefore not fully processed\n";
            	        $Nbad += 1;
                    }
	            #do not continue if you encounter exposures that have not been fully processed or have bad quality
                    if (($chip_state ne 'full') || ($cam_state ne 'full') || ($this_quality != 0) ) {
                        print STDERR "desp_diff_queue: exposure with exp_id $this_exp_id for $this_object has chip state $chip_state and cam state $cam_state and quality $this_quality and is therefore not fully processed\n";
            	        $Nbad += 1;
                    }
                }
                else {
                    print STDERR "desp_diff_queue: excluding $this_exp_id for $this_object due to not being an accepted comment string $this_comment\n";
                }
            }
            @{ $warps } = @keep_warps;
            my $nwarps = ($#{ $warps } + 1);

            #kick object out of diff consideration if it has exposures not fully processed
            if ($Nbad > 0) {
                print STDERR "desp_diff_queue: excluding $this_object from making diffs due to not being fully processed to warp stage\n";
                next;
            }

            ################################
            #We reach a junction here, where having more than 3 (but uneven) warps will result in desperate diffs
            #Conversely, less than 3 warps might result in dirty diffs
            ################################
            if($nwarps >= 3) {     
                # We are attempting to do the missing diffs, so reverse the list of retained warps.
                # Good objects with all visits will be skipped due to the duplicate check.
                # Bad objects will have the earliest visit rejected, and the visits repaired in a way that will produce all desired pairs.
                @{ $warps } = reverse @keep_warps;

                # Exclude the last entry if we do not have an even number of warps.
                if (($#{ $warps } + 1) % 2 != 0) {
                    print STDERR "desp_diff_queue: Number of input warps to make diffs is not even for target $target and object $this_object! $#{ $warps } ";
                    if ($#{ $warps} + 1 == 1) {
                        print STDERR "desp_diff_queue: I can do no diffs with only one exposure.\n";
                        next;
                    }
                    else {
                        my $rejected_warp = pop @{ $warps };
                        my $rejected_exp_id = ${ $rejected_warp }[0];
                        print STDERR "desp_diff_queue: Rejecting ${rejected_exp_id} to force visit count.\n";
                    }
                }
       
                while ($#{ $warps } > -1) {
                    # The array is sorted in pairs of input/template.
                    my $template_warp = shift @{ $warps };
                    my $input_warp = shift @{ $warps };
   
                    my $input_exp_id = ${ $input_warp }[0];
                    my $input_comment = ${ $input_warp }[3];

                    my $template_exp_id = ${ $template_warp }[0];
                    my $template_comment = ${ $template_warp }[3];

                    my $input_warp_id = ${ $input_warp }[1];
                    my $template_warp_id = ${ $template_warp }[1];

                    my $input_warp_state = ${ $input_warp }[4];
                    my $template_warp_state = ${ $template_warp }[4];

                    my $input_warp_camQuality = ${ $input_warp }[5];
                    my $template_warp_camQuality = ${ $template_warp }[5];

                    $Npotential++;

                    unless (defined($input_warp_id) && defined($template_warp_id) &&
                        ($input_warp_state eq 'full')&&($template_warp_state eq 'full')) {
                        print STDERR "desp_diff_queue: Desp diff for this $date $target $input_exp_id $template_exp_id not fully processed ($input_warp_state $template_warp_state) ($input_warp_camQuality $template_warp_camQuality)\n";
                        if (($input_warp_camQuality == 4007)||($template_warp_camQuality == 4007)) {
                            # This should now never be reached.
                            # CZW: Trigger backup plan here?  Or simply set up framework?
                            print STDERR "desp_diff_queue: ...but this is due to a camera stage astrometry quality\n";
                            $Npotential--;
                        }
                        next;
                    }

                    if (verify_uniqueness_diff($input_warp_id,$template_warp_id,$date,$target) != 0) {
                        $Nqueued++;
                        print STDERR "desp_diff_queue: Desp diffs already queued for this $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
                        next;
                    }
                    else {
                        print STDERR "desp_diff_queue: Preparing to desperate diff $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
                    }
           
                    my $cmd = "$difftool -dbname $dbname  -definewarpwarp  ";
                    $cmd .= "-input_label $label  -template_label $label -good_frac 0.1 ";
                    $cmd .= "-backwards "; # Needed because difftool assumes a different date sorting.
                    $cmd .= "-rerun "; # Needed because we may have some diffs that already use some of the exposures
                    $cmd .= "-set_workdir $workdir  -set_dist_group $dist_group  -set_data_group $data_group ";
                    $cmd .= " -simple  -set_label $label -exp_id $input_exp_id -template_exp_id $template_exp_id ";
                    if (defined($reduction)) {
                        $cmd .= " -set_reduction $reduction ";
                    }
       
                    if (defined($pretend)) {
                        $cmd .= ' -pretend ';
                    }
                    if ($debug == 1) {
                        $cmd .= ' -pretend ';
                        print STDERR "desp_diff_queue: $cmd\n";
                        print STDERR " $input_warp_id $template_warp_id\n";
                    }
           
                    if (($debug == 0)&&(!defined($pretend))) {
                        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                            run ( command => $cmd, verbose => $verbose );
                        unless ($success) {
                            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                            &my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
                        }
                        $Nqueued++;
                    }
                }           
           
            } else {       
                print STDERR "desp_diff_queue: There is a possibility for dirty diffs to be made, but this is currently not implemented\n";
                #Consider the case of on-the-fly dirty desperate diffs
                #The conditions are: uneven nr of bad quality warps greater than 3, and the most recent observations is more than $desdiffdt hours old

#                # Exclude any warps that are not stored in the more general comment_hash.
#                my @keep_warps = ();
#                foreach my $this_warp (@{ $warpsBQ }) {
#                    my $this_comment = ${ $this_warp }[3];
#                    my $this_exp_id  = ${ $this_warp }[0];
#                    if ((exists($comment_hash{$this_comment}))&&
#                        ($comment_hash{$this_comment} == $this_exp_id)) {
#                        push @keep_warps, $this_warp;
#                        my $this_date  = ${ $this_warp }[2];
#                        my $chunk_name  = ${ $this_warp }[6];
#                    }
#                    else {
#                        print STDERR "desp_diff_queue: excluding $this_exp_id for $this_object due to being rejected $this_comment\n";
#                    }
#		 }
#		 @{ $warpsBQ } = @keep_warps;
#
#		 if ($nwarps >= 2) {
#		     #one regular diff has already been made. So, need only one more. Strip a good entry from the array 	    
#		     my $nrem = 0;
#		     my @keep_warps = ();
#		     foreach my $this_warp (@{ $warpsBQ }) {
#			 my $this_comment = ${ $this_warp }[3];
#			 my $this_exp_id  = ${ $this_warp }[0];
#			 my $this_fwhm   = ${ $this_warp }[7];
#			 if (($nrem <1) && ($this_fwhm <= $maxFWHM)) {
#			     $nrem++;
#			     print STDERR "desp_diff_queue: excluding $this_exp_id for $this_object from dirty desp diffs $this_comment\n";
#			 } else {
#			     push @keep_warps, $this_warp;			 
#			 }
#		     }
#		     @{ $warpsBQ } = @keep_warps;
#		 }
#
#		 # We are attempting to do the missing diffs, so reverse the list of retained warps.
#		 # Good objects with all visits will be skipped due to the duplicate check.
#		 # Bad objects will have the earliest visit rejected, and the visits repaired in a way that will produce all desired pairs.
#		 @{ $warpsBQ } = reverse @{ $warpsBQ };
#
#		 # Exclude the last entry if we do not have an even number of warps.
#		 if (($#{ $warpsBQ } + 1) % 2 != 0) {
#		     print STDERR "desp_diff_queue: Number of input warps to make dirty diffs is not even for target $target and object $this_object!";
#		     my $rejected_warp = pop @{ $warpsBQ };
#		     my $rejected_exp_id = ${ $rejected_warp }[0];
#		     print STDERR ": Rejecting ${rejected_exp_id} to force visit count.\n";
#		 }
#		 my $nwarpsBQ = ($#{$warpsBQ} + 1);
#	
#		
#		 while ($#{ $warpsBQ } > -1) {
#		     # The array is sorted in pairs of input/template.
#		     my $template_warp = shift @{ $warpsBQ };
#		     my $input_warp = shift @{ $warpsBQ };
#   
#		     my $input_exp_id = ${ $input_warp }[0];
#		     my $input_comment = ${ $input_warp }[3];
#
#		     my $template_exp_id = ${ $template_warp }[0];
#		     my $template_comment = ${ $template_warp }[3];
#
#		     my $input_warp_id = ${ $input_warp }[1];
#		     my $template_warp_id = ${ $template_warp }[1];
#
#		     my $input_warp_state = ${ $input_warp }[4];
#		     my $template_warp_state = ${ $template_warp }[4];
#
#		     my $input_warp_camQuality = ${ $input_warp }[5];
#		     my $template_warp_camQuality = ${ $template_warp }[5];
#
#		     $Npotential++;
#
#		     unless (defined($input_warp_id) && defined($template_warp_id) &&
#			 ($input_warp_state eq 'full')&&($template_warp_state eq 'full')) {
#			 print STDERR "Desp diff for this $date $target $input_exp_id $template_exp_id not fully processed ($input_warp_state $template_warp_state) ($input_warp_camQuality $template_warp_camQuality)\n";
#			 if (($input_warp_camQuality == 4007)||($template_warp_camQuality == 4007)) {
#			     # This should now never be reached.
#			     # CZW: Trigger backup plan here?  Or simply set up framework?
#			     print STDERR "  ...but this is due to a camera stage astrometry quality\n";
#			     $Npotential--;
#			 }
#			 next;
#		     }
#
#		     if (verify_uniqueness_diff($input_warp_id,$template_warp_id,$date,$target) != 0) {
#			 $Nqueued++;
#			 print STDERR "Desp diffs already queued for this $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
#			 next;
#		     }
#		     else {
#			 print STDERR "Preparing to dirty diff $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment $template_comment\n";
#		     }
#	    
#		     my $cmd = "$difftool -dbname $dbname  -definewarpwarp  ";
#		     $cmd .= "-input_label $label  -template_label $label -good_frac 0.1 ";
#		     $cmd .= "-backwards "; # Needed because difftool assumes a different date sorting.
#		     $cmd .= "-rerun "; # Needed because we may have some diffs that already use some of the exposures
#		     $cmd .= "-set_workdir $workdir  -set_dist_group $dist_group  -set_data_group $data_group ";
#		     $cmd .= " -simple  -set_label $label -exp_id $input_exp_id -template_exp_id $template_exp_id ";
#		     if (defined($reduction)) {
#			 $cmd .= " -set_reduction $reduction ";
#		     }
#	
#		     if (defined($pretend)) {
#			 $cmd .= ' -pretend ';
#		     }
#		     if ($debug == 1) {
#			 $cmd .= ' -pretend ';
#			 print STDERR "desp_diff_queue: $cmd\n";
#			 print STDERR " $input_warp_id $template_warp_id\n";
#		     }
#	    
#		     if (($debug == 0)&&(!defined($pretend))) {
#			 my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
#			     run ( command => $cmd, verbose => $verbose );
#			 unless ($success) {
#			     $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
#			     &my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
#			 }
#			 $Nqueued++;
#		     }
#		 }
	    
	    
	    
	     }
        }
    }
    $metadata_out{nsDiffPotential} += $Npotential;
    $metadata_out{nsDiffQueued}    += $Nqueued;
}


sub multi_date_diff_queue {
    my $date = shift;
    my $target = shift;
    my $filter = shift;
    my $pretend = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $db = init_gpc_db();

    my $obj_sth = "select DISTINCT rawExp.object from warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) ";
    $obj_sth .= " WHERE warpRun.state = 'full' AND warpRun.label = '$label' AND rawExp.filter = '$filter' ORDER BY rawExp.object";

    my $object_ref = $db->selectall_arrayref( $obj_sth );

    my $Npotential = 0;
    my $Nqueued = 0;
   
    foreach my $object_row (@{ $object_ref }) {
        my $this_object = shift @{ $object_row };
#       my $input_sth = "select exp_id,warp_id,dateobs,rawExp.comment from warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) ";
#       $input_sth .= " WHERE warpRun.state = 'full' AND warpRun.label = '$label' AND warpRun.data_group = '$data_group' AND rawExp.filter = '$filter' AND rawExp.object = '$this_object' ";
#       $input_sth .= " ORDER BY dateobs ";
       
        my $input_sth = "select exp_id,warp_id,dateobs,rawExp.comment,warpRun.state AS warp_state FROM ";
        $input_sth .=   " rawExp LEFT JOIN chipRun USING (exp_id) LEFT JOIN camRun USING (chip_id) LEFT JOIN fakeRun USING (cam_id) LEFT JOIN warpRun USING (fake_id) ";
        $input_sth .=   " WHERE warpRun.label = '$label' AND warpRun.data_group = '$data_group' AND rawExp.filter = '$filter' AND rawExp.object = '$this_object' ";
        $input_sth .=   " ORDER BY dateobs ";

        my $warps = $db->selectall_arrayref( $input_sth );

        # Each comment should only appear once. Therefore, if we see it more than once, we assume the first is extra.
        my %comment_hash = ();
        foreach my $this_warp (@{ $warps }) {
            my $this_comment = ${ $this_warp }[3];
            my $this_exp_id  = ${ $this_warp }[0];
            $comment_hash{$this_comment} = $this_exp_id;
        }
       
        if (($#{ $warps } + 1) % 2 != 0) {
            print STDERR "md_diff_queue: Number of input warps to make diffs is not even for target $target and object $this_object! $#{ $warps } ";
            if ($#{ $warps} + 1 == 1) {
                print STDERR ": I can do no diffs with only one exposure.\n";
                next;
            }
            else {
                print STDERR ": I should declare an exposure to be faulty.\n";
                my @keep_warps = ();
#               print "@{ $warps }\n";
                foreach my $this_warp (@{ $warps }) {
                    my $this_comment = ${ $this_warp }[3];
                    my $this_exp_id  = ${ $this_warp }[0];
                    if ($comment_hash{$this_comment} == $this_exp_id) {
                        push @keep_warps, $this_warp;
                    }
                    else {
                        print STDERR "md_diff_queue: excluding $this_exp_id for $this_object\n";
                    }
                }
                @{ $warps } = @keep_warps;
#               print "@{ $warps }\n";
            }
        }
       
        while ($#{ $warps } > -1) {
            my $input_warp = shift @{ $warps };
            my $input_exp_id = ${ $input_warp }[0];
            my $input_comment = ${ $input_warp }[3];

           
            my $template_warp = shift @{ $warps };

            my $template_exp_id = ${ $template_warp }[0];
           
            my $input_warp_id = ${ $input_warp }[1];
            my $template_warp_id = ${ $template_warp }[1];

            my $input_warp_state = ${ $input_warp }[4];
            my $template_warp_state = ${ $template_warp }[4];
           
            unless(defined($template_warp)&& defined($template_exp_id)) {
                print STDERR "md_diff received an undef! $input_exp_id $input_comment $this_object T: $template_warp V: @$template_warp\n";
                next;
            }
            $Npotential++;
           
            unless (defined($input_warp_id) && defined($template_warp_id) &&
                    ($input_warp_state eq 'full')&&($template_warp_state eq 'full')) {
                print STDERR "md_Diff for this $date $target $input_exp_id ($input_warp_id $input_warp_state) $template_exp_id ($template_warp_id $template_warp_state) not fully processed\n";
                next;
            }

            if (multi_date_verify_uniqueness_diff($input_warp_id,$template_warp_id,$date,$target) != 0) {
                $Nqueued++;
                print STDERR "md_Diffs already queued for this $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment\n";
                next;
            }

            my $new_data_group = "${data_group}.multi";
            my $cmd = "$difftool -dbname $dbname  -definewarpwarp ";
            $cmd .= "-input_label $label  -template_label $label -good_frac 0.1 ";
            $cmd .= "-backwards "; # Needed because difftool assumes a different date sorting.
            $cmd .= "-set_workdir $workdir  -set_dist_group $dist_group  -set_data_group $new_data_group ";
            $cmd .= " -simple  -set_label $label -exp_id $input_exp_id -template_exp_id $template_exp_id ";
            if (defined($reduction)) {
                $cmd .= " -set_reduction $reduction ";
            }

#               $cmd .= " -pretend ";
            if (defined($pretend)) {
                $cmd .= ' -pretend ';
            }
            if ($debug == 1) {
                $cmd .= ' -pretend ';
                print STDERR "md_Diffs would like to queue for this $date $target $input_exp_id $template_exp_id ($input_warp_id $template_warp_id) $this_object $input_comment\n";
                print STDERR "md_diff_queue: $cmd\n";
                print STDERR " $input_warp_id $template_warp_id\n";
            }
           
            if (($debug == 0)&&(!defined($pretend))) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run ( command => $cmd, verbose => $verbose );
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
                }
                $Nqueued++;
            }
        }
    }
    $metadata_out{nsDiffPotential} += $Npotential;
    $metadata_out{nsDiffQueued}    += $Nqueued;
#      if (($metadata_out{nsDiffPotential} == $metadata_out{nsDiffQueued})&&($metadata_out{nsObservingState} eq 'END_OF_NIGHT')) {
#       $metadata_out{nsDiffState} = 'FINISHED_DIFFS';
#      }       

}

sub offnight_diff_queue {
    my $date = shift;
    my $target = shift;
    my $filter = shift;
    my $pretend = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);

    my $db = init_gpc_db();

#    my $Npotential = 0;
#    my $Nqueued = 0;

    my ($lunation_start,$lunation_end) = get_lunation_extent($date);


# Get a list of exposures that could be diffed   
    my $new_data_group = "${data_group}.offnight";
    my $new_dist_group = "${dist_group}.offnight";
    my $check_cmd = "$difftool -dbname $dbname  -definewarpwarp ";
    $check_cmd .= "-input_label $label  -template_label $label -good_frac 0.1 ";
    $check_cmd .= "-set_workdir $workdir  -set_dist_group $new_dist_group  -set_data_group $new_data_group ";
    $check_cmd .= " -mintimediff 40000 ";
    $check_cmd .= " -pretend -simple  -rerun -set_label $label -filter $filter ";
    $check_cmd .= " -dateobs_begin ${lunation_start}T00:00:00 -dateobs_end ${lunation_end}T23:59:59 -distance 1.5 ";
    if (defined($reduction)) {
        $check_cmd .= " -set_reduction $reduction ";
    }

    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
        run ( command => $check_cmd, verbose => $verbose );
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform difftool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
    }
   
    # Parse results
    my $diffs = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata from difftool -definewarpwarp", 0, 0, $date, $PS_EXIT_PROG_ERROR);
    foreach my $diff (@$diffs) {
        unless (multi_date_verify_uniqueness_diff($diff->{input_warp_id},$diff->{template_warp_id},$date,$target)) {
            # If we don't already have a diff with these inputs, make a diff with these inputs.
            my $cmd = "$difftool -dbname $dbname -definewarpwarp -good_frac 0.1 ";
            $cmd   .= "-input_label $label -template_label $label ";
            $cmd   .= "-warp_id $diff->{input_warp_id} -template_warp_id $diff->{template_warp_id} ";
            $cmd .= "-set_workdir $workdir  -set_dist_group $new_dist_group  -set_data_group $new_data_group ";
            $cmd .= " -mintimediff 40000 ";
            $cmd .= " -simple  -rerun -set_label $label -filter $filter ";
            $cmd .= " -dateobs_begin ${lunation_start}T00:00:00 -dateobs_end ${lunation_end}T23:59:59 -distance 1.5 ";
            if (defined($reduction)) {
                $cmd .= " -set_reduction $reduction ";
            }

            if (defined($pretend)) {
                $cmd .= ' -pretend ';
            }
            print STDERR "ON_diffs wants to run this command: $cmd\n";
           
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

sub intranight_WS_diff_queue {
    my $date = shift;
    my $target = shift;
    my $filter = shift;
    my $pretend = shift;
    my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) = get_tool_parameters($date,$target);
    my $db = init_gpc_db();

    my ($stacks_to_finish,$stacks_total) = check_stack_count($label);
    if (($stacks_to_finish == 0)&&($stacks_total != 0)) {
        my $cmd = "$difftool -dbname $dbname -definewarpstack ";
        $cmd .= " -good_frac 0.2 ";
        $cmd .= " -warp_label $label -stack_label $label -set_label $label ";
        $cmd .= " -set_workdir $workdir -available -set_reduction WARPSTACK -set_dist_group $dist_group ";
        $cmd .= " -rerun ";
    }
}           

sub check_stack_count {
    my $label = shift;

    my $stacks_to_finish = 0;
    my $stacks_total = 0;

    my $sth = "SELECT DISTINCT stack_id from stackRun where label = '$label' AND state = 'new'";
    my $data_ref = $db->selectall_arrayref( $sth );
    $stacks_to_finish = $#{ $data_ref } + 1;

    $sth = "SELECT DISTINCT stack_id from stackRun where label = '$label'";
    $data_ref = $db->selectall_arrayref( $sth );
    $stacks_total = $#{ $data_ref } + 1;
    return($stacks_to_finish,$stacks_total);
}


#
# Auto-Clean
################################################################################

sub construct_cleantool_args {
    my $date = shift;
    my $target = shift;
    my $mode = shift;

    my $command = $clean_config{$mode}{COMMAND};
    my $retention_time;
    if (exists($science_config{$target}{$mode})) {
        $retention_time = $science_config{$target}{$mode};
    }
    else {
        $retention_time = $clean_config{$mode}{RETENTION_TIME};
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
   
    $command =~ s/\@DBNAME\@/$dbname/;
    $command =~ s/\@LABEL\@/$label/;
    $command =~ s/\@WORKDIR\@/$workdir/;
    $command =~ s/\@OBSMODE\@/$obs_mode/;
    $command =~ s/\@OBJECT\@/$object/;
    $command =~ s/\@TESS_ID\@/$tess_id/;
    $command =~ s/\@DIST_GROUP\@/$dist_group/;
    $command =~ s/\@DATA_GROUP\@/$data_group/;
    $command =~ s/\@REDUCTION\@/$reduction/;
    $command =~ s/\@CURRENT_DATE\@/$cleaning_date/;
   
    if ($debug == 1) {
        $command .= ' -pretend ';
    }
    return($cleaning_date,$command);
}

sub execute_cleans {
    my $date = shift;
    my $pretend = shift;

    foreach my $mode (sort (keys %clean_config)) {
        foreach my $target (sort (keys %science_config)) {
            if (exists($science_config{$target}{NOCLEAN})) {
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
            if ($mode eq 'DIFF') {
                my $WS_command = $command;
                $WS_command =~ s/$target/${target}.WS/;
                print STDERR "$WS_command\n";
                if (!(defined($pretend) || $debug == 1)) {
                   
                    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                        run ( command => $WS_command, verbose => $verbose );
                    unless ($success) {
                        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                        &my_die("Unable to perform cleantool ($WS_command): $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
                    }
                }
            }

        }
    }
    return(0);
}

#
# Utilities
################################################################################

sub get_observing_state {
    my $date = shift;
    my $eon_dt = DateTime->new( year   => $datetime->year,
                                month  => $datetime->month,
                                day    => $datetime->day,
                                hour   => 17, # 7,
                                minute => 30,
                                second => 0,
                                time_zone => 'UTC');
   
    foreach my $eon (keys %eon_config) {
        my $command = "$regtool -processedexp -simple ";
        $command .= " -dbname $dbname ";
        $command .= " -dateobs_begin ${date}T00:00:00 -dateobs_end ${date}T23:59:59 ";
        $command .= " -object $eon_config{$eon}{OBJECT} " if defined($eon_config{$eon}{OBJECT});
        $command .= " -obs_mode $eon_config{$eon}{OBSMODE} " if defined($eon_config{$eon}{OBSMODE});
        $command .= " -exp_type $eon_config{$eon}{EXPTYPE} " if defined($eon_config{$eon}{EXPTYPE});
        $command .= " -comment $eon_config{$eon}{COMMENT} " if defined($eon_config{$eon}{COMMENT});

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run ( command => $command, verbose => $verbose );
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform regtool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
        }
        my @eon_exposures = split /\n/, (join '', @$stdout_buf);
        if ($#eon_exposures >= 0) {
            return("END_OF_NIGHT");
        }
    }   
    if ($force_registration) {
        return("END_OF_NIGHT");
    }
#    print "$now $eon_dt " . DateTime->compare($now,$eon_dt) . "\n";
    if (DateTime->compare($now,$eon_dt) < 1) {
        return("OBSERVING");
    }
    else {
        return("END_OF_NIGHT");
    }
}

# This basically does the end of night check, but does it "the hard way," to prevent the time from fooling us.
sub get_registration_state {
    my $date = shift;

    foreach my $eon (keys %eon_config) {
        my $command = "$regtool -processedexp -simple ";
        $command .= " -dbname $dbname ";
        $command .= " -dateobs_begin ${date}T00:00:00 -dateobs_end ${date}T23:59:59 ";
        $command .= " -object $eon_config{$eon}{OBJECT} " if defined($eon_config{$eon}{OBJECT});
        $command .= " -obs_mode $eon_config{$eon}{OBSMODE} " if defined($eon_config{$eon}{OBSMODE});
        $command .= " -exp_type $eon_config{$eon}{EXPTYPE} " if defined($eon_config{$eon}{EXPTYPE});
        $command .= " -comment $eon_config{$eon}{COMMENT} " if defined($eon_config{$eon}{COMMENT});

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run ( command => $command, verbose => $verbose );
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform regtool: $error_code", 0,0,$date, $PS_EXIT_SYS_ERROR);
        }
        my @eon_exposures = split /\n/, (join '', @$stdout_buf);
        if ($#eon_exposures >= 0) {
            return("REGISTERED");
        }
    }   
    return("NOT_REGISTERED");
}

   

sub get_tool_parameters {
    my $date = shift;
    my $target = shift;
    my $workdir_date = $date; $workdir_date =~ s%-%/%g;
    my $trunc_date = $date; $trunc_date =~ s/-//g;

    my $label      = "${target}.nightlyscience";
    my $workdir    = "neb://\@HOST\@.0/${dbname}/${target}.nt/${workdir_date}";
    my $obs_mode   = $science_config{$target}{OBSMODE};
    my $object     = $science_config{$target}{OBJECT};
    my $comment    = $science_config{$target}{COMMENT};
    my $dist_group = $science_config{$target}{DISTRIBUTION};
    my $data_group = "${target}.${trunc_date}";
    my $tess_id    = $science_config{$target}{TESS};
    my $reduction  = $science_config{$target}{REDUCTION};
    return($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction);
}

sub get_dettool_parameters {
    my $date = shift;
    my $target = shift;
    my $workdir_date = $date; $workdir_date =~ s%-%/%g;
    my $trunc_date = $date; $trunc_date =~ s/-//g;

    my $exp_type   = $detrend_config{$target}{EXPTYPE};
    my $det_type   = $detrend_config{$target}{DETTYPE};
    my $ref_det_id = $detrend_config{$target}{REF_ID};
    my $ref_iter   = $detrend_config{$target}{REF_ITER};
    my $maxN       = $detrend_config{$target}{MAX_EXP};
    my $det_filter = $detrend_config{$target}{DET_FILTER};
    my $internal_filter;
    if (defined($det_filter)) {
        $internal_filter = $det_filter; $internal_filter =~ s/\..*//;
        $internal_filter = '.' . $internal_filter;
    }
    else {
        $internal_filter = '';
    }
    unless (defined($maxN)) {
        $maxN = 0;
    }
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

    if ((defined($macro_config{$proc_mode}))&&($do_or_do_not)) {
        unless (defined($metadata_out{N_MACROS})) {
            $metadata_out{N_MACROS} = 0;
        }
        my $N = $metadata_out{N_MACROS};
        $metadata_out{"ns${N}Macro"} = $macro_config{$proc_mode};
        if ($debug == 1) {
            print STDERR "WORKING ON A MACRO: ns${N}Macro $proc_mode $macro_config{$proc_mode}\n";
        }
        if (defined($date)&&(defined($target))) {

            my ($label,$workdir,$obs_mode,$object,$comment,$tess_id,$dist_group,$data_group,$reduction) 
                = get_tool_parameters($date,$target);
            $metadata_out{"ns${N}Macro"} =~ s/\@LABEL\@/$label/;
            $metadata_out{"ns${N}Macro"} =~ s/\@WORKDIR\@/$workdir/;
            $metadata_out{"ns${N}Macro"} =~ s/\@OBSMODE\@/$obs_mode/;
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
            print STDERR "DONE WITH A MACRO: ns${N}Macro $proc_mode $macro_config{$proc_mode}\n";
        }
        $metadata_out{N_MACROS} ++;
    }
}

sub init_gpc_db {
    ## change to use the siteConfig setting, while readonly probably do not want to use a readonly replicated DB incase it gets behind
    my $dbserver = metadataLookupStr($siteConfig, 'DBSERVER');
    my $dbuser = metadataLookupStr($ipprc->{_siteConfig}, "RO_DBUSER");
    my $dbpass = metadataLookupStr($ipprc->{_siteConfig}, "RO_DBPASSWORD");
    die "database configuration not set up" unless defined($dbserver);
    die "database configuration not set up" unless defined($dbuser);
    die "database configuration not set up" unless defined($dbpass);
    #my $dbserver = 'ippdb01';
    #my $dbuser = 'ippuser';
    #my $dbpass = 'ippuser';
    use constant DB_SOCKET => '/var/run/mysqld/mysqld.sock';
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
    print STDOUT "   key                   STR          ${date}-${dbname}\n";
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
    my $date = shift;
    my $exit_code = shift; # Exit code

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
