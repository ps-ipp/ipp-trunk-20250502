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

use Cache::File;
use Storable qw( freeze thaw );
use File::Basename qw( basename );
use File::Temp qw( tempfile );                   # tools to construct temp files
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for commands we need
my $missing_tools;
my $regtool = can_run('regtool') or (warn "can't find regtool" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($cache, $exp_id, $exp_tag, $label, $dvodb, $end_stage, $tess_id, $dbname, $verbose, $no_update, $no_op, $save_temps, $logfile);
GetOptions(
    'caches'        => \$cache,
    'exp_id|e=s'    => \$exp_id,
    'exp_tag|t=s'   => \$exp_tag,
    'dbname|d=s'    => \$dbname, # Database name
    'label=s'       => \$label,
    'dvodb=s'       => \$dvodb,
    'end_stage=s'   => \$end_stage,
    'tess_id=s'     => \$tess_id,
    'verbose'       => \$verbose,   # Print to stdout
    'no-update'     => \$no_update,
    'no-op'         => \$no_op,
    'save-temps'    => \$save_temps, # Save temporary files?
    'logfile=s'     => \$logfile,
) or pod2usage( 2 );

my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $exp_id, $PS_EXIT_CONFIG_ERROR );
$ipprc->redirect_to_logfile($logfile) or my_die( "Unable to redirect output", $exp_id, $PS_EXIT_SYS_ERROR ) if $logfile;

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --exp_id --exp_tag",
           -exitval => 3) unless
    defined $exp_id and
    defined $exp_tag;

# add -detrend UNLESS type is one of SCIENCE listed below (eg, OBJECT)
my @SCIENCE = ( "object", "science", "light" ); # Observation types to NOT mark as detrend
my $DETREND_FLAG = "-end_stage reg"; # Flag to use to mark detrend exposure

# setup cache interface
my $c = Cache::File->new(
    cache_root => File::Spec->catdir($ENV{'HOME'}, '.pxtools', basename($0)),
    default_expires => '7200 sec',
);

my $cmdflags;

# Get the list of imfiles & their stats
{
    my $command = "$regtool -processedimfile -exp_id $exp_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        cache_run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn ("Unable to perform regtool -processedimfile on exposure id $exp_id: $error_code");
        exit ($error_code);
    }

    # since I can't figure out how to do input and output within PERL, I'm writing to a temp file
    my ($statFile, $statName) = tempfile( "/tmp/$exp_id.register.XXXX", UNLINK => !$save_temps );
    print "saving stats to $statName\n";
    foreach my $line (@$stdout_buf) {
        print $statFile $line;
    }
    close $statFile;

    # parse the stats in the metadata file
    $command = "$ppStatsFromMetadata $statName - REGISTER_EXP";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform ppStatsFromMetadata: $error_code\n");
        exit($error_code);
    }

    foreach my $line (@$stdout_buf) {
        $cmdflags .= " $line";
    }
    chomp $cmdflags;
}

# we require at a minimum: -telescope, -inst, -filelevel, -class_id, -exp_name, -exp_type
if (uc(&value_for_flag ($cmdflags, "-telescope")) eq "NULL") { &my_die ("telescope not found", $exp_id, $PS_EXIT_CONFIG_ERROR); }
if (uc(&value_for_flag ($cmdflags, "-inst"))      eq "NULL") { &my_die ("inst      not found", $exp_id, $PS_EXIT_CONFIG_ERROR); }
if (uc(&value_for_flag ($cmdflags, "-filelevel")) eq "NULL") { &my_die ("filelevel not found", $exp_id, $PS_EXIT_CONFIG_ERROR); }
if (uc(&value_for_flag ($cmdflags, "-exp_name"))  eq "NULL") { &my_die ("exp_name  not found", $exp_id, $PS_EXIT_CONFIG_ERROR); }

my $exp_type = &value_for_flag ($cmdflags, "-exp_type");
if (uc($exp_type) eq "NULL") { &my_die ("exp_type  not found", $exp_id, $PS_EXIT_CONFIG_ERROR); }

my ($data_group,$dist_group,$chip_workdir,$reduction);
($label,$data_group,$dist_group,$end_stage,
 $tess_id,$chip_workdir,$reduction) = advance_decisions(&value_for_flag($cmdflags,"-exp_name"),
							&value_for_flag($cmdflags,"-dateobs"),
							&value_for_flag($cmdflags,"-exp_type"),
							&value_for_flag($cmdflags,"-obs_mode"),
							&value_for_flag($cmdflags,"-object"),
							&value_for_flag($cmdflags,"-comment"),
							$dbname);
