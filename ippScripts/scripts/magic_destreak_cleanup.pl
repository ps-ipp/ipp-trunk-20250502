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
use File::Basename qw( basename dirname );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );
use Nebulous::Client;
use DBI;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $magicdstool   = can_run('magicdstool') or (warn "Can't find magicdstool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($magic_ds_id, $camera, $stage);
my ($dbname, $save_temps, $verbose, $no_update, $no_op, $logfile);

GetOptions(
           'magic_ds_id=s'  => \$magic_ds_id,# Magic destreak run identifier
           'camera=s'       => \$camera,     # camera for evaluating file rules
           'stage=s'        => \$stage,      # ipp stage for this magicDSRun
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'no-op'          => \$no_op,      # Don't do any operations?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --magic_ds_id --camera",
           -exitval => 3) unless
    defined $magic_ds_id and
    defined $camera and
    defined $stage;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $magic_ds_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$ipprc->redirect_to_logfile($logfile) or my_die( "Unable to redirect output", $magic_ds_id, $PS_EXIT_SYS_ERROR ) if $logfile;


$dbname = metadataLookupStr( $ipprc->{_siteConfig}, 'DBNAME' ) if !$dbname;
&my_die ("Unable to find DBNAME in site or command line arguments", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if !$dbname;

my $dbuser = metadataLookupStr( $ipprc->{_siteConfig}, 'DBUSER' );
&my_die ("Unable to find DBUSER in site", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if !$dbuser;

my $dbpassword = metadataLookupStr( $ipprc->{_siteConfig}, 'DBPASSWORD' );
&my_die ("Unable to find DBPASSWORD in site", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if !$dbpassword;

my $dbserver = metadataLookupStr( $ipprc->{_siteConfig}, 'DBSERVER' );
&my_die ("Unable to find DBSERVER in site", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if !$dbserver;

my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";
my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) or die "Cannot connect to mysql server\n";

my $q1;

if ($stage ne 'diff') {
    $q1 = "SELECT magicDSRun.*, camera, camProcessedExp.path_base AS cam_path_base, camRun.reduction AS cam_reduction"
     . " FROM magicDSRun JOIN magicRun USING(magic_id) JOIN rawExp USING(exp_id) LEFT JOIN camProcessedExp USING(cam_id) LEFT JOIN camRun USING(cam_id)";
} else {
    $q1 = "SELECT magicDSRun.*, diffRun.diff_mode FROM magicDSRun JOIN diffRun ON stage_id = diffRun.diff_id AND stage = 'diff'";
}
$q1 .= " WHERE magic_ds_id = $magic_ds_id";

my $q2 = "SELECT * from magicDSFile WHERE (data_state = 'full' OR data_state = 'update') AND magic_ds_id = $magic_ds_id";

my $stmt1 = $dbh->prepare($q1);
$stmt1->execute();
my $nrows = $stmt1->rows;
&my_die ("Unable to find magicDSRun $magic_ds_id", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if !$nrows;
my $run = $stmt1->fetchrow_hashref();
$stmt1->finish();

my $state = $run->{state};
my $stage_id = $run->{stage_id};
my $cam_path_base = $run->{cam_path_base};
my $cam_reduction = $run->{cam_reduction};
my $replace = $run->{re_place};
$cam_reduction = 'DEFAULT' if !$cam_reduction or ($cam_reduction eq 'NULL');

my $warp_warp = ($stage eq 'diff' and $run->{diff_mode} eq 1);


&my_die("cleanup not supported for camera stage", $magic_ds_id, $PS_EXIT_PROG_ERROR) if $stage eq "camera";


&my_die("unexpected run state found: $state", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if $state ne "goto_cleaned";
&my_die("cleanup not allowed for raw stage, use goto_restore", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if $stage eq "raw" and $replace;

my $recipe_psastro;
if ($stage eq 'chip') {
    $recipe_psastro = $ipprc->reduction($cam_reduction, 'PSASTRO'); # Recipe to use
    &my_die("Unrecognised PSASTRO recipe", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) unless defined $recipe_psastro;
}


my $stmt2 = $dbh->prepare($q2);
$stmt2->execute();

my $num_components = 0;
my @components;
# save the data, so we can disconnect from the database
# deleting can take awhile
while (my $comp = $stmt2->fetchrow_hashref()) {
        push @components, $comp;
        $num_components++;
}
if ($num_components == 0) {
    # no components to clean up set run state to cleaned
    my $command = "$magicdstool -updaterun -magic_ds_id $magic_ds_id -set_state cleaned";
    $command   .= " -dbname $dbname" if defined $dbname;

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            carp("failed to update database for $magic_ds_id");
        }
    } else {
        print "Skipping command: $command\n";
    }
    exit 0;
}

$stmt2->finish();
$dbh->disconnect() or warn $dbh->errstr;

# We no longer clean up the camera stage mask files, but the code was left in place in case
# we change our minds.
my $cleanup_cam_mask = 0;

my $dynamicMasks;               # Use dynamic masks?
foreach my $comp (@components) {
        my $component = $comp->{component};
        my $backup_path_base = $comp->{backup_path_base};
        my ($bimage, $bmask, $bch_mask, $bweight, $bsources, $bastrom);
#        my ($rimage, $rmask, $rch_mask, $rweight, $rsources, $rastrom);

        if ($stage eq "chip") {
            # Check to see if we're using dynamic masks
            if ($cleanup_cam_mask && !defined $dynamicMasks) {
                # Get the PSASTRO recipe
                my $command = "$ppConfigDump -camera $camera -recipe PSASTRO $recipe_psastro -dump-recipe PSASTRO -";
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform ppConfigDump: $error_code", $magic_ds_id, $component,
                            $PS_EXIT_CONFIG_ERROR);
                }
                my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
                my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
                    &my_die("Unable to parse metadata config doc", $magic_ds_id, $component,
                            $PS_EXIT_CONFIG_ERROR);

                $dynamicMasks = metadataLookupBool($recipeData, 'REFSTAR_MASK');
            }

            if ($backup_path_base) {
                $bimage  = $ipprc->filename("PPIMAGE.CHIP", $backup_path_base, $component);

                if ($dynamicMasks) {
                    my $mask = $ipprc->filename("PSASTRO.OUTPUT.MASK", $cam_path_base, $component);
                    # This is kludgey but correct
                    $bmask = dirname($backup_path_base) . "/SR_" . basename($mask);
                    $bch_mask= $ipprc->filename("PPIMAGE.CHIP.MASK", $backup_path_base, $component);
                } else {
                    $bmask = $ipprc->filename("PPIMAGE.CHIP.MASK", $backup_path_base, $component);
                }
                $bweight = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $backup_path_base, $component);
                $bsources = $ipprc->filename("PSPHOT.OUTPUT", $backup_path_base, $component);
            }
        } elsif ($stage eq "camera") {
            if ($backup_path_base) {
                $bastrom = $ipprc->filename("PSASTRO.OUTPUT", $backup_path_base);
            }
        } elsif ($stage eq "warp") {
            if ($backup_path_base) {
                $bimage  = $ipprc->filename("PSWARP.OUTPUT", $backup_path_base);
                $bmask   = $ipprc->filename("PSWARP.OUTPUT.MASK", $backup_path_base);
                $bweight = $ipprc->filename("PSWARP.OUTPUT.VARIANCE", $backup_path_base);
                $bsources = $ipprc->filename("PSWARP.OUTPUT.SOURCES", $backup_path_base);
            }
        } elsif ($stage eq "diff") {
            my $name = "PPSUB.OUTPUT";
            if ($backup_path_base) {
                $bimage  = $ipprc->filename($name, $backup_path_base);
                $bmask   = $ipprc->filename("$name.MASK", $backup_path_base);
                $bweight = $ipprc->filename("$name.VARIANCE", $backup_path_base);
                # bills 2011-01-24
                # don't clean up the uncensored Diff sources file
                # $bsources = $ipprc->filename("$name.SOURCES", $backup_path_base);
            }
        } elsif ($stage eq "raw") {
            if ($backup_path_base) {
                if ($backup_path_base =~ /fits/) {
                    $bimage = $backup_path_base;
                } else {
                    $bimage = $backup_path_base . ".fits";
                }
            }
        }

        delete_files($bimage, $bmask, $bweight, $bsources, $bastrom, $bch_mask);

        if ($stage eq "diff" and $warp_warp) {
            my $name = "PPSUB.INVERSE";
            if ($backup_path_base) {
                $bimage  = $ipprc->filename($name, $backup_path_base);
                $bmask   = $ipprc->filename("$name.MASK", $backup_path_base);
                $bweight = $ipprc->filename("$name.VARIANCE", $backup_path_base);
                # bills 2011-01-24
                # don't clean up the uncensored sources file
                # $bsources = $ipprc->filename("$name.SOURCES", $backup_path_base);
            }
            delete_files($bimage, $bmask, $bweight, $bsources);
        }
        my $command = "$magicdstool -tocleanedfile -magic_ds_id $magic_ds_id -component $component";
        $command   .= " -dbname $dbname" if defined $dbname;

        unless ($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                carp("failed to update database for $magic_ds_id");
            }
        } else {
            print "Skipping command: $command\n";
        }
}

### Pau.

sub delete_files {
    foreach my $file (@_) {
        if ($file) {
            my $error_code = $ipprc->kill_file($file);
            my_die("Failed to delete $file", $magic_ds_id, $error_code) if $error_code;
        }
    }
}



sub my_die
{
    my $msg = shift;            # Warning message on die
    my $magic_ds_id = shift;    # Magic DS identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    my $command = "$magicdstool -updaterun -set_state error_cleaned";
    $command   .= " -magic_ds_id $magic_ds_id";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            carp("failed to update database for $magic_ds_id");
        }
    } else {
        print "Skipping command: $command\n";
    }

    carp($msg);
    exit $exit_code;
}

__END__
