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


use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Parse the command-line arguments
my ($dist_id, $stage, $stage_id, $outdir, $clean, $camera);
my ($dbname, $save_temps, $verbose, $no_update, $no_op, $logfile);

GetOptions(
           'dist_id=s'      => \$dist_id,# Magic destreak run identifier
           'camera=s'       => \$camera,
           'stage=s'        => \$stage,      # raw, chip, warp, or diff
           'stage_id=s'     => \$stage_id,   # exp_id, chip_id, warp_id, or diff_id
           'clean'          => \$clean,      # exporting a clean run
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

my $ipprc = PS::IPP::Config->new($camera); # IPP configuration

$ipprc->redirect_output($logfile) if $logfile;

# Look for programs we need
my $missing_tools;
my $disttool   = can_run('disttool') or (warn "Can't find disttool" and $missing_tools = 1);
my $regtool   = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
my $chiptool   = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $camtool   = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $faketool   = can_run('faketool') or (warn "Can't find faketool" and $missing_tools = 1);
my $warptool   = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $difftool   = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $stacktool   = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $bgtool = can_run('bgtool') or (warn "Can't find bgtool" and $missing_tools = 1);
my $fftool = can_run('fftool') or (warn "Can't find fftool" and $missing_tools = 1);
my $staticskytool = can_run('staticskytool') or (warn "Can't find staticskytool" and $missing_tools = 1);
if ($missing_tools) {
    &my_die("Can't find required tools.", $dist_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new;

my $tool_cmd;
my $tool_cmd2;
my $list_mode;
my $component_key;
if ($stage eq "raw") {
    $tool_cmd = "$regtool -exp_id";
    $list_mode = "-processedimfile";
    $component_key = "class_id";
} elsif ($stage eq "chip") {
    $tool_cmd = "$chiptool -chip_id";
    $list_mode = "-processedimfile";
    $component_key = "class_id";
} elsif ($stage eq "chip_bg") {
    $tool_cmd = "$bgtool -chip_bg_id";
    $list_mode = "-chip";
    $component_key = "class_id";
} elsif ($stage eq "camera") {
    $tool_cmd = "$camtool -cam_id";
    $list_mode = "-processedexp";
    $component_key = "";
} elsif ($stage eq "fake") {
    $tool_cmd = "$faketool -fake_id";
    $list_mode = "-processedimfile";
    $component_key = "class_id";
} elsif ($stage eq "warp") {
    $tool_cmd = "$warptool -warp_id";
    $list_mode = "-warped";
    $component_key = "skycell_id";
} elsif ($stage eq "warp_bg") {
    $tool_cmd = "$bgtool -warp_bg_id";
    $list_mode = "-warp";
    $component_key = "skycell_id";
} elsif ($stage eq "stack") {
    $tool_cmd = "$stacktool -stack_id";
    $list_mode = "-sumskyfile";
    $component_key = "skycell_id";
} elsif ($stage eq "sky") {
    $tool_cmd = "$staticskytool -sky_id";
    $list_mode = "-result";
    $component_key = "skycell_id";
} elsif ($stage eq "skycal") {
    $tool_cmd = "$staticskytool -skycal_id";
    $list_mode = "-skycalresult";
    $component_key = "skycell_id";
} elsif ($stage eq "diff") {
    $tool_cmd = "$difftool -diff_id";
    $list_mode = "-diffskyfile";
    $component_key = "skycell_id";
} elsif ($stage eq "SSdiff") {
    $tool_cmd = "$difftool -diff_id";
    $list_mode = "-diffskyfile";
    $component_key = "skycell_id";
} elsif ($stage eq "ff") {
    $tool_cmd = "$fftool -ff_id";
    $list_mode = "-result";
    $component_key = "dist_component";
    $tool_cmd2 = "$fftool -summary -ff_id"
} else {
    &my_die("Unexpected stage: $stage", $dist_id, $PS_EXIT_CONFIG_ERROR);
}

$tool_cmd .= " $stage_id";
$tool_cmd2 .= " $stage_id" if $tool_cmd2;

my $exportarg = '-exportrun';
if ($stage eq 'chip_bg') {
    $exportarg = '-exportchip';
} elsif ($stage eq 'warp_bg') {
    $exportarg = '-exportwarp';
} elsif ($stage eq 'skycal') {
    $exportarg = '-exportskycalrun';
}


# work around the fact that $ipprc->file_create does not actually create a file on disk
# unless the scheme is nebulous
sub create_file {
    my $rule = shift;
    my $path_base = shift;
    my $ref = shift;

    my $file;

    my $error;
    $file = $ipprc->prepare_output($rule, $path_base, undef, 1, \$error)
        or &my_die("Unable to prepare outut for $rule", $dist_id, $PS_EXIT_SYS_ERROR);

    my $scheme = file_scheme($file);
    $scheme = "" if !$scheme;

    my $resolved;
    if ($scheme) {
        $ipprc->file_create($file) 
            or &my_die("Unable to create $file", $dist_id, $PS_EXIT_SYS_ERROR);

        $resolved = $ipprc->file_resolve($file) 
            or &my_die("Unable to resolve $file", $dist_id, $PS_EXIT_SYS_ERROR);

        if ($scheme eq 'neb') {
            &my_die("$resolved not found", $dist_id, $PS_EXIT_SYS_ERROR) unless ($resolved and -e $resolved);
    }
    } else {
        $resolved = $file;
    }
    $$ref = $resolved;

    return $file
}

my $dbinfo_root = "$outdir/dbinfo.$stage.$stage_id";
my $resolved;
my $dbinfo_file = create_file("DIST.OUTPUT.DBINFO", $dbinfo_root, \$resolved);

{
    my $command = "$tool_cmd $exportarg -outfile $resolved";
    $command .= " -clean" if ((defined $clean) and ($stage ne "raw"));
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
    }
}

my $dirinfo_root = "$outdir/dirinfo.$stage.$stage_id";
my $dirinfo = create_file("DIST.OUTPUT.DIRINFO", $dirinfo_root, \$resolved);

{
    my $command = "$tool_cmd $list_mode";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
    }
    if (@$stdout_buf == 0) {
        &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $dist_id, $PS_EXIT_UNKNOWN_ERROR);
    my $components = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $dist_id, $PS_EXIT_UNKNOWN_ERROR);

    if ($tool_cmd2) {
        my $command = "$tool_cmd2";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
            unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
        }
        if (@$stdout_buf == 0) {
            &my_die("Unable to perform $command: $error_code", $dist_id, $error_code);
        }
        my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", $dist_id, $PS_EXIT_UNKNOWN_ERROR);
        my $more_components = parse_md_list($metadata) or
            &my_die("Unable to parse metadata list", $dist_id, $PS_EXIT_UNKNOWN_ERROR);
        if (scalar @$more_components) {
            push @$components, @$more_components;
        }
    }

    open MANIFEST, ">$resolved" or
        &my_die("Unable to open dirinfo file $resolved",  $dist_id, $PS_EXIT_UNKNOWN_ERROR);

    my $destdir;
    foreach my $c (@$components) {
        # take the workdir from the first component
        if (!$destdir) {
            my $workdir = $c->{workdir};
            if ($workdir) {
                $destdir = stripvolume($workdir, $stage);
            } elsif ($stage eq 'raw') {
                $destdir = 'none';
            } else {
                &my_die("workdir not found for open dirinfo file $dirinfo",  $dist_id, $PS_EXIT_UNKNOWN_ERROR);
            }
            print MANIFEST "destdir METADATA\n";
            print MANIFEST "\t" , "destdir", "\tSTR\t", $destdir, "\n";
            print MANIFEST "END\n\n";
            print MANIFEST "components METADATA\n";
        }
        my $component = $c->{$component_key} ? $c->{$component_key} : "exposure";
        my $path;
        if ($stage eq 'raw') {
            $path = $c->{uri};
        } else {
            $path = $c->{path_base};
        }
        &my_die("unable to find path",  $dist_id, $PS_EXIT_UNKNOWN_ERROR) if !$path;
        my $component_dir = find_componentdir($destdir, $path);
        print MANIFEST "\t" , "$component", "\tSTR\t", $component_dir, "\n";
    }
    print MANIFEST "END\n\n";
    close MANIFEST or
        &my_die("Unable to close dirinfo file $dirinfo",  $dist_id, $PS_EXIT_UNKNOWN_ERROR);
}