my $command = "$regtool -addprocessedexp";
$command .= " -exp_id $exp_id";
$command .= " -exp_tag $exp_tag";
$command .= " -hostname  $host"      if defined $host;
$command .= " -dbname    $dbname"    if defined $dbname;
$command .= " -label     $label"     if defined $label;
$command .= " -dvodb     $dvodb"     if defined $dvodb;
$command .= " -end_stage $end_stage" if defined $end_stage;
$command .= " -tess_id   $tess_id"   if defined $tess_id;
$command .= " -data_group $data_group" if defined $data_group;
$command .= " -dist_group $dist_group" if defined $dist_group;
$command .= " -chip_workdir $chip_workdir" if defined $chip_workdir;
$command .= " -reduction  $reduction"  if defined $reduction;
$command .= " -state full";
$command .= " $cmdflags";

# Add the detrend flag, if needed
{
    my $object = 0;             # Is it an object exposure?
    foreach my $scienceType (@SCIENCE) {
        if (lc($exp_type) =~ /$scienceType/) {
            $object = 1;
            last;
        }
    }
    $command .= " $DETREND_FLAG" unless $object;
}

# Output results to the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        cache_run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn ("Unable to run regtool -addprocessedexp for $exp_id: $error_code");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

### Pau.
sub advance_decisions {
    my $exp_name = shift;
    my $dateobs = shift;
    my $exptype = shift;
    my $obsmode = shift;
    my $object  = shift;
    my $comment = shift;
    my $dbname  = shift;

    # The strings come pre-quoted.
    $exp_name =~ s/\'//g;
    $dateobs =~ s/\'//g;
    $exptype =~ s/\'//g;
    $obsmode =~ s/\'//g;
    $object =~ s/\'//g;
    $comment =~ s/\'//g;

    print "Inside decision engine\n";
    my ($label,$data_group,$dist_group,$end_stage,$tess_id,$chip_workdir,$reduction);
    my $target;

    if ($exp_name =~ /^c/) {
	print "Skipping because this is a camera commanded exposure: $exp_name\n";
	return(undef,undef,undef,"reg",undef,undef,undef);
    }
    
    if ($exptype ne 'OBJECT') {
	print "Skipping because exptype doesn't claim to be OBJECT: $exptype\n";
	return(undef,undef,undef,"reg",undef,undef,undef);
    }
    # Grab the configuration data.
    my %nightlyscience_config = ();
    my $conf_cmd = "$ppConfigDump -dump-recipe NIGHTLY_SCIENCE -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $conf_cmd, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform ppConfigDump: $error_code", $date, $PS_EXIT_SYS_ERROR);
    }
    print "data loaded\n";
    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $metadata = $mdcParser->parse(join "", @$stdout_buf);
    foreach my $entry (@{ $metadata }) {
	if (${ $entry }{name} eq 'TARGETS') {
	    my @target_data = @{ ${ $entry }{value} };
	    my $this_target = '';
	    foreach my $tentry (@target_data) {
		if (${ $tentry }{name} eq 'NAME') {
		    $this_target = ${ $tentry }{value};
		    $nightlyscience_config{$this_target}{DISTRIBUTION} = $this_target;
		}
		elsif (${ $tentry }{name} eq 'DISTRIBUTION') {
		    $nightlyscience_config{$this_target}{DISTRIBUTION} = ${ $tentry }{value};
		}
		elsif (${ $tentry }{name} eq 'TESS') {
		    $nightlyscience_config{$this_target}{TESS_ID} = ${ $tentry }{value};
		}
		elsif (${ $tentry }{name} eq 'OBSMODE') {
		    $nightlyscience_config{$this_target}{OBSMODE} = ${ $tentry }{value};
		    $nightlyscience_config{$this_target}{OBSMODE} =~ s/%/\.\*/g;
		}
		elsif (${ $tentry }{name} eq 'OBJECT') {
		    $nightlyscience_config{$this_target}{OBJECT} = ${ $tentry }{value};
		    $nightlyscience_config{$this_target}{OBJECT} =~ s/%/\.\*/g;
		}
		elsif (${ $tentry }{name} eq 'COMMENT') {
		    $nightlyscience_config{$this_target}{COMMENT} = ${ $tentry }{value};
		    $nightlyscience_config{$this_target}{COMMENT} =~ s/%/\.\*/g;
		}
		elsif (${ $tentry }{name} eq 'STACKABLE') {
		    $nightlyscience_config{$this_target}{STACKABLE} = ${ $tentry }{value};
		}
		elsif (${ $tentry }{name} eq 'DIFFABLE') {
		    $nightlyscience_config{$this_target}{DIFFABLE} = ${ $tentry }{value};
		}
		elsif (${ $tentry }{name} eq 'REDUCTION') {
		    $nightlyscience_config{$this_target}{REDUCTION} = ${ $tentry }{value};
		}
	    }
	}
    }
    print "data parsed\n";
    my $found_target;
    foreach $target (keys %nightlyscience_config) {
#	print "TEST: $target ($obsmode $object $comment) ($nightlyscience_config{$target}{OBSMODE} $nightlyscience_config{$target}{OBJECT} $nightlyscience_config{$target}{COMMENT}\t";
	my $match = 0;
	my $possible = 0;
	if (exists($nightlyscience_config{$target}{OBSMODE})) {
	    $possible++;
	    if ($obsmode =~ /^$nightlyscience_config{$target}{OBSMODE}$/) {
		$match++;
	    }
	}
	if (exists($nightlyscience_config{$target}{OBJECT})) {
	    $possible++;
	    if ($object =~ /^$nightlyscience_config{$target}{OBJECT}$/) {
		$match++;
	    }
	}
	if (exists($nightlyscience_config{$target}{COMMENT})) {
	    $possible++;
	    if ($comment =~ /^$nightlyscience_config{$target}{COMMENT}$/) {
		$match++;
	    }
	}
	print "$possible $match\n";
	if (($possible > 0)&&($match == $possible)) {
	    $found_target = $target;
	    last;
	}
    }
    unless ($found_target) {
	print "No acceptible target found.\n";
	return(undef,undef,undef,"reg",undef,undef,undef);
    }
    print "Found this target: $found_target\n";
    print "$dateobs\n";
    $target = $found_target;
    
    my $date  = $dateobs;
    $date =~ s/T.*//;
    my $workdir_date = $date;
    $date =~ s/-//g;
    $workdir_date =~ s%-%/%g;
    print "$dateobs $date $workdir_date\n";    
    $label = "${target}.nightlyscience";
    $data_group = "${target}.${date}";
    $dist_group = $nightlyscience_config{$target}{DISTRIBUTION};
    $end_stage  = 'warp';
    $tess_id    = $nightlyscience_config{$target}{TESS_ID};
    $chip_workdir = "neb://\@HOST\@.0/${dbname}/${target}.nt/${workdir_date}/";
    if (exists($nightlyscience_config{$target}{REDUCTION})) {
	$reduction  = $nightlyscience_config{$target}{REDUCTION};
    }
    else {
	undef($reduction);
    }

    
    return ($label,$data_group,$dist_group,$end_stage,$tess_id,$chip_workdir,$reduction);
}
	    
	    
	    

sub cache_run
{
    my %p = @_;

    my $cmd_output = $c->get($p{command}) if $cache;
    if (defined $cmd_output) {
        return @{thaw $cmd_output};
    } else {
        my @output = run(%p);
        $c->set($p{command}, freeze \@output) if $cache;
        return @output;
    }
}

sub value_for_flag
{
    my $cmdflags = shift;
    my $flag = shift;

    my $value = 0.0;
    if ($cmdflags =~ m|$flag|) {
        ($value) = $cmdflags =~ m|$flag\s+(\S+)|;
    }
    $value;
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $exp_id = shift;
    my $exit_code = shift;

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $exp_id and not $no_update) {
        my $command = "$regtool -addprocessedexp -exp_id $exp_id -fault $exit_code";
	$command .= " -exp_tag $exp_tag";
	$command .= " -exp_name UNKNOWN";
	$command .= " -inst UNKNOWN";
	$command .= " -telescope UNKNOWN";
	$command .= " -telescope UNKNOWN";
	$command .= " -filelevel UNKNOWN";
        $command .= " -hostname $host" if defined $host;
        $command .= " -dbname $dbname" if defined $dbname;
        system($command);
    }
    exit $exit_code;
}

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}
