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

my $where_label = defined $label ? "AND diffRun.label = '$label'" : ""; # WHERE for label

my $sql = "
-- Get a list of exposures on which magic may be performed
SELECT
    rawExp.exp_id,
    MAX(diffWarps.diff_id) AS diff_id,
    -- The following trick pulls out the appropriate values for the maximum diff_id
    SUBSTRING_INDEX(GROUP_CONCAT(camProcessedTemplate.zpt_obs ORDER BY diffWarps.diff_id), ',', 1) AS zpt_obs,
    SUBSTRING_INDEX(GROUP_CONCAT(rawTemplate.exp_time ORDER BY diffWarps.diff_id), ',', 1) AS exp_time,
    CONVERT(SUBSTRING_INDEX(GROUP_CONCAT(diffWarps.inverse ORDER BY diffWarps.diff_id), ',', 1), UNSIGNED) AS inverse
FROM (
    -- Forward diffs
    SELECT
        diffRun.diff_id,
        warp1 AS warp_id,
        warp1 AS template_warp,
        0 AS inverse
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    WHERE diffInputSkyfile.warp1 IS NOT NULL
        AND diffRun.state = 'full'
        AND diffRun.exposure = 1
        $where_label
    UNION
    -- Backward diffs
    SELECT
        diffRun.diff_id,
        warp2 AS warp_id,
        warp1 AS template_warp,
        1 AS inverse
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    WHERE diffInputSkyfile.warp2 IS NOT NULL
        AND diffRun.state = 'full'
        AND diffRun.exposure = 1
        AND diffRun.bothways = 1
        $where_label
    ) AS diffWarps
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
JOIN rawExp USING(exp_id)
JOIN warpRun AS warpTemplate ON warpTemplate.warp_id = diffWarps.template_warp
JOIN fakeRun AS fakeTemplate ON fakeTemplate.fake_id = warpTemplate.fake_id
JOIN camRun AS camTemplate ON camTemplate.cam_id = fakeTemplate.cam_id
JOIN camProcessedExp AS camProcessedTemplate ON camProcessedTemplate.cam_id = camTemplate.cam_id
JOIN chipRun AS chipTemplate ON chipTemplate.chip_id = camTemplate.chip_id
JOIN rawExp AS rawTemplate ON rawTemplate.exp_id = chipTemplate.exp_id
WHERE rawExp.camera = '$camera'
GROUP BY exp_id;";

my $diffs = $db->selectall_arrayref( $sql, { Slice => {} } ) or die "Unable to execute SQL: $DBI::errstr";

print "Selected " . scalar @$diffs . " rows.\n";

$ipprc->outroot_prepare( $outroot );
my $outrootResolved = $ipprc->file_resolve( $outroot );
my ($dsFile, $dsName) = tempfile( "$outrootResolved.dslist.XXXX", UNLINK => !$save_temps);

foreach my $diff ( @$diffs ) {
    my $exp_id = $diff->{exp_id};
    my $zp = $diff->{zpt_obs};
    my $exp_time = $diff->{exp_time};
    my $diff_id = $diff->{diff_id};
    my $inverse = $diff->{inverse};

    (carp "Bad ZP or EXPTIME for $exp_id" and next) if not defined $zp or not defined $exp_time;
    $zp += 2.5 * log($exp_time) / log(10);

    my $sql = "SELECT * FROM diffSkyfile WHERE diff_id = $diff_id AND fault = 0 AND quality = 0;";
    my $skycells = $db->selectall_arrayref( $sql, { Slice => {} } ) or die "Unable to execute SQL: $DBI::errstr";

    foreach my $skycell ( @$skycells ) {
        my $skycell_id = $skycell->{skycell_id};
        my $path_base = $skycell->{path_base};

        my $sources = $inverse ? "PPSUB.INVERSE.SOURCES" : "PPSUB.OUTPUT.SOURCES";

        my $input = $ipprc->filename($sources, $path_base);
        (carp "Can't find $input\n" and next) unless $ipprc->file_exists($input);
        $input = $ipprc->file_resolve($input);

        my $output = caturi( $outroot, "mops.$exp_id.$skycell_id.$diff_id.fits" );
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

