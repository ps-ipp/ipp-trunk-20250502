#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

use vars qw( $VERSION );
$VERSION = '1.0';

#use Date::Parse;

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $dsrootls = can_run('dsrootls') or (warn "Can't find rootls" and $missing_tools = 1);
my $cleandsproduct = can_run('cleandsproduct.pl') or 
        (warn "Can't find cleandsproduct.pl" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($product, $datastore, $verbose);

# Parse the command-line arguments
GetOptions(
           'product=s'      => \$product,      # product name
           'datastore=s'    => \$datastore,    # datastore uri
           'verbose'        => \$verbose,      # Print stuff?
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV\n", -exitval => 2 ) if @ARGV;

if (!$datastore) {
    $datastore = metadataLookupStr($ipprc->{_siteConfig}, "DATA_STORE_ROOT_URL");
    die ("Unable to find DATA_STORE_ROOT_URL in siteConfig\n") unless $datastore;
}

my @product_list;
push @product_list, $product if $product;
if (!$product) {
    my $uri = "$datastore/index.txt";

    my $command = "$dsrootls --uri $uri --timeout 150 | grep IPP-MOPS";

    # run the command. Don't set verbose here. That will just fill the log file uselessly.
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 0);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        die("Unable to perform $command: $error_code");
    }

    my @lines = split "\n", join("", @$stdout_buf);

    if ((scalar @lines) eq 1) {
        print "no products found at $uri\n";
        exit 0;
    }

    foreach my $line (@lines) {
        chomp $line;
        next if $line =~ /#/;

        my ($product_uri, $product_name, $update_time, $type, $desc) = split " ", $line;

        push @product_list, $product_name;
    }

}

foreach my $product (@product_list) {
    my $command = "$cleandsproduct --datastore $datastore --product $product";
    $command .= " --verbose" if $verbose;

    print "$command\n";
}

__END__

=pod
=head1 NAME

cleandsproduct.pl - cleans up a data store product

=head1 SYNOPSIS

cleandsproduct.pl --product <product name> [--last-fileset <filesetid>] [--no-rm] [--retention-days <days>] [--use-configured-publish-retention]

=head1 DESCRIPTION

Uses L<dsproductls> to get a listing of all of the filesets in a data store product, then the program L<dsreg> is used to delete
the filesets from the data store directory and their representation in the database.

By default, all filesets are deleted. Certain options may be used to stop the cleanup after a given fileset 
or once all remaining filesets are newer than some time.

=head1 OPTIONS

=over 4

=item --product <product name>

The name of the product to be cleaned up.

Required.

=item --retention-days <days>

The minimum number of days old for filesets to be cleaned.
Optional.

=item --use-configured-publish-retention

Get retention-days value from the SiteConfig
Optional.

=item --last_fileset <filesetid>

The FileSet ID of the last FileSet to be cleaned. Note: if the specified fileset is newer than the retention time
cleanup stops after cleaning that fileset.
Optional.

=item --no-rm

Update the database but do not remove the files on disk. The files will thus still exist, but they will be invisible
through the data store interface.

=head1 Configuration Values

The following configuration values are used to set certain default values.

=item DATA_STORE_ROOT_URL 

The http address of the data store. Required.

=item DATA_STORE_PUBLISH_RETENTION_DAYS

The number of days that filesets in the publishing data stores should be preserved. Only used and required if
the option --use-configured-publish-retention is supplied.

=back

=head1 CREDITS

Bill Sweeney IfA

=head1 SUPPORT

Please contact the author directly via e-mail.

=head1 AUTHOR

Bill Sweeney bills@ifa.hawaii.edu

=head1 COPYRIGHT

Copyright (C) 2011 - 2016  University of Hawaii IfA.


=head1 SEE ALSO

L<dsreg> L<dsproductls>

=cut
