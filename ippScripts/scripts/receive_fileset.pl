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

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use File::Temp qw( tempfile );
use Carp;

# Look for programs we need
my $missing_tools;
my $receivetool = can_run('receivetool') or (warn "Can't find receivetool" and $missing_tools = 1);
my $dsproductls = can_run('dsfilesetls') or (warn "Can't find dsfilesetls" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ( $fileset_id, $source, $product, $fileset, $dbname, $verbose, $no_update, $save_temps );

GetOptions(
           'fileset_id=s'      => \$fileset_id, # Fileset identifier
           'source=s'          => \$source, # Source for data
           'product=s'         => \$product, # Product for data
           'fileset=s'         => \$fileset, # Fileset for data
           'dbname=s'          => \$dbname,    # Database name
           'verbose'           => \$verbose,   # Print to stdout
           'no-update'         => \$no_update, # Don't update the database?
           'save-temps'        => \$save_temps, # keep temp files
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --fileset_id --source --product --fileset",
           -exitval => $PS_EXIT_CONFIG_ERROR) unless
    defined $fileset_id and
    defined $source and
    defined $product and
    defined $fileset;

# Get list of files, sanity check and then write it to a temporary file
my $numFiles = 0;
my ($listFile, $listName) = tempfile("/tmp/$product.$fileset.list.XXXX", UNLINK => !$save_temps);
{
    my $uri = "$source/$product/$fileset"; # URI for datastore fileset
    $uri .= "/index.txt" unless $uri =~ m|/index.txt$|;
    my $command = "dsfilesetls --uri $uri"; # Command to execute

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die( "Unable to get fileset listing from $uri\n", $fileset_id, $error_code) unless $success;

    # Get files
    my @lines = split(/\n/, join "", @$stdout_buf); # Lines from output
    foreach my $line ( @lines ) {
        $line =~ s/\#.*$//;
        next unless $line =~ /\S+/;
        my @fields = split(/\s+/, $line); # Fields in line
        &my_die( "too few columns in fileset list entry: $line\n", $fileset_id, $PS_EXIT_DATA_ERROR) unless scalar @fields >= 6;

        print $listFile "FILE$numFiles\tMETADATA\n";

        print $listFile "\turi\t\tSTR\t" . $fields[0] .  "\n";
        print $listFile "\tfile\t\tSTR\t" . $fields[1] .  "\n";
        print $listFile "\tbytes\t\tS64\t" . $fields[2] .  "\n";
        print $listFile "\tmd5sum\t\tSTR\t" . $fields[3] .  "\n";
        print $listFile "\tfile_type\tSTR\t" . $fields[4] .  "\n";
        print $listFile "\tcomponent\tSTR\t" . $fields[5] .  "\n";

        print $listFile "END\n";
        $numFiles++;
    }
    close $listFile;
}

# Add files
my $new_state;
if ($numFiles > 0) {
    $new_state = "listed";
    my $command = "receivetool -addfile"; # Command to execute
    $command .= " -fileset_id $fileset_id";
    $command .= " -file_list $listName";
    $command .= " -dbname $dbname" if defined $dbname;

    unless (defined $no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die( "Unable to add file list for fileset $fileset\n", $fileset_id, $error_code) unless $success;
    }
} else {
    print STDERR "fileset $fileset has no files\n" if $verbose;
    $new_state = "full";
}

{
    my $command = "receivetool -updatefileset"; # Command to execute
    $command .= " -fileset_id $fileset_id";
    $command .= " -set_state $new_state";
    $command .= " -dbname $dbname" if defined $dbname;

    unless (defined $no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die( "Unable to set state to $new_state for fileset $fileset\n", $fileset_id, $error_code) unless $success;
    }
}

sub my_die {
    my $msg = shift;            # Exit message
    my $fileset_id = shift;     # File identifier
    my $fault = shift;          # Fault code

    $fault = $PS_EXIT_PROG_ERROR unless defined $fault;

    carp($msg);
    if (not $no_update) {
        my $command = "$receivetool -updatefileset";
        $command .= " -fileset_id $fileset_id";
        $command .= " -fault $fault";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        die "Unable to set fault to $fault for fileset $fileset\n" unless $success;
    }
    exit $fault;
}
__END__
