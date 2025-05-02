#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Basename qw( basename );
use Digest::MD5::File qw( file_md5_hex );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


# Look for programs we need
my $missing_tools;
my $disttool   = can_run('disttool') or (warn "Can't find disttool" and $missing_tools = 1);
my $dsreg   = can_run('dsreg') or (warn "Can't find dsreg" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($dist_id, $dist_dir, $target_id, $stage, $stage_id, $dest_id, $product_name, $ds_dbhost, $ds_dbname);
my ($label, $data_group, $filter);
my ($dbname, $save_temps, $verbose, $no_update, $logfile);

GetOptions(
           'dist_id=s'      => \$dist_id,    # distribution run identifier
           'dist_dir=s'     => \$dist_dir,   # directory containing dist run outputs
           'target_id=s'    => \$target_id,  #
           'stage=s'        => \$stage,      # raw, chip, camera, fake, warp, stack, or diff
           'stage_id=s'     => \$stage_id,   # exp_id, chip_id, etc.
           'dest_id=s'      => \$dest_id,    # id for the product
           'product_name=s' => \$product_name,  # location of the data store directory for this product
           'label=s'        => \$label,
           'data_group=s'   => \$data_group,
           'filter=s'       => \$filter,
           'ds_dbhost=s'    => \$ds_dbhost,  # database host for the datastore database
           'ds_dbname=s'    => \$ds_dbname,  # database name for the datastore database
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dist_id --dist_dir --target_id --stage --stage_id --data_group --filter --dest_id --ds_dbhost --ds_dbname",
           -exitval => 3) unless
    defined $dist_id and
    defined $dist_dir and
    defined $target_id and
    defined $stage and
    defined $stage_id and
    defined $data_group and
    defined $filter and
    defined $dest_id and
    defined $product_name and
    defined $ds_dbhost and
    defined $ds_dbname;

$ipprc->redirect_output($logfile) if $logfile;


my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $fs_tag = get_fileset_tag($ipprc, $stage, $stage_id, $dbname);

&my_die("failed to lookup fileset tag", $dist_id, $dest_id, $PS_EXIT_UNKNOWN_ERROR) if !defined $fs_tag;

my $fileset_name = $fs_tag ? "$fs_tag." : "";
$fileset_name .= "$stage.$stage_id.$dist_id.$dest_id";

print "$fileset_name\n";


# make sure that the database info file for this run exists
my $dbinfo_file = "$dist_dir/dbinfo.$stage.$stage_id.mdc";
if (! $ipprc->file_exists($dbinfo_file) ) {
    &my_die("dbinfo file for dist run $dbinfo_file not found", $dist_id, $dest_id, $PS_EXIT_UNKNOWN_ERROR);
}
print "dbinfo file $dbinfo_file exists\n" if $verbose;

# make sure that the dirinfo file for this run exists
my $dirinfo_file = "$dist_dir/dirinfo.$stage.$stage_id.mdc";
if (!$ipprc->file_exists($dirinfo_file)) {
    &my_die("dirinfo file for dist run $dirinfo_file not found", $dist_id, $dest_id, $PS_EXIT_UNKNOWN_ERROR);
}
print "dirinfo file $dirinfo_file exists\n" if $verbose;

# open the dsreg file list
my ($listFile, $listFileName) = tempfile("/tmp/$stage.$stage_id.list.XXXX", UNLINK => !$save_temps );

# add the dbinfo file to the list
# XXX: change type from text to dbinfo and dirinfo once we define the types
print $listFile "$dbinfo_file|||text|dbinfo|\n";
print $listFile "$dirinfo_file|||text|dirinfo|\n";

my $components;
{
    my $command = "$disttool -processedcomponent -dist_id $dist_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command error_code: $error_code", $dist_id, $dest_id, $error_code);
    }

    my $metadata = $mdcParser->parse (join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $dist_id, $dest_id, $PS_EXIT_PROG_ERROR);

    $components = parse_md_list($metadata);

    my $num_components = scalar @$components;

    print "distRun $dist_id has $num_components components\n";
}

foreach my $component (@$components) {
    my $size = $component->{bytes};
    # skip zero size components. They are placeholders to complete processing
    next if $size == 0;
    my $md5sum = $component->{md5sum};
    # name of the file
    my $file_name = $component->{name};
    # component id (class_is or skycell_id)
    my $comp_dir = $component->{outdir};
    my $comp_name = $component->{component};

    # XXX: if tarfile is not always the right type we need to add a type to distComponent
    print $listFile "$comp_dir/$file_name|$size|$md5sum|tgz|$comp_name|\n";
}

close $listFile;

