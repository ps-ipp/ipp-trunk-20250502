#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
my $cmd_line = join ' ', @ARGV;
print "Starting script $0 $cmd_line on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use Cache::File;
use Storable qw(freeze thaw);
use File::Basename qw( basename);
use IPC::Run;
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Config 1.01 qw( :standard );
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Metadata::Config;
use Math::Trig;

use File::Spec;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $regtool = can_run( 'regtool' ) or (warn "Can't find regtool" and $missing_tools = 1);
my $ppStats = can_run( 'ppStats' ) or (warn "Can't find ppStats" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run( 'ppStatsFromMetadata' ) or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $ppConfigDump = can_run( 'ppConfigDump' ) or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ippApplyBurntoolSingle = can_run( 'ipp_apply_burntool_single.pl' ) or (warn "Can't find ipp_apply_burntool_single" and $missing_tools = 1);

my ($cache, $exp_id, $tmp_class_id, $tmp_exp_name, $uri, $bytes, $md5sum, $dbname, $verbose, $no_update, $no_op, $logfile);
my ($sunrise, $sunset, $summit_dateobs);
GetOptions(
    'caches'           => \$cache,
    'exp_id|e=s'       => \$exp_id,
    'tmp_class_id|i=s' => \$tmp_class_id,
    'tmp_exp_name|n=s' => \$tmp_exp_name,
    'uri|u=s'          => \$uri,
    'bytes=s'          => \$bytes,
    'md5sum=s'         => \$md5sum,
    'sunrise=s'        => \$sunrise,
    'sunset=s'         => \$sunset,
    'summit_dateobs=s' => \$summit_dateobs,
    'dbname|d=s'       => \$dbname,    # Database name
    'verbose'          => \$verbose,   # Print to stdout
    'no-update'        => \$no_update,
    'no-op'            => \$no_op,
    'logfile=s'        => \$logfile,
) or pod2usage( 2 );

my $ipprc = PS::IPP::Config->new() or my_die_for_add( "Unable to set up", $exp_id, $tmp_exp_name, $tmp_class_id, $uri, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$ipprc->redirect_to_logfile($logfile) or my_die_for_add( "Unable to redirect output", $exp_id, $tmp_exp_name, $tmp_class_id, $uri, $PS_EXIT_SYS_ERROR ) if $logfile;

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --exp_id --tmp_class_id --tmp_exp_name --uri",
           -exitval => 3) unless
    defined $exp_id and
    defined $tmp_class_id and
    defined $tmp_exp_name and
    defined $uri;

unless (defined($sunset)) {
    $sunset = '03:30:00';
}
unless (defined($sunrise)) {
    $sunrise = '17:30:00';
}

my $RECIPE = "REGISTER"; # Recipe to use for ppStats

if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR);
}

# setup cache interface
# XXX why is this being cached?
my $c = Cache::File->new(
    cache_root => File::Spec->catdir($ENV{'HOME'}, '.pxtools', basename($0)),
    default_expires => '7200 sec',
);

my $now_time = localtime();
printf STDERR "\nstarting ppStats: %s\n", $now_time if $verbose;

