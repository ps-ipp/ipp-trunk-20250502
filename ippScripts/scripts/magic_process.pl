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
use File::Copy;
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use Astro::FITS::CFITSIO qw( :constants );
Astro::FITS::CFITSIO::PerlyUnpacking(1);

use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $magictool      = can_run('magictool') or (warn "Can't find magictool" and $missing_tools = 1);
my $difftool       = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $ppSubConvolve = can_run('ppSubConvolve') or (warn "Can't find ppSubConvolve" and $missing_tools = 1);
my $detectstreaks = can_run('DetectStreaks') or (warn "Can't find DetectStreaks" and $missing_tools = 1);
my $VerifyStreaks = can_run('VerifyStreaks') or (warn "Can't find VerifyStreaks, will not produce png images");
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($magic_id, $node, $camera, $dbname, $baseroot, $save_temps, $verbose, $no_update, $no_op, $logfile, $final_outroot);

GetOptions(
           'magic_id=s'      => \$magic_id,   # Magic identifier
           'node=s'          => \$node,       # Node name
           'camera=s'        => \$camera,     # Camera name
           'dbname=s'        => \$dbname,     # Database name
           'baseroot=s'      => \$baseroot,   # Output root name
           'final-outroot=s' => \$final_outroot,   # location for final outputs
           'save-temps'      => \$save_temps, # Save temporary files?
           'verbose'         => \$verbose,    # Print stuff?
           'no-update'       => \$no_update,  # Don't update the database?
           'no-op'           => \$no_op,      # Don't do any operations?
           'logfile=s'       => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --magic_id --camera --node --baseroot",
           -exitval => 3) unless
    defined $magic_id and
    defined $node and
    defined $camera and
    defined $baseroot;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $magic_id, $node, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $magic_id, $node, $PS_EXIT_SYS_ERROR ) if $logfile;

# DetectStreaks doesn't know about nebulous. It expects to be able to
# append strings to baseroot to form valid file names.  So forbid
# nebulous path in baseroot. We could relax this by change
# DetectStreaks to take all of the file names as arguments or by
# teaching it about Nebulous
if ($baseroot =~ 'neb:/') {
    &my_die("DetectStreaks does not support nebulous paths in outroot", $magic_id, $node, $PS_EXIT_CONFIG_ERROR);
}

# most filenames are of the form $baseroot.$node.*, but VerifyStreaks
# needs access to $baseroot.*, so we construct $outroot =
# $baseroot.$node in here

# resolve any path:// or file:// in outroot
$baseroot = $ipprc->file_resolve($baseroot);
my $outroot = "$baseroot.$node";
$ipprc->outroot_prepare($outroot);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# list of VerifyStreaks input and output files to copy to nebulous 
my %verify_outputs = (
'clusterPos.txt' => 0,
'duplicate.png' => 0,
'mask.png' => 0,
'original.png' => 0,
'original.fits' => 0,
'residual.png' => 0,
'residual.fits' => 0,
'clusters.list' => 1
);

### Get a list of inputs
my $inputs;                     # List of inputs
{
    my $command = "$magictool -inputs -magic_id $magic_id -node $node"; # Command to run
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform magictool -inputs: $error_code", $magic_id, $node, $error_code);
    }

    $inputs = $mdcParser->parse_list(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $magic_id, $node, $PS_EXIT_PROG_ERROR);
}


