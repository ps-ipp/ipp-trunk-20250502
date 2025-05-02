#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
#print "\n\n";
#print "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use Digest::MD5::File qw( file_md5_hex );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

use DBI;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $disttool   = can_run('disttool') or (warn "Can't find disttool" and $missing_tools = 1);
my $regtool   = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
my $dist_component   = can_run('dist_component.pl') or (warn "Can't find dist_component.pl" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($dist_id, $camera, $component );
my ($outdir, $run_state, $data_state, $magicked, $poor_quality, $exp_type, $rerun);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);

$camera = 'GPC1';

GetOptions(
           'dist_id=s'      => \$dist_id,    # distribution run identifier
           'component=s'    => \$component,  # the class_id or skycell_id
           'camera=s'       => \$camera,     # the class_id or skycell_id
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dist_id --camera --component",
           -exitval => 3) unless
    defined $dist_id and
    defined $camera and
    defined $component;

my $ipprc = PS::IPP::Config->new($camera); # IPP configuration
$ipprc->redirect_to_logfile($logfile) if $logfile;

my $dbh = getDBHandle() unless $no_update;

my $temproot = metadataLookupStr($ipprc->{_siteConfig}, "TEMP.DIR");

$temproot = "/tmp" if !defined $temproot;

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

$disttool .= " -dbname $dbname" if $dbname;
$regtool .= " -dbname $dbname" if $dbname;

my $comp = get_distcomponent($dist_id, $component);

my $stage = $comp->{stage};
my $stage_id = $comp->{stage_id};
my $no_magic = $comp->{no_magic};
my $path_base;
my $stage_args = "";
if ($stage eq 'raw') {
    # need path_base (get from uri) 
    # exp_type
    # no_magic
    # camera path base if !$no_magic
    my $command = "$regtool -processedimfile -exp_id $stage_id -class_id $component";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $component, $outdir, $error_code);
    }

    my $metadata = $mdcParser->parse (join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);

    my $results = parse_md_list($metadata);

    if ((scalar @$results) != 1) {
        my $n = scalar @$results;
        &my_die("Unexected number of results from regtool -processedimfile: $n", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);
    }
    my $rawfile = $results->[0];
    $path_base = $rawfile->{uri};
    $path_base =~ s/.fits//;
    my $exp_type = $rawfile->{exp_type};
    if (!$no_magic) {
        die "don't know how to create magicked raw bundles yet\n";
    }
    $stage_args .= " --exp_type $exp_type --chip_path_base NULL";
    $stage_args .= " --no_magic" if $no_magic;
} else {

    die "not ready for stage $stage yet\n";
}
my $dist_cmd = "$dist_component --rerun --camera $camera --dist_id $dist_id --stage $stage --stage_id $stage_id --component $component --outdir $comp->{outdir} --path_base $path_base $stage_args";

$dist_cmd .= " --dbname $dbname" if $dbname;
$dist_cmd .= " --verbose" if $verbose;
$dist_cmd .= " --no-update" if $no_update;

print "$dist_cmd\n";

$comp = get_distcomponent($dist_id, $component);

my $bytes = $comp->{bytes};
my $md5sum = $comp->{md5sum};
my $name = $comp->{name};


my $query = "UPDATE dsFile join dsFileset using(fileset_id) join dsProduct USING(prod_id) SET dsFile.bytes =?, dsFile.md5sum = ? WHERE prod_name = ? AND fileset_name = ? AND file_name = ?";

my $stmt = $dbh->prepare($query);

my $filesets = get_filesets($dist_id);

foreach my $fs (@$filesets) {
    my $product = $fs->{product};
    my $fileset = $fs->{fileset};
    $stmt->execute($bytes, $md5sum, $product, $fileset, $name)  or
        &my_die("failed to update dsFile for $product $fileset $name\n", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);

    print "Updated dsFile for $product $fileset $name $bytes $md5sum\n";
}


exit 0;

sub getDBHandle {
    my $dbserver = metadataLookupStr($ipprc->{_siteConfig}, "DS_DBSERVER");
    my $dbuser = metadataLookupStr($ipprc->{_siteConfig}, "DS_DBUSER");
    my $dbpassword = metadataLookupStr($ipprc->{_siteConfig}, "DS_DBPASSWORD");
    my $dbname = metadataLookupStr($ipprc->{_siteConfig}, "DS_DBNAME");

    &my_die("database configuration not set up", $dist_id, $component, $outdir, $PS_EXIT_UNKNOWN_ERROR) unless defined($dbserver) and defined($dbuser)
        and defined($dbpassword) and defined($dbname);

    my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";

    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) 
        or die "Cannot connect to database $dbname on $dbserver.\n";

    return $dbh;
}



sub get_distcomponent {
    my $dist_id = shift;
    my $component = shift;
    my $command = "$disttool -processedcomponent -dist_id $dist_id -component $component";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $component, $outdir, $error_code);
    }

    my $metadata = $mdcParser->parse (join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);

    my $results = parse_md_list($metadata);

    if ((scalar @$results) != 1) {
        my $n = scalar @$results;
        &my_die("Unexected number of results from disttool -processedcomponent: $n", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);
    }
    return $results->[0];
}

sub get_filesets {
    my $dist_id = shift;
    my $command = "$disttool -listfilesets -dist_id $dist_id";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $component, $outdir, $error_code);
    }

    my $output = join "", @$stdout_buf;
    if (!$output) {
        return undef;
    }
    my $metadata = $mdcParser->parse ($output) or
        &my_die("Unable to parse metadata config doc", $dist_id, $component, $outdir, $PS_EXIT_PROG_ERROR);

    my $results = parse_md_list($metadata);

    return $results
}


### Pau.
sub my_die
{
    my $msg = shift;            # Warning message on die
    my $dist_id = shift;        # distRun.dist_id
    my $component = shift;      # class_id, skycell_id, or exposure
    my $outdir = shift;         # output directory
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    exit $exit_code;
}

__END__
