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
use File::Basename qw( basename dirname );

use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


# Look for programs we need
my $missing_tools;
my $dsreg   = can_run('dsreg') or (warn "Can't find dsreg" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($status_fs_name, $status_product, $received_fs_name, $fault, $status_file);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);

GetOptions(
           'status_fs_name=s'   => \$status_fs_name,
           'status_product=s'   => \$status_product,
           'received_fs_name=s' => \$received_fs_name,
           'fault=i'            => \$fault,
           'status_file=s'      => \$status_file,
           'dbname=s'           => \$dbname,     # Database name
           'save-temps'         => \$save_temps, # Save temporary files?
           'verbose'            => \$verbose,    # Print stuff?
           'logfile=s'          => \$logfile,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --status_fs_name --status_product",
           -exitval => 3) unless
    defined $received_fs_name and
    defined $status_product and
    defined $fault;
#    and ($fault == 0 or defined $status_file);

# if a name for the status fileset is not provided use the received value
$status_fs_name = $received_fs_name if !defined $status_fs_name;

$ipprc->redirect_output($logfile) if $logfile;

{
    # if a status file is provided put it in the fileset otherwise make an empty one
    # The status information in the produce list is enough for the server
    my $regline = "";
    my $fileargs = " --empty";

    if ($status_file) {
        $regline = "$status_file'|||text|'";

        $fileargs = " --list - --copy --abspath";
    }

    # XXX need to create a fileset type for this
    my $command = "echo $regline | dsreg --add $status_fs_name --product $status_product --type notset";
    $command .= " $fileargs --ps0 $received_fs_name --ps1 $fault";
    $command .= " --dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_CONFIG_ERROR);
        &my_die("Unable to perform $command error_code: $error_code", $status_fs_name, $error_code);
    }
}

exit 0;


sub my_die {
    my $msg = shift;
    my $fs_name = shift;
    my $fault = shift;

    die "$msg fs_name: $fs_name: $fault";

    exit $fault;
}

__END__