my @outputs;
my $inverse;                    # Using inverse diff?
### Do the heavy lifting
{
    my $command;                # Command to execute
    $command = "$detectstreaks --outroot $outroot";
    $command .= " --verbose" if $verbose;

    ### To enable debugging output:
    #    $command .= " --test";

    ### per email from Paul Sydney 2010.02.11, we should use threshold of 2.35 to catch the faint streak(s)
    $command .= " --threshold 2.35";

    my @deletions;          # Files to delete
    if (scalar @$inputs == 1 and $node ne "root") {
        #
        #  DetectStreaks --detect --image filename --mask maskname --weight weightname --outroot path_base
        #
        # Leaf node: 'detect' stage
        my $innode = $$inputs[0];     # Input node

        # expected outputs for detect stage
        @outputs = ("${outroot}.clusters", "${outroot}_hough.fits", "${outroot}.streaks");

        my $diff_id = $innode->{diff_id};
        if (!$diff_id) {
            &my_die("input for node has null diff_id", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);
        }
        my ($image, $mask, $weight) = resolve_inputs($innode);
        if (!defined($image) or !defined($mask) or !defined($weight)) {
            &my_die("failed to resolve inputs", $magic_id, $node, $PS_EXIT_DATA_ERROR);
        }

	# to run magic on a diff image, we need the convolved version
	# of the corresponding TEMPLATE image
	# (see diff_skycell.pl:124)

        my $diff_base = $innode->{diff_path_base}; # Base name for diff
        my $tempName = $innode->{inverse} ? "PPSUB.INPUT.CONV" : "PPSUB.REF.CONV"; # File rule of interest
        my $template = $ipprc->file_resolve($ipprc->filename($tempName, $diff_base));

        # Delete the convolved products when done (we can recreate them as we need)
        push @deletions, $ipprc->filename($tempName . ".MASK", $diff_base);
        push @deletions, $ipprc->filename($tempName . ".VARIANCE", $diff_base);

        unless (defined $template and $ipprc->file_exists($template)) {
            # Template doesn't exist (or can't be found); try to recreate it
            my $tempPath = "/tmp/magic.$magic_id.$node.template";

            my $kernel = $ipprc->filename("PPSUB.OUTPUT.KERNELS", $diff_base); # Name of kernel file
            &my_die("Unable to find kernel file $kernel", $magic_id, $node, $PS_EXIT_DATA_ERROR) unless $ipprc->file_exists($kernel);

            my ($image, $mask);   # Image and mask
            {
                my $command = "$difftool -inputskyfile -diff_id $diff_id -skycell_id $node"; # Command to run
                $command .= " -dbname $dbname" if defined $dbname;
                if ($innode->{inverse}) {
                    # Want the input because we're magicking the reference
                    $command .= " -input";
                } else {
                    # Want the reference because we're magicking the input
                    $command .= " -template";
                }

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to determine convolution inputs: $error_code", $magic_id, $node, $error_code);
                }

                my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
                    &my_die("Unable to parse metadata config doc", $magic_id, $node, $PS_EXIT_PROG_ERROR);

                my $inputs = parse_md_list($metadata) or
                    &my_die("Unable to parse metadata list", $magic_id, $node, $PS_EXIT_PROG_ERROR);
                &my_die("Unexpected number of outputs", $magic_id, $node, $PS_EXIT_PROG_ERROR) unless scalar @$inputs == 1;
                my $input = $$inputs[0];
                my $path = $input->{path_base}; # Path of interest
                if (defined $input->{warp_id} and $input->{warp_id} > 0) {
                    $image = $ipprc->filename("PSWARP.OUTPUT", $path);
                    $mask = $ipprc->filename("PSWARP.OUTPUT.MASK", $path);
                } elsif (defined $input->{stack_id} and $input->{stack_id} > 0) {
                    $image = $ipprc->filename("PPSTACK.UNCONV", $path);
                    $mask = $ipprc->filename("PPSTACK.UNCONV.MASK", $path);
                }
                &my_die("Unable to determine image and mask name", $magic_id, $node, $PS_EXIT_PROG_ERROR) unless defined $image and defined $mask;
            }

            {
                &my_die("Unable to find image and mask: $image $mask", $magic_id, $node, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($image) and $ipprc->file_exists($mask);

                my $command = "$ppSubConvolve $tempPath -image $image -mask $mask -kernel $kernel";
                $command .= " -reference" unless $innode->{inverse};
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run(command => $command, verbose => $verbose);
                unless ($success) {
                    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                    &my_die("Unable to create template image: $error_code", $magic_id, $node, $error_code);
                }

                $template = $ipprc->filename("PPSUB.INPUT.CONV", $tempPath) or &my_die("Unable to determine filename for created template", $magic_id, $node, $PS_EXIT_PROG_ERROR);
                $template = $ipprc->file_resolve($template) or &my_die("Unable to resolve filename for created template", $magic_id, $node, $PS_EXIT_PROG_ERROR);
                &my_die("Unable to find created template: $template", $magic_id, $node, $PS_EXIT_PROG_ERROR) unless $ipprc->file_exists($template);
            }
        }

        &my_die("Cannot find template", $magic_id, $node, $PS_EXIT_DATA_ERROR) unless defined $template and $ipprc->file_exists($template);
        push @deletions, $template;

        $command .= " --detect --image $image --mask $mask --weight $weight -k $template";

        # create the list of inputs used at this stage. At higher levels the
        # these files will get catenated together to create the file for the subsquent stage
        # this causes major file pollution, but avoids multi-level queries
        # at higher level nodes.

        my ($in_fh, $input_list)  = open_list_file($outroot, "input.list");
        print $in_fh "$outroot\n";
        close $in_fh;
        my ($ifh, $image_list)  = open_list_file($outroot, "image.list");
        print $ifh "$image\n";
        close $ifh;
        my ($mfh, $mask_list)   = open_list_file($outroot, "mask.list");
        print $mfh "$mask\n";
        close $mfh;
        my ($wfh, $weight_list) = open_list_file($outroot, "weight.list");
        print $wfh "$weight\n";
        close $wfh;
    } else {
        #
        # DetectStreaks --merge --inputs input.list --images image0_1.list \
        #                      --masks mask0_1.list --weight weights0_1.list
        #                      --outroot outroot

        # Branch node: 'merge' stage

        # expected outputs from merge stage
        @outputs = ("${outroot}.streaks");

        my ($in_fh, $input_list) = open_list_file($outroot, "input.list");
        my ($ifh, $image_list)   = open_list_file($outroot, "image.list");
        my ($mfh, $mask_list)    = open_list_file($outroot, "mask.list");
        my ($wfh, $weight_list)  = open_list_file($outroot, "weight.list");
        my ($sfh, $streaks_list) = open_list_file($outroot, "streaks.list");

        # do this in eval so we can fault the exposure without
        # passing the $magic_id and $node everywhere
        eval {
            foreach my $innode (@$inputs) {
                # root for inputs from previous stage
                my $in_path_base = $innode->{magic_path_base};
                print $sfh "$in_path_base\n";

                cat_list_to_list($in_fh, $in_path_base, "input.list");
                # build input lists by combining the lists from
                # previous stages
                cat_list_to_list($ifh, $in_path_base, "image.list");
                cat_list_to_list($mfh, $in_path_base, "mask.list");
                cat_list_to_list($wfh, $in_path_base, "weight.list");

                if ($innode->{inverse}) {
                    $inverse = 1;
                }
            }
            close $in_fh;
            close $ifh;
            close $mfh;
            close $wfh;
            close $sfh;

            $command .= " --merge --inputs $input_list";
            $command .= " --images $image_list --masks $mask_list --weights $weight_list" ;
            $command .= " --inputstreaks $streaks_list";
            # new option to "merge duplicate streaks into unique streaks"
            $command .= " --duplicates";
        };
        if ($@) {
            &my_die("failed to create file lists: $@", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);
        }
    }

    unless ($no_op) {
        # DetectStreaks fails if an output file already exists
        foreach my $output (@outputs) {
            unlink($output) if -e $output;
        }

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform DetectStreaks: $error_code", $magic_id, $node, $error_code);
        }

        foreach my $output (@outputs) {
            file_check( $output );
        }

        foreach my $file (@deletions) {
            print "Deleting $file...\n";
            $ipprc->file_delete($file);
        }

    } else {
        print "Skipping command: $command\n";
    }

}