my $cmdflags;
my $burntoolStateTarget;
my $burntoolStateCurrent;
# Run ppStats on the input file
{
    my $command1 = "$ppStats $uri -recipe PPSTATS $RECIPE -level";
    my $command2 = "$ppStatsFromMetadata - - REGISTER_IMFILE";

    # Since there are no spaces in the arguments, we can get away with this:
    my @command1 = split(/ /, $command1);
    my @command2 = split(/ /, $command2);

    # Run ppStats
    my ($in1, $out1, $err1);    # Buffers for ppStats
    my $h1 = IPC::Run::harness \@command1, \$in1, \$out1, \$err1;
    print "[Running $command1]\n";
    my $result1 = IPC::Run::run $h1;
    print "STDOUT:\n$out1";
    print "STDERR:\n$err1";
    &my_die_for_add("Unable to perform ppStats on exposure id $exp_id: " . $h1->result(), $exp_id, $tmp_exp_name, $tmp_class_id, $uri, ($h1->result() or $PS_EXIT_PROG_ERROR) ) unless $result1;

    print "[Running " . join(' ', @command2) . "]\n";
    my ($out2, $err2);          # Buffers for ppStatsFromMetadata
    my $h2 = IPC::Run::harness \@command2, \$out1, \$out2, \$err2;
    print "[Running $command2]\n";
    my $result2 = IPC::Run::run $h2;
    print "STDOUT:\n$out2";
    print "STDERR:\n$err2";
    &my_die_for_add("Unable to perform ppStatsFromMetadata on exposure id $exp_id: " . $h2->result(), $exp_id, $tmp_exp_name, $tmp_class_id, $uri, ($h2->result() or $PS_EXIT_PROG_ERROR) ) unless $result2;
    chomp $out2;
    $cmdflags = $out2;

    # Manually parse the burntool_state entry.
    $burntoolStateCurrent = 0;
    my $isGPC1 = 0;
    $burntoolStateTarget = 0;
    foreach my $line (split /\n/, $out1) {
        if ($line =~ /FPA.BURNTOOL.APPLIED/) {
            $line =~ s/^\s+//;
            $burntoolStateCurrent = (split /\s+/, $line)[2];
        }
        if ($line =~ /FPA.CAMERA/) {
            $line =~ s/^\s+//;
            if ((split /\s+/, $line)[2] eq 'GPC1') {
                $isGPC1 = 1;
            }
        }
    }
    if ($isGPC1 != 1) {
        $burntoolStateCurrent = 0; # If it's not GPC1, you shouldn't have run burntool.
    }
    elsif (($isGPC1 == 1) && ($burntoolStateCurrent == 1)) {
#       print STDERR "In the good region: >>$burntoolStateCurrent<<\n";
        my $ppConfigDump_cmd = "$ppConfigDump -camera GPC1 -get-key BURNTOOL.STATE.GOOD";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            IPC::Cmd::run(command => $ppConfigDump_cmd, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn ("Unable to perform ppConfigDump");
            exit($error_code);
        }

        # This is ugly, but doing a full parse for one entry is a bit wasteful.
        foreach my $line (split /\n/, (join "", @$stdout_buf)) {
            if ($line =~ /BURNTOOL.STATE.GOOD/) {
                $line =~ s/^\s+//;
                $burntoolStateTarget = (split /\s+/, $line)[2];
                last;
            }
        }
        # XXX why was this being equated??
        # $burntoolState = $burntoolStateGood; # Positive because this has the header table.

    }
    $cmdflags .= " -burntool_state $burntoolStateTarget ";
}

$now_time = localtime();
printf STDERR "\ndone with ppStats: %s\n", $now_time if $verbose;
printf STDERR "\nburntool state current: %d target: %d\n", $burntoolStateCurrent, $burntoolStateTarget;

# we require at a minimum: -telescope, -inst, -filelevel, -class_id, -exp_type
if (uc(&value_for_flag ($cmdflags, "NULL", "-telescope")) eq "NULL") { &my_die_for_add ("telescope not found", $exp_id, $tmp_exp_name, $tmp_class_id, $uri, $PS_EXIT_CONFIG_ERROR); }
if (uc(&value_for_flag ($cmdflags, "NULL", "-inst"))      eq "NULL") { &my_die_for_add ("inst      not found", $exp_id, $tmp_exp_name, $tmp_class_id, $uri, $PS_EXIT_CONFIG_ERROR); }
if (uc(&value_for_flag ($cmdflags, "NULL", "-filelevel")) eq "NULL") { &my_die_for_add ("filelevel not found", $exp_id, $tmp_exp_name, $tmp_class_id, $uri, $PS_EXIT_CONFIG_ERROR); }
if (uc(&value_for_flag ($cmdflags, "NULL", "-class_id"))  eq "NULL") { &my_die_for_add ("class_id  not found", $exp_id, $tmp_exp_name, $tmp_class_id, $uri, $PS_EXIT_CONFIG_ERROR); }
if (uc(&value_for_flag ($cmdflags, "NULL", "-exp_type"))  eq "NULL") { &my_die_for_add ("exp_type  not found", $exp_id, $tmp_exp_name, $tmp_class_id, $uri, $PS_EXIT_CONFIG_ERROR); }
my $dateobs   = &value_for_flag($cmdflags, 0.0, "-dateobs");
unless($summit_dateobs) {
    $summit_dateobs = $dateobs;
}

