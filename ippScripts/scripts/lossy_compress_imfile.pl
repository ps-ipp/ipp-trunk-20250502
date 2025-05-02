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


use Digest::MD5::File qw( file_md5_hex );
use File::Basename qw( basename);
use IPC::Run;
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Config 1.01 qw( :standard );
use PS::IPP::Metadata::Config;
use File::Temp qw( tempfile );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $regtool = can_run( 'regtool' ) or (warn "Can't find regtool" and $missing_tools = 1);
my $fpack   = can_run( 'fpack' ) or (warn "Can't find fpack" and $missing_tools = 1);
my $funpack   = can_run( 'funpack' ) or (warn "Can't find funpack" and $missing_tools = 1);
my ($exp_id, $class_id, $exp_name, $uri, $bytes, $md5sum, $dbname, $state);
my ($verbose, $no_update, $no_op, $logfile, $save_temps);
my ($camera);
GetOptions(
    'exp_id|e=s'          => \$exp_id,
    'class_id|i=s'        => \$class_id,
    'exp_name|n=s'        => \$exp_name,
    'camera=s'            => \$camera,
    'uri|u=s'             => \$uri,
    'bytes=s'             => \$bytes,
    'md5sum=s'            => \$md5sum,
    'dbname|d=s'          => \$dbname,
    'state=s'             => \$state,
    'verbose'             => \$verbose,
    'no-update'           => \$no_update,
    'no-op'               => \$no_op,
    'logfile=s'           => \$logfile,
    'save-temps'          => \$save_temps,
    ) or pod2usage ( 2 );


pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --exp_id --class_id --exp_name --uri --camera --state", -exitval => 3) unless
    defined $exp_id and
    defined $class_id and
    defined $exp_name and
    defined $uri and
    defined $camera and
    defined $state;

my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $exp_id, $exp_name, $class_id, $uri, $state, $PS_EXIT_CONFIG_ERROR );
if (defined($logfile)) {
    $ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $exp_id, $exp_name, $class_id, $uri, $state, $PS_EXIT_SYS_ERROR );
}

my $RECIPE = "LOSSYCOMP";

if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR);
}



my $now_time = localtime();
printf STDERR "\nstarting compression: %s\n", $now_time if $verbose;


# If goto_compress
#  - verify disk file matches
#  - create compressed version
#  - update database
#

if ($state eq 'goto_compressed') {
    # Find the actual filename for this run:
    &my_die("Couldn't find input file: $uri\n", $exp_id,$exp_name,$class_id,$uri,$state,$PS_EXIT_SYS_ERROR)
        unless ($ipprc->file_exists($uri));
    my $uriReal = $ipprc->file_resolve( $uri );

    # Create a compressed version:
    my $compUri = $uri . ".fz";
    &my_die("Output compressed file already exists: $compUri\n", $exp_id,$exp_name,$class_id,$uri,$state,$PS_EXIT_SYS_ERROR)
        if ($ipprc->file_exists($compUri));
    my $compReal= $ipprc->file_resolve( $compUri, 'create');
    # Apparently we need to funpack before fpacking.  Probably should have realized that beforehand.
    my $tempfile = new File::Temp ( TEMPLATE => "${exp_name}.XXXX",
                                    DIR => '/tmp/',
                                    UNLINK => !$save_temps,
                                    SUFFIX => '.fits');
    my $tempReal = $tempfile->filename;

    my $uncompress_command = "$funpack -S $uriReal > $tempReal";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) =
        run(command => $uncompress_command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to uncompress file: $uri -> $compUri: $error_code",
                $exp_id,$exp_name,$class_id,$uri, $state, $PS_EXIT_SYS_ERROR);
    }
    my $compress_command = "$fpack -h -s 8 -S $tempReal >  $compReal";
    print STDERR "$compReal $uriReal $compress_command\n";
    ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) =
        run(command => $compress_command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to compress file: $uri -> $compUri: $error_code",
                $exp_id,$exp_name,$class_id,$uri, $state, $PS_EXIT_SYS_ERROR);
    }
    my $database_command = "$regtool -updateprocessedimfile -exp_id $exp_id -class_id $class_id -set_state compressed";
    $database_command .= " -dbname $dbname" if defined $dbname;

    &my_die("Expected output compressed file not found: $compUri\n", $exp_id,$exp_name,$class_id, $uri,$state, $PS_EXIT_SYS_ERROR)
        unless ($ipprc->file_exists($compUri));

    if ($no_update || $no_op) {
        print STDERR "NOUPDATE: $database_command\n";
    }
    else {
        ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) =
            run(command => $database_command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to update database file: $uri -> $compUri: $error_code",
                    $exp_id,$exp_name,$class_id,$uri, $state, $PS_EXIT_SYS_ERROR);
        }
    }
    exit(0);
}
# If goto_lossy
#  - verify disk file matches
#  - verify compressed version && create if not found
#  - swap nebulous keys
#  - update gpc1 database with bytes/md5sum and set state
#  - remove original version