if ($node eq "root") {
    # XXXX: Since we just added the result above, all of these my_dies are going to fail
    # with a duplicate row error.
    # see more comments below
    my $streaks_file = "$outroot.streaks";
    my $resolved = $ipprc->file_resolve($streaks_file);

    my $num_streaks = -1;
    unless ($no_op) {
        my $fh;
        open $fh, "<$resolved" or
            &my_die("failed to open streaks file $streaks_file", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);
        # the first line in the streaks file contains the number of streaks found
        $num_streaks = <$fh>;
        chomp $num_streaks;
        close $fh;
        print "$num_streaks streaks found on magicRun $magic_id\n" if $verbose;
    }


    my $exp_id;                 # Exposure identifier
    my $cam_path;               # Camera stage path_base
    {
        my $command = "magictool -exposure -magic_id $magic_id";
        $command .= " -inverse" if defined $inverse;
        $command .= " -dbname $dbname" if defined $dbname;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        my $exposures = $mdcParser->parse_list(join "", @$stdout_buf);
        my $exp = $$exposures[0]; # Exposure of interest (should only be one)
        if (!$exp) {
            &my_die("magictool -exposure returned no output", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);
        }

        $exp_id = $exp->{exp_id};
        $cam_path = $exp->{path_base};
    }

    &run_verifystreaks($baseroot, $exp_id);

    {
        my $astrom = $ipprc->filename("PSASTRO.OUTPUT", $cam_path); # Astrometry file
        my $streaks = "$outroot.streaks";                           # Streaks file
        my $clusters = "$baseroot.verify/${exp_id}_clusterPos.txt"; # Clusters file

        my $command = "ppCoord -astrom $astrom -streaks $streaks";
        if ($ipprc->file_exists($clusters)) {
            $command .= " -clusters $clusters";
        }

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        open my $coords, "> $outroot.coords";
        print $coords join("", @$stdout_buf);
        close $coords;
    }

    my $output_streaks = $final_outroot . ".streaks";
    if ($output_streaks and ($output_streaks ne $streaks_file)) {
        copy_to_nebulous($ipprc, $streaks_file, $output_streaks, 1);
        my $streaks_map = $outroot . ".streakMap";
        my $output_streaks_map = $final_outroot . ".streakMap";
        copy_to_nebulous($ipprc, $streaks_map, $output_streaks_map, 1);
        foreach my $f (keys %verify_outputs) {
            my $replicate = $verify_outputs{$f};
            my $src = "$baseroot.verify/${exp_id}_$f";
            my $dest = "$final_outroot.${f}";
            copy_to_nebulous($ipprc, $src, $dest, $replicate);
        }
    } else {
        $output_streaks = $streaks_file;
    }

    {
        my $command = "$magictool -addmask";
        $command   .= " -magic_id $magic_id";
        $command   .= " -path_base $final_outroot";
        $command   .= " -streaks $num_streaks";
        $command   .= " -dbname $dbname" if defined $dbname;

        # Add the processed file to the database
        unless ($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform magictool -addmask: $error_code", $magic_id, $node, $error_code);
            }
        } else {
            print "Skipping command: $command\n";
        }
    }
}

    ### Input result into database
{
    my $command = "$magictool -addresult";
    $command   .= " -magic_id $magic_id";
    $command   .= " -node $node";
    $command   .= " -path_base $outroot";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            # XXX: if this is the root node we need to revert delete the magicMask object
            # inserted above
            &my_die("Unable to perform magictool -addresult: $error_code", $magic_id, $node, $error_code);
        }
    } else {
        print "Skipping command: $command\n";
    }
}


