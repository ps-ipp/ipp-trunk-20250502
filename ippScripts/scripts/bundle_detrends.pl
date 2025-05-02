#! /usr/bin/env perl

use Carp;

use warnings;
use strict;
use DBI;
use IPC::Cmd 0.36 qw( can_run run);
use File::Temp qw( tempfile );
use PS::IPP::Config 1.01 qw( :standard );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


my $missing_tools = 0;
my $tar = can_run('tar') or (warn "Can't find tar" and $missing_tools = 1);

my $db;

my ($det_id, $outfile, $dbname, $verbose, $save_temps);

GetOptions(
    'det_id=s'        => \$det_id,
    'outfile=s'       => \$outfile,
    'dbname=s'        => \$outfile,
    'verbose'         => \$verbose,
    'save_temps'      => \$save_temps,
    ) or pod2usage ( 2 );
pod2usage( -msg =>
"USAGE: bundle_detrends.pl <options...>
        Options:
           --det_id <det_id>      det_id to bundle
           --outfile <out.tgz>    specify and output name
           --dbname <db>          Default gpc1.
           --verbose\n",
	   -exitval => 2, ) if @ARGV;

unless(defined($dbname)) {
    $dbname = 'gpc1';
}
unless(defined($outfile)) {
    $outfile = "./detrend_${det_id}.tgz";
}
my $ipprc = PS::IPP::Config->new(  ) or my_die( "Unable to set up", $det_id, $outfile);

# Create database information
$db = init_gpc_db();

my ($dbFile, $dbName) = tempfile("/tmp/bundle_detrend.tmp.XXXX", UNLINK => !$save_temps);
my @tables = ('detInputExp','detNormalizedExp','detNormalizedImfile','detNormalizedStatImfile','detProcessedExp',
	      'detProcessedImfile','detRegisteredImfile','detResidExp','detResidImfile','detRun','detRunSummary',
	      'detStackedImfile');

foreach my $t (@tables) {
    print $dbFile "-- $t\n";
#    print STDERR "-- $t\n";
    my $sth = "select * from $t WHERE det_id = $det_id";
#    my $dr  = $db->selectall_arrayref( $sth );
    my $prep = $db->prepare($sth);

    my $ex   = $prep->execute() or die "Execute execption: $DBI::errstr";
    my $colstring = join ',', @{ $prep->{NAME} };
    my @types = @{ $prep->{TYPE} };
    my $dr   = $prep->fetchall_arrayref();
    foreach my $rr (@{ $dr }) {
	for (my $i = 0; $i <= $#{ $rr }; $i++) {
	    ${ $rr }[$i] = $db->quote(${ $rr }[$i], $types[$i]);
	}
	my $valstring = join ',', @{ $rr };
	print $dbFile "INSERT INTO $t ($colstring) VALUES($valstring);\n";
#	print STDERR  "INSERT INTO $t ($colstring) VALUES($valstring);\n";
    }
    print $dbFile "\n";
    $prep->finish();
}
    
# Create tar file

# Identify files
my @neb_files = ();
# Do we have any registered imfiles for this det_id?
if ($#neb_files == -1) {
    my $sth = "select uri from detRegisteredImfile where det_id = $det_id";
    my $dr = $db->selectall_arrayref( $sth );
    foreach my $rr (@{ $dr }) {
	push @neb_files, @{ $rr };
    }
}
# Try the normalized imfiles
if ($#neb_files == -1) {
    my $sth = "select uri from detNormalizedImfile where det_id = $det_id";
    my $dr = $db->selectall_arrayref( $sth );
    foreach my $rr (@{ $dr }) {
	push @neb_files, @{ $rr };
    }
}    
# Try the stacked imfiles if we have to
if ($#neb_files == -1) {
    my $sth = "select uri from detStackedImfile where det_id = $det_id";
    my $dr = $db->selectall_arrayref( $sth );
    foreach my $rr (@{ $dr }) {
	push @neb_files, @{ $rr };
    }
}    

my @disk_files = map { $_ = $ipprc->file_resolve( $_ ) } @neb_files;

my $cmd_files = join ' ', @disk_files;
my $tar_cmd = "tar -zvcf $outfile --transform='s/\.tmp\...../.sql/;s/tmp/./;s/.*://;' --strip-components=5 --show-transformed-names $dbName $cmd_files";

print "$tar_cmd\n";
my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run ( command => $tar_cmd, verbose => $verbose);
unless ($success) {
    &my_die("Unable to perform tar: $error_code", $det_id, $outfile);
}

# Utilities

sub my_die {
    my $message = shift;
    my $det_id = shift;
    my $out   = shift;

    carp($message);
    exit(1);
}

sub init_gpc_db {
    ## change to use the site.config setting now, however may want to use replicated scidbs instead
    my $siteConfig = $ipprc->{_siteConfig};
    use constant DB_SOCKET => '/var/run/mysqld/mysqld.sock';
    # my $dbserver = 'ippdb01';
    my $dbuser = 'ippuser';
    my $dbpass = 'ippuser';
    my $dbserver = metadataLookupStr($siteConfig, 'DBSERVER');
    die "database configuration set up" unless defined($dbserver);
    $db = DBI->connect("DBI:mysql:database=${dbname};host=${dbserver};" .
                       "mysql_socket=" . DB_SOCKET(),
                       ${dbuser},${dbpass},
		       { RaiseError => 1, AutoCommit => 1}
        ) or die "Unable to connect to database $DBI::errstr\n";
    return($db);
}


