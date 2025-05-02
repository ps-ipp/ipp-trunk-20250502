#!/bin/env perl

# pstamp_finish.pl

use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

use Sys::Hostname;
use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Copy;

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw($PS_EXIT_SUCCESS
		       $PS_EXIT_UNKNOWN_ERROR
		       $PS_EXIT_SYS_ERROR
		       $PS_EXIT_CONFIG_ERROR
		       $PS_EXIT_PROG_ERROR
		       $PS_EXIT_DATA_ERROR
		       $PS_EXIT_TIMEOUT_ERROR
		       metadataLookupStr
		       metadataLookupBool
		       caturi
		       );


my ( $req_id, $dbname, $dbserver, $verbose, $save_temps );

GetOptions(
	   'req_id=s'   => \$req_id,
	   'dbname=s'   => \$dbname,
	   'dbserver=s' => \$dbserver,
	   'verbose'    => \$verbose,
	   'save-temps' => \$save_temps,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

pod2usage ( -msg => "--req_id is required", -exitval => 2 ) if !$req_id;

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $dsreg  = can_run('dsreg')  or (warn "Can't find dsreg"  and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new(); # IPP Configuration

my $outputDataStoreRoot = metadataLookupStr($ipprc->{_siteConfig}, 'DATA_STORE_ROOT');
exit ($PS_EXIT_CONFIG_ERROR) unless defined $outputDataStoreRoot; # lookup failure outputs a message

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $req;
{
    my $command = "$pstamptool -listreq -req_id $req_id";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
    my $output = join "", @$stdout_buf;
    if (!$output) {
        if ($verbose) {
            print STDERR "no requests found\n"
        }
        exit 0;
    }
    my $metadata = $mdcParser->parse($output) or die("Unable to parse metdata config doc");

    my $requests = parse_md_list($metadata);

    $req = $requests->[0];

    die "request $req_id not found in metadata config doc" if !$req;
}

my $product = $req->{outProduct};

my $req_name = $req->{name};

# set the output fileset's name to the request name.
my $fileset = $req_name;

print STDERR "product: $product  REQ_NAME: $req_name\n" if $verbose;

if ($product and $fileset) {
    my $command = "$dsreg --del $fileset --product $product --rm --force";
#   don't add dbname let dsreg get it from the DS_DBNAME in site.config
#    $command .= " --dbname $dbname" if $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die "Unable to perform $command error code: $error_code";
    }
}


{
    my $command = "$pstamptool -revertreq -req_id $req_id";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
}