### Pau.

sub run_verifystreaks {

    my $baseroot = shift;
    my $exp_id = shift;

    unless ($VerifyStreaks) {
        print STDERR "skipping VerifyStreaks\n";
        return 1;
    }

    # VerifyStreaks --out $outdir --clusters $outdir/clusters.list $rootname.root.streakMap

    my $outdir = "$baseroot.verify";

    my($status) = system ("mkdir -p $outdir");
    if ($status) {
        print STDERR "failed to create output directory $outdir\n";
        return 1;
    }

    my $FILE;

    my @files = <$baseroot.*.clusters>;

    my $clusters_list = "$outdir/${exp_id}_clusters.list";
    unless (open ($FILE, ">$clusters_list")) {
        print "failed to create cluster file $clusters_list\n";
        return 1;
    }
    foreach my $file (@files) {
        $file =~ s|.clusters$||;
        print $FILE "$file\n";
    }
    close ($FILE);
    if ($status) {
        print "failed to create cluster file $clusters_list\n";
        return 1;
    }

    my $command = "$VerifyStreaks --out $outdir --clusters $clusters_list $baseroot.root.streakMap";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        print "failed to run VerifyStreaks:\n";
        return 1;
    }

    return 0;
}

sub open_list_file {
    my $outroot = shift;
    my $extension = shift;

    my $filename = "$outroot.$extension";

    my $fh;
    open $fh, ">$filename" or die "failed to open list file $filename";

    return ($fh, $filename);
}

