#!/usr/bin/env perl

# ipp_image_path.pl print out the unix path name(s) for rawImage files for an exposure
#

use warnings;
use strict;

# get images that are on ipp008 from alternative location until the
# database is updated
my $use_008_workaround  = 0;
my $use_017_workaround  = 0;

use vars qw( $VERSION );
$VERSION = '0.01';

use DBI;
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Config 1.01 qw( :standard );
use Nebulous::Client;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ($exp_name, $class_id, $from_registered, $dbname, $verbose, $alternate);

GetOptions(
    'exp_name|e=s'  => \$exp_name,
    'class_id|c=s'  => \$class_id,
    'registered|r'  => \$from_registered,
    'dbname|d=s'    => \$dbname, # Database name    
    'verbose'       => \$verbose,   # Print to stdout
    'alternate'     => \$alternate,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --exp_name",
	   -exitval => 3) unless
    defined $exp_name;

my $ota_num;
if ($class_id) {
    $class_id = lc($class_id);
    my $extra;
    (undef, $ota_num, $extra) = $class_id =~ m/(\D*)(\d\d)(.*)/;
    if ($extra || !$ota_num) {
        print STDERR "$class_id is not a valid class_id\n";
        exit 1;
    }
}

# Look for commands we need
my $missing_tools;
my $neb_locate = can_run('neb-locate')
    or (warn "can't find neb-locate" and $missing_tools = 1);

if ($missing_tools) { 
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR); 
}

my $ipprc = PS::IPP::Config->new();
my $dbh = getDBHandle();

# Get the list of imfiles
{
    # XXX: If there are multiple exposures with the same exposure name
    # this query will return them all
    my $query;
    if ($from_registered) {
        $query = "SELECT exp_id, class_id, uri FROM rawImfile"
                . " WHERE exp_name = \'$exp_name\'";
        $query .= " AND class_id = \'xy$ota_num\'" if ($ota_num);
    } else {
        $query = "SELECT exp_id FROM newExp WHERE tmp_exp_name = ?";
        my $stmt = $dbh->prepare($query);
        $stmt->execute($exp_name);
        my $exp_ref = $stmt->fetchrow_hashref();
        if (!$exp_ref) {
            print STDERR "exposure $exp_name not found\n";
            exit 1;
        }
        $stmt->finish();
        $query = "SELECT exp_id, tmp_class_id, uri FROM newImfile WHERE exp_id = $exp_ref->{exp_id}";
        $query .= " AND tmp_class_id = \'ota$ota_num\'" if ($ota_num);
    }

    my $stmt = $dbh->prepare($query);
    $stmt->execute();
    my $nfiles = 0;
    while (my $ref = $stmt->fetchrow_hashref()) {
        my $uri = $ref->{uri};
        my $path;
        my ($scheme) = $uri =~/^(path|neb|file):/;

        if (!$scheme) {
            $path =  $uri;
        } else {
            if ($use_008_workaround && ($uri =~ /ipp008/)) {
                $path = resolve_ipp008_file($uri);
            } elsif ($use_017_workaround && ($uri =~ /4683/) &&
		     ($uri =~ /ipp017/)) {
                $path = resolve_ipp017_file($uri);
            } elsif ($alternate) {
		my $neb = $ipprc->nebulous();
		my $uris = $neb->find_instances($uri,'any');
		my @files = map {URI->new($_)->file if $_} @$uris;

		if ($#files > 0) {
		    $path = $files[1];
		}
		else {
		    $path = $files[0];
		}

	    }
	    else {
                $path = $ipprc->file_resolve($uri);
            }
        }
        if ($path) {
            # remove the leading "file://" if present
            $path =~ s/^file\:\/\///;
            $nfiles++;
            print "$path\n";
        }
    }
    if (!$nfiles) {
        my $cstr = defined($class_id) ? $class_id . " " : "";
        print STDERR "exposure $exp_name " . $cstr . "not found\n";
        exit 2;
    }
}

sub getDBHandle {
    my $dbserver = metadataLookupStr($ipprc->{_siteConfig}, "DBSERVER");
    my $dbuser = metadataLookupStr($ipprc->{_siteConfig}, "DBUSER");
    my $dbpassword = metadataLookupStr($ipprc->{_siteConfig}, "DBPASSWORD");
    if (!$dbname) {
        $dbname = metadataLookupStr($ipprc->{_siteConfig}, "DBNAME");
    }

    die "database configuration set up" unless defined($dbserver) and defined($dbuser)
        and defined($dbpassword) and defined($dbname);

    my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";

    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) 
        or die "Cannot connect to database.\n";

    return $dbh;
}

sub resolve_ipp008_file {
    my %locations = ( "ota24" => "ipp006",
                      "ota25" => "ipp009",
                      "ota26" => "ipp011",
                      "ota27" => "ipp019",
                      "ota30" => "ipp020",
                      "ota31" => "ipp021"
                    );

    my $uri = shift;

    # uri's look like neb://ipp008.0/gpc1/20080625/o4642g0400o/o4642g0400o.ota24.fits
    
    my ($left, $middle, $ota, $fits) = split '\.', $uri;
    
    my $node = $locations{$ota};

    die "can't find node for $ota: $uri" if (!$node);

    my $path =  $uri;
    
    $path =~  s%neb://ipp008.0%/data/${node}.0/recover08%;

    die "sorry backup image for $uri not found\n" if (!-e $path);

    return $path;
}

sub resolve_ipp017_file {
    my $uri = shift;

    # uri's look like neb://ipp008.0/gpc1/20080625/o4642g0400o/o4642g0400o.ota24.fits
    
    my ($left, $middle, $ota, $fits) = split '\.', $uri;
    
    my $node = "ipp015";

    die "can't find node for $ota: $uri" if (!$node);

    my $path =  $uri;
    
    $path =~  s%neb://ipp017.0%/data/${node}.0/recover17%;

    die "sorry backup image for $uri not found\n" if (!-e $path);

    return $path;
}
