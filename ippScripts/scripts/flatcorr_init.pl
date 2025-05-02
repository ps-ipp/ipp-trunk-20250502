#!/usr/bin/env perl

## USAGE:flatcorr_init.pl
## given dbname, dvodb, filter, time range, camera, telescope, etc?
## select matching images and register as a new flatcorr run

## should this be implemented as a ippTool program?
## this is a lot like dettool -definebyquery
## flatcorr -definebyquery
## this should queue the selected images with a mode that stops at camera
## ?? how do we block this batch from running through warp ??

## we also need the test / query tool:
## flatcorr -pendingcorr
## examine the chip & camera stages and check for all images to have completed (successfully or not)

## allow a certain fraction of failures...

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

use Storable qw(freeze thaw);
use File::Basename qw( basename);
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration
use File::Spec;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ($dvo_id, $catdir, $region, $dbname, $workdir, $no_update, $no_op);
GetOptions(
    'dvo_id|i=s'       => \$dvo_id,
    'catdir|c=s'       => \$catdir,
    'region|r=s'       => \$region,
    'dbname|d=s'       => \$dbname,# Database name
    'workdir|w=s'      => \$workdir, # Working directory for output files
    'no-update'        => \$no_update,
    'no-op'            => \$no_op,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dvo_id --catdir --region",
           -exitval => 3) unless
    defined $dvo_id and
    defined $catdir and
    defined $region;

# Look for programs we need
my $missing_tools;
my $addstar  = can_run('addstar')  or (warn "Can't find addstar"  and $missing_tools = 1);
my $relphot  = can_run('relphot')  or (warn "Can't find relphot"  and $missing_tools = 1);
my $uniphot  = can_run('uniphot')  or (warn "Can't find uniphot"  and $missing_tools = 1);
my $relastro = can_run('relastro') or (warn "Can't find relastro" and $missing_tools = 1);
my $caltool  = can_run('caltool')  or (warn "Can't find caltool"  and $missing_tools = 1);

if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR);
}

# select the primary filters from DVO query?
my (@filters) = `photcodeList -average`;

# parse the region (RAs,RAe:DECs,DECe) : item = +/-NNN.NNNN
my @coords = split (":", $region);
my ($RAs, $RAe) = split (",", $coords[0]);
my ($DECs, $DECe) = split (",", $coords[1]);

# Run addstar -resort
{
    my $command = "$addstar -resort";
    $command .= "-D CATDIR $catdir";
    $command .= "-region $RAs $RAe $DECs $DECe";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        cache_run(command => $command, verbose => 1);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die ("Unable to perform addstar -resort on region $region: $error_code", $dvo_id, $region, "RESORT", $status, $dbname);
    }
}

# Run relphot (filter) for each filter
{
    foreach my $filter (@filters) {
        my $command = "$relphot $filter";
        $command .= "-D CATDIR $catdir";
        $command .= "-region $RAs $RAe $DECs $DECe";

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            cache_run(command => $command, verbose => 1);

        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die ("Unable to perform addstar -resort on region $region: $error_code", $dvo_id, $region, "RELPHOT", $status, $dbname);
        }
    }
}

# Run uniphot (filter) for each filter
# XXX skip this one?  run less frequently?
if (0) {
    foreach my $filter (@filters) {
        my $command = "$uniphot $filter";
        $command .= "-D CATDIR $catdir";
        $command .= "-region $RAs $RAe $DECs $DECe";

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            cache_run(command => $command, verbose => 1);

        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die ("Unable to perform addstar -resort on region $region: $error_code", $dvo_id, $region, "UNIPHOT", $status, $dbname);
        }
    }
}

{
    my $command = "$relastro -objects";
    $command .= "-D CATDIR $catdir";
    $command .= "-region $RAs $RAe $DECs $DECe";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        cache_run(command => $command, verbose => 1);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die ("Unable to perform addstar -resort on region $region: $error_code", $dvo_id, $region, "RELASTRO.OBJECTS", $status, $dbname);
    }
}

{
    my $command = "$relastro -images";
    $command .= "-D CATDIR $catdir";
    $command .= "-region $RAs $RAe $DECs $DECe";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        cache_run(command => $command, verbose => 1);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die ("Unable to perform addstar -resort on region $region: $error_code", $dvo_id, $region, "RELASTRO.IMAGES", $status, $dbname);
    }
}

my $command = "$caltool -addcalrun";
$command .= " -dvo_id $dvo_id";
$command .= " -region $region";
$command .= " -last_step RELASTRO.IMAGES";
$command .= " -status SUCCESS";
$command .= " -dbname $dbname" if defined $dbname;

# Push the results into the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 1);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn ("Unable to perform regtool -addprocessedimfile: $error_code");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $dvo_id    = shift;
    my $region    = shift;
    my $last_step = shift;
    my $status    = shift;
    my $dbname    = shift;

    carp($msg);
    if (defined $dvo_id && defined $region && defined $last_step && defined $status and not $no_update) {
        my $command = "$caltool -addcalrun";
        $command .= " -dvo_id $dvo_id";
        $command .= " -region $region";
        $command .= " -last_step $last_step";
        $command .= " -status $status";
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
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
