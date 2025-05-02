#! /usr/bin/env perl

use warnings;
use strict;
use Carp;
use DBI;
use IPC::Cmd 0.36 qw( can_run run);
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


my ($dbname,$stack_id,$sass_id,$label,$camera,$filter);

my $ipprc =  PS::IPP::Config->new(); # IPP Configuration
my $siteConfig = $ipprc->{_siteConfig};

#$dbname = 'gpc1';
#$camera = 'GPC1';
GetOptions(
    'stack_id=s'   => \$stack_id, 
    'sass_id=s'    => \$sass_id,
    'label=s'      => \$label,
    'filter=s'     => \$filter,
    'dbname=s'     => \$dbname,
    'camera=s'     => \$camera,
    ) or pod2usage ( 2 );
pod2usage( -msg => "Required options; --stack_id | --label | --sass_id", -exitval => 3) unless
    ((defined $stack_id)||(defined $label));
pod2usage( -msg => "Required options; --camera --dbname ", -exitval => 3) unless
    (defined($camera) && defined($dbname));
my $verbose = 0;

use constant DB_SOCKET => '/var/run/mysqld/mysqld.sock';
my $dbserver = metadataLookupStr($siteConfig, 'DBSERVER');
my $dbuser = metadataLookupStr($ipprc->{_siteConfig}, "RO_DBUSER");
my $dbpass = metadataLookupStr($ipprc->{_siteConfig}, "RO_DBPASSWORD");
die "database configuration not set up" unless defined($dbserver);
die "database configuration not set up" unless defined($dbuser);
die "database configuration not set up" unless defined($dbpass);
my $db = DBI->connect("DBI:mysql:database=${dbname};host=${dbserver};" .
                   "mysql_socket=" . DB_SOCKET(),
                   ${dbuser},${dbpass}, 
		   { RaiseError => 1, AutoCommit => 1}
    ) or die "Unable to connect to database $DBI::errstr\n";

$ipprc = PS::IPP::Config->new( $camera );
my $mdcParser = PS::IPP::Metadata::Config->new;


my $query = "SELECT DISTINCT chip_id,cam_id,path_base FROM stackRun JOIN stackInputSkyfile USING(stack_id) JOIN stackAssociationMap USING(stack_id) JOIN warpRun USING(warp_id) JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN camProcessedExp USING(cam_id) JOIN chipRun USING (chip_id) WHERE 1 ";

if (defined($label)) {    $query .= " AND stackRun.label = '${label}' "; }
if (defined($stack_id)) { $query .= " AND stack_id = ${stack_id} "; }
if (defined($filter)) {   $query .= " AND stackRun.filter = '${filter}' "; }
if (defined($sass_id))  { $query .= " AND sass_id = ${sass_id} "; }

my $cams = $db->selectall_arrayref( $query );

#my $logDest = $ipprc->filename($logRule, $outroot, $class_id);

print "backgroundStackInputs MULTI\n\n";
my $index = 0;

foreach my $cam (@{ $cams }) {
    my ($chip_id,$cam_id,$cam_path) = @{ $cam };
    my $smf_file = $ipprc->filename("PSASTRO.OUTPUT",$cam_path);
    print "backgroundStackInput${index} METADATA\n";
    print "   chip_id             S64             $chip_id\n";
    print "   cam_id              S64             $cam_id\n";
    print "   astrom              STR             $smf_file\n";
    print "   models              METADATA\n";

    my $command = "chiptool -processedimfile -chip_id $chip_id";
    $command   .= " -dbname $dbname " if defined $dbname;
    
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	die "Unable to parse metadata.";
    }
    my $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	die "Unabel to parse metadata: 2.";

    foreach my $imf (@$imfiles) {
	my $class_id = $imf->{class_id};
	my $path_base= $imf->{path_base};

	my $model = $ipprc->filename("PPIMAGE.BACKMDL",$path_base,$class_id);

	print "      ${class_id}            STR           $model\n";
    }
    print "   END\n";
    print "END\n\n";
    
    $index++;
}

