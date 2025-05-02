#!/usr/bin/env perl

use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use Carp;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $receivetool = can_run('receivetool') or (warn "Can't find receivetool" and $missing_tools = 1);
my $receive_setstatus = can_run('receive_setstatus.pl') or (warn "Can't find receive_setstatus.pl" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ( $fileset_id, $fileset, $dbinfo_uri, $product, $ds_dbname, $ds_dbhost, $dbname, $verbose, $no_update, $save_temps );

GetOptions(
           'fileset_id=s'      => \$fileset_id,# database id for the fileset
           'fileset=s'         => \$fileset,   # fileset name
           'dbinfo_uri=s'      => \$dbinfo_uri,# uri for the database info file
           'status_product=s'  => \$product,   # Product for status update
           'ds_dbname=s'       => \$ds_dbname, # data store host
           'ds_dbhost=s'       => \$ds_dbhost, # data store dbname
           'dbname=s'          => \$dbname,    # Database name
           'verbose'           => \$verbose,   # Print to stdout
           'no-update'         => \$no_update, # Don't update the database?
           'save-temps'        => \$save_temps, # Save temporary files?
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --file_id --source --product --fileset --file --workdir",
           -exitval => $PS_EXIT_CONFIG_ERROR) unless
    defined $fileset_id;

my $ipprc = PS::IPP::Config->new() or
    &my_die( "Unable to set up", $fileset_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $import_run_to_db = 0;
if ($import_run_to_db and $dbinfo_uri and ($dbinfo_uri ne "NULL")) {
    my $filename = basename($dbinfo_uri);
    my ($stage) = $filename =~ m|^dbinfo\.(\S+)\.\d+\.mdc$|; # Stage of interest
    my $tool_name;
    my $tool_mode = '-importrun';
    if ($stage eq "raw") {
        $tool_name = "regtool";
    } elsif ($stage eq "camera") {
        $tool_name = "camtool";
    } elsif ($stage eq "chip_bg" or $stage = "warp_bg") {
        $tool_name = "bgtool";
    } elsif ($stage eq "sky") {
        $tool_name = "staticskytool";
    } elsif ($stage eq "skycal") {
        $tool_name = "staticskytool";
        $tool_mode = '-importskycal';
    } else {
        $tool_name = "${stage}tool";
    }
    my $tool = can_run("$tool_name") or &my_die("Can't find tool to load $dbinfo_uri\n", $fileset_id, $PS_EXIT_CONFIG_ERROR);

    my $file = $ipprc->file_resolve($dbinfo_uri);
    &my_die("Unable to resolve $dbinfo_uri\n", $PS_EXIT_UNKNOWN_ERROR) unless $file;

    my $command = "$tool $tool_mode -infile $file"; # Command to execute
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die("Unable to load $file\n", $fileset_id, $PS_EXIT_UNKNOWN_ERROR) unless $success;

    # XXX: once the dbinfo file is imported we cannot revert this fileset
}

if ($product and ($product ne 'NULL')) {
    my $command = "$receive_setstatus --dbname $ds_dbname --status_product $product";
    $command .= " --received_fs_name $fileset --fault 0";
    if ($ds_dbname) {
        $command .= " --dbname $ds_dbname";
    } else {
        # XXX: is falling back to the other database a good idea?
        $command .= " --dbname $dbname" if defined $dbname;
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die("Unable to set transfer status on data store for $fileset_id\n", $fileset_id, $PS_EXIT_UNKNOWN_ERROR) unless $success;
}

# update the fileset entry
# All done
{
    my $command = "$receivetool -updatefileset -fileset_id $fileset_id -set_state full";
    $command .= " -dbname $dbname" if defined $dbname;

    unless (defined $no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die("Unable to add result for $fileset_id\n", $fileset_id, $PS_EXIT_CONFIG_ERROR) unless $success;
    }
}

# Pau.


sub my_die
{
    my $msg = shift;            # Exit message
    my $fileset_id = shift;     # Fileset identifier
    my $fault = shift;          # Fault code

    $fault = $PS_EXIT_PROG_ERROR unless defined $fault;

    carp($msg);
    if (defined $fileset_id and not $no_update) {
        my $command = "$receivetool -updatefileset";
        $command .= " -fileset_id $fileset_id";
        $command .= " -fault $fault";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
    }
    exit $fault;
}


__END__