{
    my $command = "$dsreg --add $fileset_name --product $product_name --type IPP-DIST --list $listFileName";

    # the data store will refer to the distribution bundle via symlinks back to distRun.outdir
#    $command .= " --datapath $dist_dir --link";
    $command .= " --abspath --link";

    # set the product specific columns in product list
    my $prod_col_3 = $fs_tag ? $fs_tag : "$stage.$stage_id";

    $command .= " --ps0 $target_id --ps1 $stage --ps2 $stage_id --ps3 $prod_col_3";
    $command .= " --ps4 $data_group --ps5 $filter";

    $command .= " --dbname $ds_dbname --dbhost $ds_dbhost";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
         &my_die("Unable to perform $command error_code: $error_code", $dist_id, $dest_id, $error_code);
    }
}

{
    my $command = "$disttool -addfileset -dist_id $dist_id -dest_id $dest_id -name $fileset_name";
    $command .= " -dbname $dbname" if $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        # XXX: if we get here we the fileset has been created but the database update failed
        # We need to have revertfileset check whether the fileset exists and do dsreg -del if it does
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command error_code: $error_code", $dist_id, $dest_id, $error_code);
    }
}



exit 0;

sub getDBHandle {
    my $ipprc = shift;
    my $dbname = shift;
    if (!$dbname) {
        $dbname = metadataLookupStr($ipprc->{_siteConfig}, "DBNAME");
    }
    my $dbserver = metadataLookupStr($ipprc->{_siteConfig}, "DBSERVER");
    my $dbuser = metadataLookupStr($ipprc->{_siteConfig}, "DBUSER");
    my $dbpassword = metadataLookupStr($ipprc->{_siteConfig}, "DBPASSWORD");

    die "database configuration not set up" unless defined($dbserver) and defined($dbuser)
        and defined($dbpassword) and defined($dbname);

    my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";

    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword)
        or die "Cannot connect to database.\n";

    return $dbh;
}


sub get_fileset_tag {
    my $ipprc = shift;
    my $stage = shift;
    my $stage_id = shift;
    my $dbname = shift;

    if (($stage eq 'stack') or ($stage eq 'diff') or ($stage eq 'SSdiff') or $stage eq 'sky' or $stage eq 'skycal' or $stage eq "ff") {
        return "";
    }

    #
    # we are a long ways away from the rawExp in the pipeline. Rather than do some
    # very long joins in disttool, we look up the exp_name in the database using DBI
    #
    my $dbh = getDBHandle($ipprc, $dbname);

    my $query;

    if ($stage eq 'raw') {
        $query = "SELECT exp_name FROM rawExp WHERE exp_id = $stage_id";
    } elsif ($stage eq 'chip') {
        $query = "SELECT exp_name FROM chipRun JOIN rawExp USING(exp_id) WHERE chip_id = $stage_id";
    } elsif ($stage eq 'chip_bg') {
        $query = "SELECT exp_name FROM chipBackgroundRun JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) WHERE chip_bg_id = $stage_id";
    } elsif ($stage eq 'camera') {
        $query = "SELECT exp_name FROM camRun JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id)"
                    . " WHERE cam_id = $stage_id";
    } elsif ($stage eq 'fake') {
        $query = "SELECT exp_name FROM fakeRun JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id)"
                    . " JOIN rawExp USING(exp_id) WHERE fake_id = $stage_id";
    } elsif ($stage eq 'warp') {
        $query = "SELECT exp_name FROM warpRun JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id)"
                    . " JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) WHERE warp_id = $stage_id";
    } elsif ($stage eq 'warp_bg') {
        $query = "SELECT exp_name FROM warpBackgroundRun JOIN warpRun USING(warp_id) JOIN fakeRun USING(fake_id) JOIN camRun USING(cam_id) JOIN chipRun USING(chip_id) JOIN rawExp USING(exp_id) WHERE warp_bg_id = $stage_id";

    } else {
        &my_die("$stage is invalid value for stage", $dist_id, $dest_id, $PS_EXIT_UNKNOWN_ERROR);
    }

    my $stmt = $dbh->prepare($query);
    $stmt->execute();
    my $ref = $stmt->fetchrow_hashref();

    my $tag = $ref->{exp_name};

    return "$tag";
}

sub my_die {
    my $msg = shift;
    my $dist_id = shift;
    my $dest_id = shift;
    my $fault = shift;

    # TODO: disttool -adddsfileset -dest_id $dest_id -dist_id $dist_id -fault $fault
    print STDERR "$msg\n";

    my $command = "$disttool -addfileset -dist_id $dist_id -dest_id $dest_id -fault $fault";
    $command .= " -dbname $dbname" if $dbname;

    if (!$no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            print STDERR "Unable to perform $command error_code: $error_code\n";
        }
    } else {
        print STDERR "skipping $command\n";
    }
    exit $fault;
}

__END__