my $exp_type  = &value_for_flag($cmdflags, "NULL", "-exp_type");

my $command = "$regtool -addprocessedimfile";
$command .= " -exp_id $exp_id";
$command .= " -exp_name $tmp_exp_name"; # keep the supplied exp_name (could be derived from the file)
$command .= " -tmp_class_id $tmp_class_id"; # the original class_id supplied by the user, replace by ppStats CLASS.ID
$command .= " -uri $uri ";
$command .= " -bytes $bytes" if $bytes;
$command .= " -md5sum $md5sum" if $md5sum and ($md5sum ne 'NULL');
$command .= " -hostname $host" if defined $host;
$command .= " -dbname $dbname" if defined $dbname;
if (abs($burntoolStateCurrent) == $burntoolStateTarget) {
    printf STDERR "This has already been burned.\n";
    $command .= " -data_state full";
}
elsif (is_daytime($summit_dateobs,$sunrise,$sunset)) {
    printf STDERR "This is a daytime exposure.\n";
    $command .= " -data_state full";
}
elsif (is_ccim($tmp_exp_name,$exp_type)) {
    printf STDERR "This is a camera commanded detrend exposure that should not cause a burn.\n";
    $command .= " -data_state full";
}    
else {
    printf STDERR "Need to check burntool.\n";
    $command .= " -data_state check_burntool";
}
$command .= " $cmdflags";

print "$command\n";
# determine solar-system parameters
my $longitude = &value_for_flag($cmdflags, 0.0, "-longitude");
my $latitude  = &value_for_flag($cmdflags, 0.0, "-latitude");
my $elevation = &value_for_flag($cmdflags, 0.0, "-elevation");
my $ra        = &value_for_flag($cmdflags, 0.0, "-ra");
my $dec       = &value_for_flag($cmdflags, 0.0, "-decl");


# if the needed data is available, pass it to sunmoon:
if ($longitude && $latitude && $ra && $dec && $dateobs) {

    $longitude *= 12.0 / pi; # longitude is reported in West radians; sunmoon wants it in West Hours
    $latitude *= 180.0 / pi; # latitude is reported in North radians; sunmoon wants it in North Degrees
    $ra *= 180.0 / pi; # ra is reported in radians; sunmoon wants it in degrees
    $dec *= 180.0 / pi; # dec is reported in radians; sunmoon wants it in degrees

    my $sunmoon_cmd = "sunmoon -latitude $latitude -longitude $longitude -elevation $elevation $dateobs $ra $dec";
    my $sunmoon_data = `$sunmoon_cmd`;
    chomp $sunmoon_data;

    # print STDERR "run: $sunmoon_cmd\n";
    # print STDERR "got: $sunmoon_data\n";

    if ($?) {
        warn ("failure running $sunmoon_cmd, not supplying\n");
    } else {
        $command .= " $sunmoon_data";
    }
}

# This might have a race condition
# $date_end = $dateobs;
# $date_start = $dateobs - 30 minutes ? dateobs_UTC_midnight?
# lock file?
# if exp_type = DARK and date > MIDNIGHT HST { wait }
# system("ipp_apply_burntool.pl --class_id $class_id --dateobs_begin $date_start --dateobs_end $date_end --dbname gpc1 --logfile /data/$host.0/burntool_logs/$class_id.$start_date.log");

$now_time = localtime();
printf STDERR "\nrunning regtool update: %s\n", $now_time if $verbose;

# Push the results into the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        IPC::Cmd::run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn ("Unable to perform regtool -addprocessedimfile: $error_code");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

# We now have an imfile in the database, check if we can burntool it.  If not, continue on.

