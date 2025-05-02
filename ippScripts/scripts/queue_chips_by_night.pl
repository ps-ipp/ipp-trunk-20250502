#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $stdate = `date`;
use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );
use File::Basename;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

my ( $date, $data_group_base, $label, $tess_id, $end_stage, $workdir_base, $dbname );

GetOptions(
    'date=s'             => \$date,       # date, like this 2011-02-12
    'data_group_base=s'  => \$data_group_base,    # data_group_base, like this: isp
    'label=s'            => \$label,   # label
    'dbname|d=s'         => \$dbname,    # Database name
    'tess_id=s'          => \$tess_id, # tess_id
    'end_stage=s'        => \$end_stage, # end_stage
    'workdir_base=s'          => \$workdir_base,  # workdir
     ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dbname --label --tess_id --date \n Optional: --data_group_base, end_stage, workdir_base",
           -exitval => 3) unless
    defined $dbname and
    defined $label and
    defined $tess_id and
    defined $date;

my $truncdate = $date;
    $truncdate =~ s/-//g;
if (defined $data_group_base) {
    print "found data_group_base = $data_group_base\n";
} else {
    $data_group_base = $label;
    print "using $label as data_group_base\n";
}
if (defined $end_stage) {
    print "found end_stage = $end_stage\n";
} else {
    $end_stage = "camera";
    print "using camera as end_stage\n";
}
if (defined $workdir_base) {
    print "found workdir_base = $workdir_base\n";
} else {
    $workdir_base = "neb://\@HOST\@.0/$dbname/$label";
    print "using $workdir_base as workdir\n";
}

# we have all the pieces, now build up the chiptool:

my $command = "$chiptool -definebyquery -unique";
$command .= " -dbname $dbname";
$command .= " -dateobs_begin '$date"."T00:00:00'";
$command .= " -dateobs_end '$date"."T23:59:59'";
$command .= " -set_label $label";
$command .= " -set_data_group $data_group_base.$truncdate";
$command .= " -set_workdir $workdir_base/$truncdate";
$command .= " -set_end_stage $end_stage";
$command .= " -set_tess_id $tess_id";

print "\nrunning this command:\n\n$command\n\n";


my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $command, verbose => 0);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    warn("Unable to perform chiptool -definebyquery: $error_code\n");
    exit($error_code);
    
}

 
