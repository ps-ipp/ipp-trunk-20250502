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

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use Digest::MD5::File qw( file_md5_hex );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


# Look for programs we need
my $missing_tools;
my $disttool   = can_run('disttool') or (warn "Can't find disttool" and $missing_tools = 1);
my $dsproductls = can_run('dsproductls') or (warn "Can't find dsproductls" and $missing_tools = 1);
my $wget   = can_run('wget') or (warn "Can't find wget" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($dest_id, $status_uri, $last_fileset, $limit);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);

GetOptions(
           'dest_id=s'      => \$dest_id,    # destination identifier
           'status_uri=s'   => \$status_uri, # uri for the desination's status data store product
           'last_fileset=s' => \$last_fileset, # name of last fileset seen on this datastore
           'limit=i'        => \$limit,      # limit on the number of filesets to process
           'dbname=s'       => \$dbname,     # Database name
           'save-temps'     => \$save_temps, # Save temporary files?
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dest_id --status_uri",
           -exitval => 3) unless
    defined $dest_id and
    defined $status_uri;

$ipprc->redirect_output($logfile) if $logfile;

my $url = "$status_uri/index.txt";
$url .= "?$last_fileset" if defined($last_fileset) and ($last_fileset ne "NULL");

my @fileset_list;
{
    # XXX TODO: use dsproductls once it's been updated to output product type specific columns
    my $command = "$wget --no-verbose -O - $url";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command error_code: $error_code", $dest_id, $error_code);
    }

    @fileset_list = split "\n", join "", @$stdout_buf;

    my $numlines = scalar @fileset_list;
}

my $numFilesets = 0;
foreach my $line (@fileset_list) {
    # skip comment lines
    next if $line =~ /^#/;
    $numFilesets++;
    last if $limit and ($numFilesets > $limit);

    my ($status_fs_name, $time, $type, $fs_name, $fault) = split /\|/, $line;

    my $command = "$disttool -updatercrun -dest_id $dest_id -fs_name $fs_name -status_fs_name $status_fs_name -set_last_fileset $status_fs_name -fault $fault";
    if ($fault == 0) {
        $command .= " -set_state full";
    }
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command error_code: $error_code", $dest_id, $error_code);
    }
}



sub my_die {
    my $msg = shift;
    my $dest_id = shift;
    my $fault = shift;

    die "$msg dest_id: $dest_id: $fault";

    exit $fault;
}

__END__