if ($state eq 'goto_lossy') {
    # Confirm we have both files
    &my_die("Couldn't find original file: $uri\n", $exp_id,$exp_name,$class_id,$uri,$state,$PS_EXIT_SYS_ERROR)
        unless($ipprc->file_exists($uri));
    my $uriReal = $ipprc->file_resolve( $uri );

    my $compUri = $uri . ".fz";
    unless($ipprc->file_exists($compUri)) {
        &my_die("Couldn't find compressed version of the file: $compUri\n",
                $exp_id,$exp_name,$class_id,$uri,$state,$PS_EXIT_SYS_ERROR);

        # If that die is removed, this will compress things as well.
        #
#       &my_die("Output compressed file already exists: $compUri\n",
#               $exp_id,$exp_name,$class_id,$uri,$PS_EXIT_SYS_ERROR)
#           if ($ipprc->file_exists($compUri));
#       my $compReal= $ipprc->file_resolve( $compUri, 'create');

#       my $compress_command = "$fpack -h -s 8 -S $uriReal >  $compReal";
#       print STDERR "$compReal $uriReal $compress_command\n";
#       my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) =
#           run(command => $compress_command, verbose => $verbose);
#       unless ($success) {
#           $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
#           &my_die("Unable to compress file: $uri -> $compUri: $error_code",
#                   $exp_id,$exp_name,$class_id,$uri, $PS_EXIT_SYS_ERROR);
#       }
    }

    # Swap keys
    my $compReal = $ipprc->file_resolve( $compUri );
    my ($new_bytes, $new_md5) = (-s $compReal,file_md5_hex($compReal));
    my $neb = $ipprc->nebulous();

    unless ($no_op) {
        $neb->replicate($compUri);
        $neb->swap($uri,$compUri) or
            &my_die("Nebulous swap failed between $uri and $compUri",
                    $exp_id,$exp_name,$class_id,$uri,$state,$PS_EXIT_SYS_ERROR);
    }
    # Update database

    my $database_command = "$regtool -updateprocessedimfile -exp_id $exp_id -class_id $class_id -set_state lossy ";
    $database_command .= " -set_bytes $new_bytes -set_md5sum $new_md5 ";

    $database_command .= " -dbname $dbname" if defined $dbname;

    if ($no_update || $no_op) {
        print STDERR "NOUPDATE: $database_command\n";
    }
    else {
        my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) =
            run(command => $database_command, verbose => $verbose);
        unless($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            $neb->swap($uri,$compUri) or
                &my_die("DB update failed and Nebulous swap-back failed between $uri and $compUri",
                        $exp_id,$exp_name,$class_id,$uri,$state,$PS_EXIT_SYS_ERROR);
            &my_die("Unable to update database file: $uri -> $compUri: $error_code",
                    $exp_id,$exp_name,$class_id,$uri, $state,$PS_EXIT_SYS_ERROR);
        }
        # remove original version
    $neb->delete($compUri);
    }
    exit(0);
}

die "lossy_compress_imfile.pl -state $state not recognized\n";



sub my_die {
    my $msg = shift; # Warning message on die
    my $exp_id = shift; # exp_id
    my $exp_name = shift; # exp_name
    my $class_id = shift; # Class identifier
    my $uri = shift; # uri for the file
    my $state = shift;
    my $exit_code = shift; # Exit code
    # outputImage and path_base are globals
    my $fail_state = $state;
    $fail_state =~ s/goto_/error_/;
    my $error_command = "$regtool -updateprocessedimfile -exp_id $exp_id -class_id $class_id -set_state $fail_state ";
    $error_command .= " -dbname $dbname" if defined $dbname;
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf) =
	run(command => $error_command, verbose => $verbose);
    unless($success) {
	warn("Failed at setting the error state too.");
    }

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    exit $exit_code;
}
