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
use File::Basename qw( dirname);
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Parse the command-line arguments
my ($dist_id, $stage, $stage_id, $outdir);
my ($dbname, $save_temps, $verbose, $no_update, $no_op, $logfile);

GetOptions(
           'dist_id=s'  => \$dist_id,# Magic destreak run identifier
           'stage=s'        => \$stage,      # raw, chip, warp, or diff
           'stage_id=s'     => \$stage_id,   # exp_id, chip_id, warp_id, or diff_id
           'outdir=s'       => \$outdir,     # "directory" for outputs
           'save-temps'     => \$save_temps, # Save temporary files?
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'no-op'          => \$no_op,      # Don't do any operations?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --dist_id --stage --stage_id --outdir",
           -exitval => 3) unless
    defined $dist_id and
    defined $stage and
    defined $stage_id and
    defined $outdir;

$ipprc->redirect_output($logfile) if $logfile;

# Look for programs we need
my $missing_tools;
my $disttool   = can_run('disttool') or (warn "Can't find disttool" and $missing_tools = 1);
my $dsreg   = can_run('dsreg') or (warn "Can't find dsreg" and $missing_tools = 1);
if ($missing_tools) {
    &my_die("Can't find required tools.", $dist_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new;

my $filesets;
{
    my $command = "$disttool -listfilesets -dist_id $dist_id";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
    }
    my $stdout = join "", @$stdout_buf;
    if ($stdout) {
        my $metadata = $mdcParser->parse ($stdout) or
            &my_die("Unable to parse metadata config doc", $dist_id, $PS_EXIT_UNKNOWN_ERROR);

        $filesets = parse_md_list($metadata);
    }
}

foreach my $fileset (@$filesets) {
    my $fs_id = $fileset->{fs_id};
    my $name = $fileset->{fileset};
    my $product = $fileset->{product};
    my $ds_dbname = $fileset->{dbname};
    my $ds_dbserver = $fileset->{dbhost};
    my $command = "$dsreg --del $name --product $product --rm --force";
    $command .= " --ds_dbname $ds_dbname" if defined $ds_dbname;
    $command .= " --ds_dbserver $ds_dbserver" if defined $ds_dbserver;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        my $stderr = join "", @$stderr_buf;
        # fail unless error message says "not found"
        if (!$stderr =~ /not found/) {
            print STDERR $stderr;
            &my_die("failed to delete fileset $name from $product: $error_code", $dist_id, $error_code);
        }
    }
    {
        my $command = "$disttool -updatefileset -fs_id $fs_id -set_state cleaned";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
        }
    }
}

# XXX should we create file rules for these?
my $dbinfo_file = "$outdir/dbinfo.$stage.$stage_id.mdc";
my $dirinfo_file = "$outdir/dirinfo.$stage.$stage_id.mdc";

zap_it($ipprc, $dbinfo_file);
zap_it($ipprc, $dirinfo_file);

my $files;
{
    my $command = "$disttool -processedcomponent -dist_id $dist_id";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
    }
    my $stdout = join "", @$stdout_buf;
    if ($stdout) {
        my $metadata = $mdcParser->parse ($stdout) or
            &my_die("Unable to parse metadata config doc", $dist_id, $error_code);

        $files = parse_md_list($metadata);
    }
}

foreach my $file (@$files) {
    my $outdir = $file->{outdir};
    my $name = "$outdir/" . $file->{name};

    zap_it($ipprc, $name);
}

{
    my $command = "$disttool -updaterun -dist_id $dist_id -set_state cleaned";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
    }
}


exit 0;

### Pau.

sub zap_it {
    my $ipprc = shift;
    my $name = shift;

    if ($ipprc->file_exists($name)) {
        $ipprc->file_delete($name) or my_die("failed to delete $name", $dist_id, $PS_EXIT_UNKNOWN_ERROR);
    }
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $dist_id = shift;        # distRun identifier
    my $exit_code = shift;      # fault code

    # replace exit code if it's nonzero
    $exit_code = $PS_EXIT_PROG_ERROR unless $exit_code;

    my $command = "$disttool -updaterun";
    $command   .= " -dist_id $dist_id";
    $command   .= " -fault $exit_code";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            carp("failed to update database for $dist_id");
        }
    } else {
        print "Skipping command: $command\n";
    }

    carp($msg);
    exit $exit_code;
}

__END__
