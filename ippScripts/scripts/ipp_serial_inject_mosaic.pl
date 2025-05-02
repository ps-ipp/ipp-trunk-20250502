#!/usr/bin/env perl

# this program injects a set of multi-file exposures into the db.  the
# program takes a list of base exposure names and injects all files
# associated with the exposure.  It constructs the expected filenames
# from the exposure tag and rules for the camera.  This program is for
# the test only since it requires too much information at the inject
# stage.  use 'ipp_serial_inject.pl' for single-file images and
# 'ipp_serial_inject_split.pl' for multiple file images in split
# format

# this program should not fail because of the data format or the
# configuration, except for the very basic database setup.

use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Config qw( caturi );
use Data::Dumper;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ($camera,                    # Camera used
    $telescope,                 # Telescope used
    $tess_id,                   # Tessellation identifier
    $dbname,                    # Database name
    $workdir,                   # Working directory
    $path,                      # Path to data
    );
GetOptions(
           'camera|c=s'    => \$camera,
           'telescope|t=s' => \$telescope,
           'workdir=s'     => \$workdir,
           'tess_id=s'     => \$tess_id,
           'path=s'        => \$path,
           'dbname=s'      => \$dbname,
) or pod2usage( 2 );

pod2usage(
          -msg => "Required options: --camera --telescope --workdir --path --dbname --tess_id",
          -exitval => 3,
          ) unless defined $camera
    and defined $telescope
    and defined $workdir
    and defined $path
    and defined $dbname
    and defined $tess_id;

my $ipprc = PS::IPP::Config->new(); # IPP configuration

# Look for programs we need
my $missing_tools;
my $pxinject = can_run('pxinject')  or (warn "Can't find pxinject" and $missing_tools = 1);

if (scalar @ARGV == 0) {
    die "No exposures provided.\n";
}

# Inject new data into the database
my @classes;                    # Names of the classes
my @files;                      # What to add to the filename for each class
my $imfiles;
my $add_dir;                    # Add directory name to get file name?
if ($camera eq "MEGACAM") {
    for (my $i = 0; $i < 36; $i++) {
        push @classes, sprintf("ccd%02d", $i);
        push @files, sprintf(".ccd%02d", $i);
    }
} elsif ($camera eq "MCSHORT") {
    @classes = ( 'ccd12', 'ccd13', 'ccd14', 'ccd21', 'ccd22', 'ccd23' );
    @files   = ( '.ccd12', '.ccd13', '.ccd14', '.ccd21', '.ccd22', '.ccd23' );
} elsif ($camera eq "CTIO_MOSAIC2") {
    @classes = ();
    @files = ();
} elsif ($camera eq "TC3") {
    @classes = ( 'CCID58-1-06b2', 'CCID45-1-14A', 'CCID45-1-11A', 'CCID45-1-22A',
                 'CCID45-1-04C', 'CCID45-1-13A', 'CCID45-1-05A', 'CCID45-1-19A' );
    @files = ( '00', '01', '10', '11', '20', '21', '30', '31' );
    $add_dir = 1;
} elsif ($camera eq "SIMMOSAIC") {
    @classes = ( 'Chip00', 'Chip01', 'Chip10', 'Chip11' );
    @files   = ( '.Chip00', '.Chip01', '.Chip10', '.Chip11' );
} elsif ($camera eq "SIMTEST") {
    @classes = ();
    @files = ();
} elsif ($camera eq "GPC1") {
    for (my $i = 0; $i < 8; $i++) {
        for (my $j = 0; $j < 8; $j++) {
            if (($i == 0 or $i == 7) and ($j == 0 or $j == 7)) {
                # Excluding corner chips
                next;
            }
            push @classes, "XY$i$j";
            push @files, "$i$j";
        }
    }
    $add_dir = 1;
} else {
    die "Unrecognised camera name: $camera.\nDid you mean to use ipp_serial_inject.pl?\n";
}

foreach my $exp_name ( @ARGV ) {
    my $command = "$pxinject -newExp -tmp_exp_name $exp_name -tmp_inst $camera -tmp_telescope $telescope -workdir $workdir -tess_id $tess_id"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to inject $exp_name: $error_code\n" if not $success;

    my @line = split(/\s+/, $$stdout_buf[0]); # The output line, containing the exposure tag
    my $exp_id = $line[2];      # The exposure tag
    for (my $i = 0; $i < scalar @classes; $i++) {
        my $class_id = $classes[$i];
        my $file_id = $files[$i];
        my $filename = $exp_name . $file_id . '.fits';
        $filename = caturi( $exp_name, $filename ) if defined $add_dir;
        $filename = caturi( $path, $filename );

        die "Unable to find file $filename" unless -f $ipprc->file_resolve( $filename );

        $filename = $ipprc->convert_filename_relative( $filename );
        my $command = "$pxinject -newImfile -exp_id $exp_id -tmp_class_id $class_id -uri $filename"; # Command to run
        $command .= " -dbname $dbname" if defined ($dbname);

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to inject $exp_name $class_id: $error_code\n" if not $success;
    }

    if (scalar @classes == 0) {
        my $filename = $exp_name . '.fits';
        $filename = caturi( $exp_name, $filename ) if defined $add_dir;
        $filename = caturi( $path, $filename );

        die "Unable to find file $filename" unless -f $ipprc->file_resolve( $filename );

        $filename = $ipprc->convert_filename_relative( $filename );
        my $command = "$pxinject -newImfile -exp_id $exp_id -tmp_class_id fpa -uri $filename"; # Command to run
        $command .= " -dbname $dbname" if defined ($dbname);
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to inject $exp_name imfile: $error_code\n" if not $success;
    }

    # Update the exposure to run
    {
        my $command = "$pxinject -updatenewExp -exp_id $exp_id -state run"; # Command to run
        $command .= " -dbname $dbname" if defined ($dbname);

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to activate $exp_name: $error_code\n" if not $success;
    }

}

END {
    my $status = $?;
system("sync") == 0
    or die "failed to execute sync: $!" ;
$? = $status;
}


__END__


