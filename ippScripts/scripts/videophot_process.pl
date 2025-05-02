#!/usr/bin/env perl

use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use Data::Dumper;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use File::Temp qw( tempfile );
use File::Basename qw( basename );

# Look for programs we need
my $missing_tools;
my $vptool = can_run('vptool') or (warn "Can't find vptool" and $missing_tools = 1);
my $psvideophot = can_run('psvideophot') or (warn "Can't find psvideophot" and $missing_tools = 1);
my $dumpvideo = can_run('dumpvideo') or (warn "Can't find dumpvideo" and $missing_tools = 1);
my $listvideocells = can_run('listvideocells.pl') or (warn "Can't find listvideocells.pl" and $missing_tools = 1);
my $dsreg = can_run('dsreg') or (warn "Can't find dsreg" and $missing_tools = 1);
my $nebrepair = can_run('neb-repair') or (warn "Can't find neb-repair" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}
my ($vp_id, $camera, $outroot, $dest_id, $dbname, $verbose, $no_update, $no_op, $redirect, $save_temps);
my ($product, $save_video_cube, $ds_dbname, $ds_dbhost);

GetOptions(
    'vp_id=s'           => \$vp_id,
    'camera=s'          => \$camera, 
    'outroot=s'         => \$outroot,
    'dest_id=s'         => \$dest_id,
    'product=s'         => \$product,
    'save-video-cube'   => \$save_video_cube,
    'ds_dbname=s'       => \$ds_dbname,
    'ds_dbhost=s'       => \$ds_dbhost,
    'dbname|d=s'        => \$dbname,
    'verbose'           => \$verbose,
    'save-temps'        => \$save_temps,
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op,     # Don't do any operations?
    'redirect-output'   => \$redirect,  # redirect output streams to logfile
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --vp_id --outroot --camera",
    -exitval => 3,
          ) unless defined $vp_id
            and defined $camera
            and defined $outroot;

pod2usage(
    -msg => " --product -ds_dbname and --dsdbhost are required if --dest_id",
    -exitval => 3,
          ) if ($dest_id and !( defined $product and defined $ds_dbname and defined $ds_dbhost));


$no_update = 1 if $no_op;
$vptool .= " -dbname $dbname" if $dbname;

my $ipprc = PS::IPP::Config->new($camera) or my_die( "Unable to set up", $vp_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot);
if ($redirect) {
    $ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $vp_id, $PS_EXIT_SYS_ERROR );

    print "\n\n";
    print "Starting script $0 on $host at $date\n\n";
}

# Get list of chips with pending video cells
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $files;
{
    my $command = "$vptool -pendingimfile -vp_id $vp_id";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform vptool -pendingimfile: $error_code", $vp_id, $error_code);
    }

    my $output = join "", @$stdout_buf;
    if ($output) {
        my $metadata = $mdcParser->parse($output) or
            &my_die("Unable to parse metadata config doc", $vp_id, $PS_EXIT_PROG_ERROR);
        $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $vp_id, $PS_EXIT_PROG_ERROR);
    }
}