sub cat_list_to_list   {
    my $out = shift;        # output file handle
    my $path_base = shift;  # path_base to append ...
    my $extension = shift;  # ... the extension to

    my $filename = "$path_base.$extension";

    my $in;
    open $in, "<$filename" or die "failed to open list file: $filename";
    foreach my $line (<$in>) {
        print $out $line;
    }
}

sub resolve_inputs
{
    my $node = shift;
    my $input_base = $node->{diff_path_base};

    my ($image, $mask, $variance); # Names to return
    if ($node->{inverse}) {
        $image = "PPSUB.INVERSE";
        $mask = "PPSUB.INVERSE.MASK";
        $variance = "PPSUB.INVERSE.VARIANCE";
    } else {
        $image = "PPSUB.OUTPUT";
        $mask = "PPSUB.OUTPUT.MASK";
        $variance = "PPSUB.OUTPUT.VARIANCE";
    }

    $image = $ipprc->file_resolve($ipprc->filename($image, $input_base));
    $mask = $ipprc->file_resolve($ipprc->filename($mask, $input_base));
    $variance= $ipprc->file_resolve($ipprc->filename($variance, $input_base));

    return ($image, $mask, $variance);
}

sub file_check
{
    my $file = shift;           # Name of file
    &my_die("Unable to find output file: $file", $magic_id, $node, $PS_EXIT_SYS_ERROR) unless
        $ipprc->file_exists($file);
}

# Copy a file to nebulous and optionally replicate it
# We should consider making this an ipprc function. For now try it here so we can print
# the right error messages
sub copy_to_nebulous {
    my $ipprc = shift;
    my $src = shift;
    my $dest = shift;
    my $replicate = shift;

    print "copying $src to $dest\n";

    $ipprc->file_exists($src) or
        &my_die("expected output file does not exist: $src", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);

    # copy the file to it's final destination - which is presumably in nebulous
    # we delete it so that all instances are deleted in case it has been replicated
    if ($ipprc->file_exists($dest)) {
        $ipprc->file_delete($dest) or 
                &my_die("failed to delete existing file: $dest", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);
    }
    my $dest_resolved = $ipprc->file_resolve($dest, 'create');
    &my_die("failed to resolve $dest", $PS_EXIT_UNKNOWN_ERROR) if !$dest_resolved;

    copy ($src, $dest_resolved) or 
        &my_die("failed to copy $src to $dest", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);

    if ($replicate and (file_scheme($dest) eq 'neb')) {
        my $neb = $ipprc->nebulous();
        $neb->setxattr($dest, 'user.copies', 2, 'create') or 
            &my_die("failed to set user.copies for $dest", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);

        $neb->replicate($dest) or
            &my_die("failed to replicate $dest", $magic_id, $node, $PS_EXIT_UNKNOWN_ERROR);
    }
}

sub my_die
{
    my $msg = shift;            # Warning message on die
    my $magic_id = shift;       # Magic identifier
    my $node = shift;           # Node name
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);
    if (defined $magic_id and defined $node and not $no_update) {
        my $command = "$magictool -addresult";
        $command .= " -magic_id $magic_id";
        $command .= " -node $node";
        $command .= " -fault $exit_code";
        $command .= " -path_base $outroot" if defined $outroot;
        $command .= " -dbname $dbname" if defined $dbname;
        system($command);
    }
    exit $exit_code;
}

__END__
