#!/usr/bin/env perl

# tool to lookup ipp database IDs from the ippadbmin database

use warnings;
use strict;

use vars qw( $VERSION );
$VERSION = '0.01';

use DBI;
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ($dbname, $list, $help);

GetOptions(
    'dbname|d=s'    => \$dbname, # Database name    
    'list'          => \$list,
    'help'          => \$help,
) or pod2usage( 2 );

pod2usage( -msg => "USAGE: ippdb.pl [--dbname dbname]", -exitval => 2 ) if defined $help;
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "either --dbname (dbname) or --list is required", -exitval => 3) unless defined $dbname or defined $list;

my $ipprc = PS::IPP::Config->new();
my $dbh = getDBHandle();

# Get the list of imfiles
{
    my $query;

    if ($list) {
	$query = "SELECT * FROM projects";
	my $stmt = $dbh->prepare($query);
	$stmt->execute();
	print STDOUT "proj_id : projname\n";
	while (my $ref = $stmt->fetchrow_hashref()) {
	    printf STDOUT "%7s : %s\n", $ref->{proj_id}, $ref->{projname};
	}
	exit 0;
    }

    $query = "SELECT proj_id FROM projects WHERE projname = \'$dbname\'";
    my $stmt = $dbh->prepare($query);
    $stmt->execute();
    my $ref = $stmt->fetchrow_hashref();
    if (!$ref) {
	print STDERR "ippdb $dbname not found\n";
	exit 1;
    }

    my $proj_id = $ref->{proj_id};
    print STDOUT "$proj_id\n";

    $stmt->finish();
    exit 0;
}

sub getDBHandle {
    my $dbserver = metadataLookupStr($ipprc->{_siteConfig}, "DBSERVER");
    my $dbuser = metadataLookupStr($ipprc->{_siteConfig}, "DBUSER");
    my $dbpassword = metadataLookupStr($ipprc->{_siteConfig}, "DBPASSWORD");
    my $admindb = "ippadmin";

    die "database configuration set up" unless defined($dbserver);
    die "database configuration set up" unless defined($dbuser);
    die "database configuration set up" unless defined($dbpassword);

    my $dsn = "DBI:mysql:host=$dbserver;database=$admindb";
    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) or die "Cannot connect to database.\n";

    return $dbh;
}