foreach my $file (@$files) {
    my $class_id = $file->{class_id};
    my $uri = $file->{uri};

    my $resolved = $ipprc->file_resolve($uri);
    &my_die("Unable to resolve $uri", $vp_id, $PS_EXIT_UNKNOWN_ERROR) unless $resolved;

    my $command = "$listvideocells --file $resolved";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        check_input_file($uri, $vp_id, $class_id, 'nocell');
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $vp_id, $error_code);
    }

    my @video_cells = split "\n", (join "", @$stdout_buf);
    &my_die("No video cells found in $uri", $vp_id, $PS_EXIT_UNKNOWN_ERROR) unless scalar @video_cells;

    foreach my $cell_id (@video_cells) {
        my $path_base =  "$outroot.$class_id.$cell_id";
        my $error;
        my $output = $ipprc->prepare_output("PSVIDEOPHOT.OUTPUT", $path_base, undef, 1, \$error)
            or &my_die("failed to prepare output file for PSVIDEOPHOT.OUTPUT", $vp_id, $error);
        my $command = "$psvideophot $output";
        $command .= " -file $uri";
        $command .= " -class_id $class_id";
        $command .= " -cell_id $cell_id";
        my $vpstart = DateTime->now->mjd;
        unless ($no_op) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform psvideophot: $error_code", $vp_id, $error_code);
            }
        } else {
            print "Not executing: $command\n";
        }
        # dump the video cell and the video table to a file
        if ($save_video_cube) {
            my $output = $path_base . ".fits";
            my $extname = "$cell_id";
            my $command = "$dumpvideo $uri $extname $output -includetable";
            unless ($no_op) {
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to perform dumpvideo: $error_code", $vp_id, $error_code);
                }
            } else {
                print "Not executing: $command\n";
            }
        }

        unless ($no_update) {
            my $command = "$vptool -addprocessedcell -vp_id $vp_id -class_id $class_id -cell_id $cell_id";
            $command .= " -path_base $path_base";
            $command .= " -hostname $host";
            $command .= " -dtime_photom " . ((DateTime->now->mjd - $vpstart) * 86400);

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                my $err_message = "Unable to perform vptool -addvpcell";
                &my_die("$err_message: $error_code", $vp_id, $error_code);
            }
        }
    }
}
if ($dest_id) {
    my $command = "$vptool -processedcell -vp_id $vp_id";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform vptool -pendingimfile: $error_code", $vp_id, $error_code);
    }

    my $output = join "", @$stdout_buf;
    my $cells;
    if ($output) {
        my $metadata = $mdcParser->parse($output) or
            &my_die("Unable to parse metadata config doc", $vp_id, $PS_EXIT_PROG_ERROR);
        $cells = parse_md_list($metadata) or
            &my_die("Unable to parse metadata list", $vp_id, $PS_EXIT_PROG_ERROR);
    }
    my ($reglist, $reglistName) = tempfile("/tmp/filelist.$vp_id.XXXX", UNLINK => !$save_temps);

    if ($redirect) {
        print $reglist "$logDest|||text|\n";
    }
    # list of extensions and types for files to distribute
    my @exts = qw( vpt );
    my @types = qw( table );
    if ($save_video_cube) {
        push @exts, 'fits';
        push @types, 'fits';
    }
    foreach my $cell (@$cells) {
        my $path_base = $cell->{path_base};
        for (my $i = 0; $i < scalar @exts; $i++) {
            my $file = "$path_base.$exts[$i]";
            my $type = $types[$i];

            print $reglist "$file|||$type|\n";
        }
    }
    close $reglist or &my_die("failed to close $reglistName", $vp_id, $PS_EXIT_UNKNOWN_ERROR);
    unless ($no_update or $no_op) {
        my $fileset = basename($outroot);
        my $command = "$dsreg --add $fileset --product $product --type dump --list $reglistName";
        $command .= " --abspath --link";
        $command .= " --dbname $ds_dbname --dbhost $ds_dbhost";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
             &my_die("Unable to perform $command error_code: $error_code", $vp_id, $error_code);
        }
    }
}

unless ($no_update) {
    my $command = "$vptool -updaterun -vp_id $vp_id -set_state full"; 
    $command .= " -set_outroot $outroot";
    $command .= " -set_hostname $host";
    $command .= " -set_dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        my $err_message = "Unable to perform vptool -updaterun";
        &my_die("$err_message: $error_code", $vp_id, $error_code);
    }
}

exit 0;

sub check_input_file {
    my $uri = shift;
    my $vp_id = shift;
    my $class_id = shift;
    my $cell_id = shift;
    my $resolved = $ipprc->file_resolve($uri);

    my $tryrepair = 0;
    if (!-e $resolved) {
        printf STDERR "instance $resolved for $uri does not exist\n";
        $tryrepair = 1;
    } elsif (-s $resolved == 0) {
        printf STDERR "instance $resolved for $uri is empty\n";
        $tryrepair = 1;
    }
    if ($tryrepair) {
        my $scheme = file_scheme($uri);
        if ($scheme and ($scheme = 'neb')) {
            my $command = "$nebrepair $uri";
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                my $err_message = "Unable to perform nebrepair";
                &my_die("$err_message: $error_code", $vp_id, $error_code);
            }
       }
    }
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $vp_id = shift;          # vpRun id
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $vp_id and not $no_update) {
        my $command = "$vptool -vp_id $vp_id";
        $command .= " -updaterun";
        $command .= " -set_fault $exit_code";
        $command .= " -set_hostname $host" if defined $host;
        $command .= " -set_outroot $outroot" if defined $outroot;
        $command .= " -set_dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400);

        run(command => $command, verbose => $verbose);
    }
    exit $exit_code;
}

__END__