if ((abs($burntoolStateCurrent) != $burntoolStateTarget)&&
    (!is_daytime($summit_dateobs,$sunrise,$sunset))&&
    (!is_ccim($tmp_exp_name,$exp_type))) {
    my $mdcParser  = PS::IPP::Metadata::Config->new;

    my $class_id   = &value_for_flag($cmdflags, 0.0, "-class_id");
    my $check_date       = &value_for_flag($cmdflags, 0.0, "-dateobs");
    $check_date =~ s/T.*$//;
    my $exp_name   = $tmp_exp_name;


    my $bt_check_command = "$regtool -checkburntoolimfile ";
    $bt_check_command .= " -class_id $class_id ";
    $bt_check_command .= " -dateobs_begin ${check_date}T${sunset} ";
    $bt_check_command .= " -dateobs_end ${check_date}T${sunrise} ";
    $bt_check_command .= " -valid_burntool $burntoolStateTarget ";
    $bt_check_command .= " -exp_name $exp_name ";
    $bt_check_command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run ( command => $bt_check_command, verbose => 0); 
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die_for_update("Unable to perform regtool: $error_code",
                           $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
    }

    my @whole = split /\n/, (join "", @$stdout_buf);
    my @single = ();
    my $exposures;
    while ( scalar @whole > 0 ) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
            push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) );
            &my_die_for_update("Unable to parse output from regtool",
                               $exp_id, $class_id, $PS_EXIT_SYS_ERROR) unless
                                   defined $list;
            push @{ $exposures }, @$list;
            @single = ();
        }
    }

    # We only care about the final entry, as that contains the exposure we are.

    my $regtool_update = "$regtool -updateprocessedimfile ";
    $regtool_update .= "-dbname $dbname " if defined $dbname;
    $regtool_update .= "-exp_id $exp_id -class_id $class_id ";

    my $burntool_data = pop(@{ $exposures });
    if ($burntool_data->{burnable} == 0) {
        $regtool_update .= " -burntool_state 0 -set_state pending_burntool ";
        unless ($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                IPC::Cmd::run(command => $regtool_update, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                warn ("Unable to perform regtool -updateprocessedimfile: $error_code");
#                exit($error_code);
            &my_die_for_update("Unable to perform updateprocessedimfile",
                               $exp_id, $class_id, $error_code);

            }
        } else {
            print "skipping command: $command\n";
        }
    }
    else {
        my $apply_command = "$ippApplyBurntoolSingle --dbname $dbname ";
        $apply_command .= " --class_id $class_id --exp_id $exp_id ";
        $apply_command .= " --this_uri $burntool_data->{uri} ";
	$apply_command .= " --camera GPC1 "; # hack, but we're only going to get here if we're GPC1.
        $apply_command .= " --previous_uri $burntool_data->{previous_uri} " if defined $burntool_data->{previous_uri};
        $apply_command .= " --imfile_state $burntool_data->{imfile_state} ";
        $apply_command .= " --verbose " if $verbose;

        # TEMPORARY
        $apply_command .= " --camera GPC1";
        print "$apply_command\n";
        unless ($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                IPC::Cmd::run(command => $apply_command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die_for_update("Unable to perform ipp_apply_burntool_single.pl: $error_code",
                               $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
            }
        }
    }


}



$now_time = localtime();
printf STDERR "\ndone with regtool update: %s\n", $now_time if $verbose;

sub cache_run
{
    my %p = @_;

    my $cmd_output = $c->get($p{command}) if $cache;
    if (defined $cmd_output) {
        return @{thaw $cmd_output};
    } else {
        my @output = IPC::Cmd::run(%p);
        $c->set($p{command}, freeze \@output) if $cache;
        return @output;
    }
}

sub value_for_flag
{
    my $cmdflags = shift;
    my $default = shift;
    my $flag = shift;

    my $value = $default;
    if ($cmdflags =~ m|$flag|) {
        ($value) = $cmdflags =~ m|$flag\s+(\S+)|;
    }
    $value;
}

sub is_ccim 
{
    my $exp_name = shift;
    my $exp_type = shift;
    $exp_type =~ s/'//g;
    # needs to match regtool.c checks for "is_ccim".
    if ($exp_name =~ /c/) {
	if (($exp_type eq 'DOMEFLAT')||
	    ($exp_type eq 'DARK')||
	    ($exp_type eq 'BIAS')) {
	    return(1);
	}
    }
    if ($exp_name =~ /a$/) {
	return(1);
    }
    return(0);
}

