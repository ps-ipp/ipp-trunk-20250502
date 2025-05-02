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

# Look for programs we need
my $missing_tools;
my $receivetool = can_run('receivetool') or (warn "Can't find receivetool" and $missing_tools = 1);
my $dsproductls = can_run('dsproductls') or (warn "Can't find dsproductls" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ( $source_id, $source, $product, $last_fileset, $dbname, $verbose, $no_update );

GetOptions(
           'source_id=s'       => \$source_id, # Source identifier
           'source=s'          => \$source, # Source for data
           'product=s'         => \$product, # Product for data
           'last_fileset=s'    => \$last_fileset, # Last fileset seen
           'dbname=s'          => \$dbname,    # Database name
           'verbose'           => \$verbose,   # Print to stdout
           'no-update'         => \$no_update, # Don't update the database?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --source_id --source --product",
           -exitval => $PS_EXIT_CONFIG_ERROR) unless
    defined $source_id and
    defined $source and
    defined $product;

# Get list of filesets
my $uri = "$source/$product"; # URI for datastore product
$uri .= "/index.txt" unless $uri =~ m|/index.txt$|;
my $command = "dsproductls --uri $uri"; # Command to execute
$command .= " --last_fileset $last_fileset" if defined $last_fileset;

my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $command, verbose => $verbose);
die "Unable to get source listing from $source for $product\n" unless $success;

# Parse list of filesets
my @filesets = ();              # Filesets to add
my @lines = split(/\n/, join "", @$stdout_buf); # Lines from output
my $numFilesets = 0;
foreach my $line ( @lines ) {
    $line =~ s/\#.*//g;
    next unless $line =~ /\S+/;
    $line =~ s/^\s+//;
    my @fields = split(/\s+/, $line); # Fields in line
    my $fileset = $fields[1];
    push @filesets, $fileset if defined $fileset and $fileset =~ /\S+/;

    # TODO don't overload recevietool.  Tune this limit
    $numFilesets++;
    last if $numFilesets >= 100;
}

if (scalar @filesets > 0) {
    # Add filesets
    {
        my $command = "receivetool -addfileset"; # Command to execute
        $command .= " -src_id $source_id";
        $command .= " -fileset " . join(' -fileset ', @filesets);
        $command .= " -dbname $dbname" if defined $dbname;

        unless (defined $no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            die "Unable to add filesets from $source for $product\n" unless $success;
        }
    }

    # Update the last fileset seen
    {
        my $command = "receivetool -updatelast"; # Command to execute
        $command .= " -src_id $source_id";
        my $last = pop @filesets; # Last fileset
        $command .= " -fileset $last";
        $command .= " -dbname $dbname" if defined $dbname;

        unless (defined $no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            die "Unable to update last fileset to $last\n" unless $success;
        }
    }
}


__END__