{
    my $command = "$disttool -updaterun -dist_id $dist_id -set_state full";
    $command .= " -set_outdir $outdir";
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

sub stripvolume
{
    my $path = shift;
    my $stage = shift;
    my @segments;

    # workdir isn't what we want for raw stage
    return "none" if ($stage and ($stage eq 'raw'));

    my $scheme = file_scheme($path);
    my $tail;
    if ($scheme) {
        # strip off scheme://
        $tail = substr($path, length($scheme) + 3);
    } elsif (substr($path, 0, 1) eq '/') {
        $tail = substr($path, 1);
        $scheme = "";
    }
    # remove any leading / that are left
    while ((substr($tail, 0, 1) eq '/')) {
        $tail = substr($tail, 1);
    }

    if (($scheme eq 'neb') or ($scheme eq 'path')) {
        my $volume;
        ($volume, @segments) = split '/', $tail;

    } elsif (!$scheme or ($scheme eq 'file')) {

        # XXX Here we're assuming the /data/ipp??? structure. This won't be true when data is forwarded
        # by remote sites. We need a way to configure this
        my $volume;

        # data/ippxxx/dirs
        (undef, $volume, @segments) = split '/', $tail;
    } else {
        die( "unexpected workdir value: $path\n");
    }

    return caturi(@segments);
}

sub find_componentdir
{
    my $destdir = shift;
    my $path = shift;

    my $result;
    if ($destdir eq 'none') {
        $result = stripvolume($path);
    } else {
        # find location of destdir in the path
        my $i = index($path, $destdir);

        $result = substr($path, $i + length($destdir) + 1);

        while (substr($result, 0, 1) eq '/') {
            $result = substr($result, 1);
        }
    }
    return dirname($result);
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $dist_id = shift;    # Magic DS identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

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
