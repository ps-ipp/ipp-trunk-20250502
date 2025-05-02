#! /usr/bin/env perl

use strict;
use warnings;

use Carp;
use IPC::Cmd 0.36 qw( can_run run );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );


my $missing_tools;
my $regtool = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $burntool = can_run('ipp_apply_burntool_single.pl') or (warn "Can't find ipp_burntool_apply_single.pl" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $save;
my $exp_name;
my $class_id;
my $dbname = 'gpc1';
my $camera = 'GPC1';
my $verbose = 0;
GetOptions(
    'save=s'          => \$save,
    'exp_name|e=s'    => \$exp_name,
    'class_id|c=s'  => \$class_id,
    'verbose|v'     => \$verbose,
    'dbname=s'      => \$dbname,
) or pod2usage( 2 );

die "usage: $0 --exp_name (exp_name) --class_id (class_id)\n" unless $class_id and $exp_name;

# Set up the commands we'll use:
$regtool  .= " -dbname $dbname -exp_name $exp_name -class_id $class_id";
$burntool .= " --dbname $dbname --camera $camera";

# Check database entry for this to get the exp_id and image uri
my $regData;
{
    my $command = "$regtool -processedimfile";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        carp("Unable to perform $command: $error_code");
        exit $error_code;
    }
    my $list = $mdcParser->parse(join "", @$stdout_buf) or die("Unable to parse regtool metadata");
    my $mdc = parse_md_list($list);
    $regData = $mdc->[0];
}

my $exp_id = $regData->{exp_id};
my $uri = $regData->{uri};

die "failed to find exp_name in regtool output" unless $exp_name;
die "failed to find uri in regtool output" unless $uri;

# Determine the value of a "good" burntool run.
my $config_cmd = "$ppConfigDump -camera $camera -get-key BURNTOOL.STATE.GOOD";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run ( command => $config_cmd, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    carp("Unable to perform ppConfigDUmp $config_cmd: $error_code");
    exit $error_code;
}

my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or die("Unable to parse Config metadata");

my $burntoolStateGood = 999;
foreach my $cfg (@$recipeData) {
    if ($cfg->{name} eq 'BURNTOOL.STATE.GOOD') {
        $burntoolStateGood = $cfg->{value};
    }
}
if ($burntoolStateGood == 999) {
    die("Failed to determine BURNTOOL.STATE.GOOD");
}


# Construct burntool table name
my $table;
if($burntoolStateGood == 15) {
    ($table = $uri) =~ s/\.fits/\.burn\.v15\.tbl/;	
} else {
    ($table = $uri) =~ s/\.fits/\.burn\.tbl/;		
}

print "$table\n";

# Look for the old table, and if it exists, move it out of the way and delete it so we can work from scratch.
my $table_exists;
{
    my $command = "neb-stat $table";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    if ($success) {
        $table_exists = 1;
    } else {
        $table_exists = 0;
    }
}

if ($table_exists) {
    # file exists
    my $moved = "$table.bad";
    my $command = "neb-mv $table $moved";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        carp("Unable to perform $command: $error_code");
        exit $error_code;
    }
    # don't care if this fails
    &vsystem ("neb-rm --force $moved");
}

# Calculate the previous uri for the table by decrementing the nightly image sequence
my $left = substr $exp_name, 0, 6;
my $num = substr $exp_name, 6, 4;
my $end = substr $exp_name, 10, 1;

# print "$left $num $end\n";

my $prev = $num - 1;
my $prev_name = sprintf "$left%04d$end", $prev;

my $prev_uri = $uri;
$prev_uri =~ s/$exp_name/$prev_name/g;
die "failed" unless $prev_uri;

# Run the burntool command to generate the table.
{
    my $command = "$burntool --exp_id $exp_id --class_id $class_id --this_uri $uri --previous_uri $prev_uri";
    $command .= " --verbose" if $verbose;
    print "$command\n";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 1);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        carp("Unable to perform $command: $error_code");
        exit $error_code;
    }
}

# This isn't needed, but doesn't hurt anything.
# Copy the table we just made to a local directory
if (defined $save) {
    my $nebfile = `neb-locate -p $table`; chomp $nebfile;
    vsystem ("cp $nebfile $save");
}


exit 0;

sub vsystem {
    print STDERR "@_\n";
    my $status = system ("@_");
    $status;
}
