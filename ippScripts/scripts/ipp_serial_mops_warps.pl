#!/usr/bin/env perl

use warnings;
use strict;

use DBI;

use IPC::Cmd 0.36 qw( can_run run );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use Data::Dumper;
use File::Temp qw( tempfile );

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config qw( caturi );
use Carp qw( carp );

# Look for programs we need
my $missing_tools;
my $ppmops = can_run('ppMops') or (warn "Can't find ppMops" and $missing_tools = 1);
my $dsreg = can_run('dsreg') or (warn "Can't find dsreg" and $missing_tools = 1);
die "Can't find required tools.\n" if $missing_tools;

my $ipprc = PS::IPP::Config->new; # IPP Configuration

my ( $dbhost,                   # Database host
     $dbname,                   # Database name
     $dbuser,                   # Database user
     $dbpass,                   # Database p/w
     $camera,                   # Camera used
     $outroot,                  # Output directory
     $fileset,                  # File set
     $label,                    # Data label to search for
     $verbose,                  # Verbose output?
     $no_update,                # Don't update state?
     $no_op,                    # Don't do any operations?
     $save_temps                # Save temporary files?
     );

GetOptions(
           'dbhost=s'   => \$dbhost,
           'dbname=s'   => \$dbname,
           'dbuser=s'   => \$dbuser,
           'dbpass=s'   => \$dbpass,
           'camera=s'   => \$camera,
           'outroot=s'  => \$outroot,
           'fileset=s'  => \$fileset,
           'label=s'    => \$label,
           'verbose'    => \$verbose,
           'no-update'  => \$no_update, # Don't update the database?
           'no-op'      => \$no_op, # Don't do any operations?
           'save-temps' => \$save_temps, # Save temporary files?
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dbhost --dbname --dbuser --dbpass --camera --outroot --fileset",
           -exitval => 3)
    unless defined $dbhost
    and defined $dbname
    and defined $dbuser
    and defined $dbpass
    and defined $camera
    and defined $outroot
    and defined $fileset;

$ipprc->define_camera($camera);

my $dbsrc = 'DBI:mysql:database=' . $dbname . ';host=' . $dbhost .
    ';mysql_socket=/var/run/mysqld/mysqld.sock';
my $db = DBI->connect($dbsrc, $dbuser, $dbpass, { RaiseError => 1, AutoCommit => 1 } ) or
    die "Unable to connect to database: $DBI::errstr";

my $where_label = defined $label ? "AND warpRun.label = '$label'" : ""; # WHERE for label

my $sql = "
-- Get a list of exposures on which magic may be performed
SELECT
    rawExp.exp_id,
    MAX(warpRun.warp_id) AS warp_id,
    -- The following trick pulls out the 'zpt_obs' value for the maximum warp_id
    SUBSTRING_INDEX(GROUP_CONCAT(camProcessedExp.zpt_obs ORDER BY warpRun.warp_id), ',', 1) AS zpt_obs
FROM warpRun
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN camProcessedExp USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
WHERE rawExp.camera = '$camera'
    AND warpRun.state = 'full'
    $where_label
GROUP BY exp_id;";

my $warps = $db->selectall_arrayref( $sql, { Slice => {} } ) or die "Unable to execute SQL: $DBI::errstr";

print "Selected " . scalar @$warps . " rows.\n";

$ipprc->outroot_prepare( $outroot );
my $outrootResolved = $ipprc->file_resolve( $outroot );
my ($dsFile, $dsName) = tempfile( "$outrootResolved.dslist.XXXX", UNLINK => !$save_temps);

foreach my $warp ( @$warps ) {
    my $exp_id = $warp->{exp_id};
    my $zp = $warp->{zpt_obs};
    my $warp_id = $warp->{warp_id};

    my $sql = "SELECT * FROM warpSkyfile WHERE warp_id = $warp_id AND fault = 0 AND quality = 0;";
    my $skycells = $db->selectall_arrayref( $sql, { Slice => {} } ) or die "Unable to execute SQL: $DBI::errstr";

    foreach my $skycell ( @$skycells ) {
        my $skycell_id = $skycell->{skycell_id};
        my $path_base = $skycell->{path_base};

        my $input = $ipprc->filename("PSPHOT.OUT.CMF.MEF", $path_base);
        (carp "Can't find $input\n" and next) unless $ipprc->file_exists($input);
        $input = $ipprc->file_resolve($input);

        my $output = caturi( $outroot, "mops.$exp_id.$skycell_id.$warp_id.fits" );
        $output = $ipprc->file_resolve($output);

        unless ($no_op) {
            $ipprc->file_prepare($output);
            my $command = "$ppmops $input $zp $output";
            my $success = run( command => $command, verbose => $verbose );
            (carp "Couldn't translate $input\n" and next) unless $success;
        }

        # format: filename|filesize|md5sum|filetype|
        # note: since we omit filesize and md5sum, dsreg will calculate them
        print $dsFile "${output}|||ipp-mops|\n";
    }
}
close $dsFile;
$db->disconnect;

# Register new files with the data store
unless ($no_update) {
    my $command = "$dsreg --add $fileset --product mops_transient_detections --type MOPS_TRANSIENT_DETECTIONS --list $dsName --copy --abspath --dbname DataStore";
    my $success = run( command => $command, verbose => $verbose );
    die "Couldn't register files with data store.\n" unless $success;
}


__END__