sub is_daytime
{
    my $dateobs = shift;
    my $sunset  = shift;
    my $sunrise = shift;

    my ($date,$time);

    if ($dateobs =~ /T/) {
	($date,$time) = split /T/, $dateobs;
    }
    else {
	($date,$time) = split / /, $dateobs;
    }
    my ($hour,$minute,$second) = split /\:/, $time; # /;
    my ($ss_hour,$ss_minute,$ss_second) = split /\:/, $sunset; # /;
    my ($sr_hour,$sr_minute,$sr_second) = split /\:/, $sunrise; # /;
    if ($second =~ /Z/) {
	$second =~ s/Z//;
    }
#     print "this exposure: $hour $minute $second\n";
#     print "sunset:        $ss_hour $ss_minute $ss_second\n";
#     print "sunrise:       $sr_hour $sr_minute $sr_second\n";
#     printf "Hss: %d Mss: %d Sss: %d\n",($hour >= $ss_hour),($minute >= $ss_minute),($second >= $ss_second);
#     printf "Hsr: %d Msr: %d Ssr: %d\n",($hour <= $sr_hour),($minute <= $sr_minute),($second <= $sr_second);

    if (($hour > $ss_hour)&&($hour <= 24)) {
	return(1); # After sunset by more than an hour, before midnight
    }
    elsif ($hour == $ss_hour) {
	if ($minute > $ss_minute) {
	    return(1); # After sunset by more than a minute
	}
	elsif ($minute == $ss_minute) {
	    if ($second >= $ss_second) {
		return(1); # After sunset by more than a second
	    }
	    else {
		return(0);
	    }
	}
	else {
	    return(0);
	}
    }
    elsif (($hour < $sr_hour)&&($hour >= 0)) {
	return(1); # Before sunrise by more than an hour, but after midnight
    }
    elsif ($hour == $sr_hour) {
	if ($minute < $sr_minute) {
	    return(1); # Before sunrise by more than a minute
	}
	elsif ($minute == $sr_minute) {
	    if ($second <= $sr_second) {
		return(1); # Before sunrise by more than a second
	    }
	    else {
		return(0);
	    }
	}
	else {
	    return(0);
	}
    }
    else {
	return(0); # We should never get here.
    }
}

sub my_die_for_add
{
    my $msg = shift; # Warning message on die
    my $exp_id = shift;
    my $exp_name = shift;
    my $tmp_class_id = shift;
    my $uri = shift;
    my $exit_code = shift;

    # for failed imfiles, we insert UNKNOWN for inst, telescope, class_id

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $exp_id && defined $tmp_class_id and not $no_update) {
        my $command = "$regtool -addprocessedimfile";
        $command .= " -exp_id $exp_id";
        $command .= " -exp_name $exp_name";
        $command .= " -tmp_class_id $tmp_class_id";
        $command .= " -uri $uri ";
        $command .= " -telescope UNKNOWN";
        $command .= " -inst UNKNOWN";
        $command .= " -class_id $tmp_class_id";
        $command .= " -fault $exit_code";
        $command .= " -hostname $host" if defined $host;
        $command .= " -dbname $dbname" if defined $dbname;
        print "Running: $command\n";
        system($command);
    }
    exit $exit_code;
}
sub my_die_for_update
{
    my $msg = shift; # Warning message on die
    my $exp_id = shift;
    my $class_id = shift;
    my $exit_code = shift;

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $exp_id && defined $class_id and not $no_update) {
	sleep(5);
        my $command = "$regtool -updateprocessedimfile";
        $command .= " -exp_id $exp_id";
        $command .= " -class_id $class_id";
        $command .= " -fault $exit_code";
	$command .= " -set_state pending_burntool ";
        $command .= " -hostname $host" if defined $host;
        $command .= " -dbname $dbname" if defined $dbname;
        print "Running: $command\n";
        system($command);
    }
    exit $exit_code;
}

# Pau.

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}

__END__
