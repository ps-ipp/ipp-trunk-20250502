#!/bin/env perl

# request_finish.pl

use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

use Sys::Hostname;
use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Copy;
use File::Basename qw( dirname );

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw( :standard );
use PS::IPP::PStamp::RequestFile qw( :standard );

my ( $req_id, $req_name, $req_file, $req_type, $outdir, $product, $dbname, $dbserver, $verbose, $save_temps, $redirect_output );

my $ipprc = PS::IPP::Config->new();

GetOptions(
           'req_id=s'   => \$req_id,
           'req_name=s' => \$req_name,
           'req_file=s' => \$req_file,
           'req_type=s' => \$req_type,
           'outdir=s'  => \$outdir,
           'product=s'  => \$product,
	   'dbname=s'   => \$dbname,
	   'dbserver=s' => \$dbserver,
	   'verbose'    => \$verbose,
	   'save-temps' => \$save_temps,
           'redirect-output' => \$redirect_output,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

my $err = "";

$err .= "--req_id is required\n" if !$req_id;
$err .= "--req_type is required\n" if !$req_type;
$err .= "--req_file is required\n" if !$req_file;
$err .= "--req_name is required\n" if !$req_name;
$err .= "--product is required\n" if !$product;
# $err .= "--outdir is required\n" if !$outdir;

die "$err" if $err;


if (!$outdir) {
    $outdir = dirname($req_file);
}

if ($redirect_output) {
    my $logDest = "$outdir/reqfinish.$req_id.log";
    $ipprc->redirect_output($logDest);
}

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $pstamp_finish  = can_run('pstamp_finish.pl')  or (warn "Can't find pstamp_finish.pl"  and $missing_tools = 1);
my $dquery_finish  = can_run('dquery_finish.pl')  or (warn "Can't find dquery_finish.pl"  and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $finish_cmd;
if ($req_type eq 'pstamp') {
    $finish_cmd = $pstamp_finish;
} elsif ($req_type eq 'dquery') {
    $finish_cmd = $dquery_finish;
}
if ($finish_cmd) {
    my $command = $finish_cmd . " --req_id $req_id --req_name $req_name --req_file $req_file --product $product --outdir $outdir";
    $command   .= " --dbname $dbname" if $dbname;
    $command   .= " --dbserver $dbserver" if $dbserver;
    $command   .= " --verbose" if $verbose;
    $command   .= " --save-temps" if $save_temps;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
} else {
    # Unknown request type. Stop job with fault invalid request.
    print STDERR "request  $req_id has unknown reqType $req_type\n" if $verbose;

    my $command = "$pstamptool -updatereq -req_id $req_id -set_state stop -set_fault $PSTAMP_INVALID_REQUEST";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
}
