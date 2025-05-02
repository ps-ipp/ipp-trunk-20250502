#!/usr/bin/env perl

use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

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

# Look for programs we need
my $missing_tools;
my $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $ppSub = can_run('ppSub') or (warn "Can't find ppSub" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $nebInsert = can_run('neb-insert') or (warn "Can't find neb-insert" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($diff_id, $dbname, $threads, $outroot, $reduction, $inverse, $run_state, $verbose, $no_update, $no_op, $redirect);
my ($skycell_id, $diff_skyfile_id);
my ($use_convolved);

my @ARGS = @ARGV;

GetOptions(
    'diff_id=s'         => \$diff_id, # Diff identifier
    'skycell_id=s'      => \$skycell_id, # Skycell identifier
    'diff_skyfile_id=s' => \$diff_skyfile_id, # Diff identifier
    'dbname|d=s'        => \$dbname, # Database name
    'threads=s'         => \$threads,   # Number of threads to use
    'run-state=s'       => \$run_state,   # state for run: 'new' or 'update'
    'outroot=s'         => \$outroot, # Output root name
    'inverse'           => \$inverse, # Make inverse subtraction?
    'reduction=s'       => \$reduction, # Reduction class
    'use_convolved'     => \$use_convolved, # Use convolved stacks instead of unconvolved.
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --diff_id --skycell_id --outroot --run-state --diff_skyfile_id",
    -exitval => 3,
          ) unless defined $diff_id
    and defined $skycell_id
    and defined $diff_skyfile_id
    and defined $run_state
    and defined $outroot;

my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $diff_id, $skycell_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $neb;
my $scheme = file_scheme($outroot);
if ($scheme and $scheme eq 'neb') {
    $neb = $ipprc->nebulous();
}

# XXX camera is not known here; cannot use filerules... 
my $logDest = "$outroot.log";
my $updateMode = 0;
if ($run_state eq 'update') {
    $logDest .= '.update';
    $updateMode = 1;
}
$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR ) if $redirect;
print "FULL COMMAND: $0 @ARGS\n\n";

my $source_id = $ipprc->source_id($dbname, $PS_TABLE_ID_DIFF);

# Get list of components for subtraction
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $files;
{
    my $command = "$difftool -inputskyfile -diff_id $diff_id -skycell_id $skycell_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform difftool -inputskyfile: $error_code", $diff_id, $skycell_id, $error_code);
    }

    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $diff_id, $skycell_id, $PS_EXIT_PROG_ERROR);
    $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $diff_id, $skycell_id, $PS_EXIT_PROG_ERROR);
}

&my_die("Subtraction list does not contain exactly two elements", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless scalar @$files == 2;

# Identify the input and the template
my ($input, $inputMask, $inputVariance, $inputPath, $inputSources); # Input files and path
my ($template, $templateMask, $templateVariance, $templatePath, $templateSources); # Template files and path
my $tess_id;                    # Tesselation identifier
my $camera;                     # Camera
my ($inputMagic, $templateMagic); # Are the inputs been magicked?
my ($saveInConv, $saveRefConv);   # Save the input or reference convolved images?
# CZW 2013-03-21: We only want to use unconvolved inputs.  Ever, if I understand correctly.
unless ($use_convolved) {
    $use_convolved = 0;
}

# # Prescan to decide if this is or is not a stack stack diff. The check above confirms we only have two entries.
# if ((${ $files }[0]->{warp_id} != 0)&&
#     (${ $files }[1]->{warp_id} != 0)) {
#     # Both are zero, so stack stack diff;
# }
# else {

#     # We're in some sort of warp stack or warp warp (don't care about the last one.)
#     $use_convolved = 1; ## This is a hack to do a test for mops, and should not be committed.
# }

# Re-implement prescan to decide if this is a warp-stack or warp-warp.  Or stack-stack, but that's what we don't care about now.
my $diff_type = '';

if ((${ $files }[0]->{warp_id} != 0)&&(${ $files }[1]->{warp_id} != 0)) {
    $diff_type = "WW";
}
elsif ((${ $files }[0]->{stack_id} != 0)&&(${ $files }[1]->{stack_id} != 0)) {
    $diff_type = "SS";
}
elsif (((${ $files }[0]->{warp_id} != 0)&&(${ $files }[1]->{stack_id} != 0))) {
    $diff_type = "WS";
}
else {  # Something has gone horribly wrong.
    $diff_type = "WW"; # just pretend it never happened, and let it fail elsewhere.
}

# ppSub does (input) - (template) after PSF-match convolution.  
# one of the files needs to be assigned to 'input' and the other to 'template'.
# if either of these is a WARP, then we can perform magic on it.  If we are going to 
# run magic, then we need to save the convolved version of the OTHER image
# (see magic_process.pl:155)

foreach my $file (@$files) {
    if (defined $file->{template} and $file->{template}) {
        $templatePath = $file->{path_base};
        if ($file->{warp_id} == 0) {
            if ($use_convolved) {
                $template = "PPSTACK.OUTPUT";
                $templateMask = "PPSTACK.OUTPUT.MASK";
                $templateVariance = "PPSTACK.OUTPUT.VARIANCE";
            }
            else {
                $template     = "PPSTACK.UNCONV";
                $templateMask = "PPSTACK.UNCONV.MASK";
                $templateVariance = "PPSTACK.UNCONV.VARIANCE";
            }
            $templateSources = "PSPHOT.OUT.CMF.MEF";  ## this must be consistent with the value in stack_skycell.pl
            # template is a stack so it doesn't need to be magicked
            $templateMagic = 1;
            ## use an explicit stack name for psphot output objects
        } else {
            $template     = "PSWARP.OUTPUT";
            $templateMask = "PSWARP.OUTPUT.MASK";
            $templateVariance = "PSWARP.OUTPUT.VARIANCE";
            $templateSources = "PSWARP.OUTPUT.SOURCES";
            $templateMagic = $file->{magicked};
            $saveInConv = 1;
        }
    } else {
        $inputPath = $file->{path_base};
        $inputMagic = $file->{magicked};    # if input is a stack the output can't be "magicked"
        if ($file->{warp_id} == 0) {
            if ($use_convolved) {
                $input = "PPSTACK.OUTPUT";
                $inputMask = "PPSTACK.OUTPUT.MASK";
                $inputVariance = "PPSTACK.OUTPUT.VARIANCE";
            }
            else {
                $input     = "PPSTACK.UNCONV";
                $inputMask = "PPSTACK.UNCONV.MASK";
                $inputVariance = "PPSTACK.UNCONV.VARIANCE";
            }
            $inputSources = "PSPHOT.OUT.CMF.MEF";  ## this must be consistent with the value in stack_skycell.pl
        } else {
            $input     = "PSWARP.OUTPUT";
            $inputMask = "PSWARP.OUTPUT.MASK";
            $inputVariance = "PSWARP.OUTPUT.VARIANCE";
            $inputSources = "PSWARP.OUTPUT.SOURCES";
            $saveRefConv = 1;
        }
    }
    if (defined $tess_id) {
        &my_die("Tesselation identifiers don't match", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless
            $file->{tess_id} eq $tess_id;
    } else {
        $tess_id = $file->{tess_id};
    }
    &my_die("Skycell identifiers don't match", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless
        $file->{skycell_id} eq $skycell_id;
    if (defined $camera) {
        &my_die("Cameras don't match", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $file->{camera} eq $camera;
    } else {
        $camera = $file->{camera};
    }
}

## EAM 2015.03.12 : for the PV3 3pi diffs, we do NOT want to save these images.  here i am hacking this value to 0, but it should depend on the
## values in filerules or recipe...
$saveInConv  = 0;
$saveRefConv = 0;
## MEH adding above from ipptest:ipp-20150312 hack.. 
## -- of course this wont work because of the poor code doing a defined check rather than value check.. so fix...
undef $saveInConv;
undef $saveRefConv;

&my_die("Unable to identify template", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless defined $template;
&my_die("Unable to identify input", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless defined $input;
&my_die("Unable to identify camera", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless defined $camera;
$ipprc->define_camera($camera);

# Compute the magicked status of the output.
# The output file will be considered magicked if the input has been magicked and the
# template is either a stack or a warp that has been magicked.
my $magicked = $inputMagic && $templateMagic ? $inputMagic : 0;

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_ppSub = $ipprc->reduction($reduction, 'DIFF_PPSUB'); # Recipe to use for ppSub
my $recipe_psphot  = $ipprc->reduction($reduction, 'DIFF_PSPHOT'); # Recipe to use for psphot
my $recipe_ppstats = 'DIFFSTATS';
unless ($recipe_ppSub and $recipe_psphot) {
    &my_die("Couldn't find selected reduction for DIFF_PPSUB and DIFF_PSPHOT: $reduction\n", $diff_id, $skycell_id, $PS_EXIT_CONFIG_ERROR);
}

print "reduction: $reduction\n";
print "recipe_ppSub: $recipe_ppSub\n";

# print "templateMask: $templateMask\n";
# print "templatePath: $templatePath\n";
# print "inputMask: $inputMask\n";
# print "inputPath: $inputPath\n";
# print "templateVariance: $templateVariance\n";
# print "inputVariance: $inputVariance\n";
# print "templateSources: $templateSources\n";
# print "inputSources: $inputSources\n";

$input     = $ipprc->filename($input, $inputPath);
$inputMask = $ipprc->filename($inputMask, $inputPath);
$inputVariance = $ipprc->filename($inputVariance, $inputPath);
$inputSources = $ipprc->filename($inputSources, $inputPath);

$template     = $ipprc->filename($template, $templatePath);
$templateMask = $ipprc->filename($templateMask, $templatePath);
$templateVariance = $ipprc->filename($templateVariance, $templatePath);
$templateSources = $ipprc->filename($templateSources, $templatePath);

print "template:     $template\n";
print "templateMask: $templateMask\n";
print "templatePath: $templatePath\n";
print "input:     $input\n";
print "inputMask: $inputMask\n";
print "inputPath: $inputPath\n";
print "templateVariance: $templateVariance\n";
print "inputVariance: $inputVariance\n";
print "templateSources: $templateSources\n";
print "inputSources: $inputSources\n";

&my_die("Couldn't find input: $template", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($template);
&my_die("Couldn't find input: $templateMask", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($templateMask);
&my_die("Couldn't find input: $templateVariance", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($templateVariance);
#&my_die("Couldn't find input: $templateSources", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($templateSources);

## MEH -- needs to be for both kinds of diffs using stacks...
if ($diff_type eq 'WS' || $diff_type eq 'SS') {
    # LANL processed stack images do not have a set of sources transferred back, so we need to choose a different source list.
    unless($ipprc->file_exists($templateSources)) {
	if ($ipprc->file_exists($inputSources)) {
	    $templateSources = $inputSources;
	    print "CHANGED:templateSources: $templateSources\n";
	}
    }
    &my_die("Couldn't find input: $templateSources", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($templateSources);
}
else {
    &my_die("Couldn't find input: $templateSources", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($templateSources);
}
&my_die("Couldn't find input: $input", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($input);
&my_die("Couldn't find input: $inputMask", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputMask);
&my_die("Couldn't find input: $inputVariance", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputVariance);
&my_die("Couldn't find input: $inputSources", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($inputSources);

# Get the output filenames
my $configuration;
my $outputStats;
my $traceDest;
my $do_photom = 1;
my $dump_config = 1; 
if ($reduction eq 'NOCONVDIFF') {
    $do_photom = 0;
}
if ($run_state eq 'new') {
    $configuration = prepare_output("PPSUB.CONFIG", $outroot, 1);
    $traceDest = prepare_output("TRACE.EXP", $outroot, 1);
    $outputStats = prepare_output("SKYCELL.STATS", $outroot, 1);
} else {
    $do_photom = 0;
    $dump_config = 0;
    $traceDest = prepare_output("TRACE.EXP.UPDATE", $outroot, 1);
    # we rerun stats because we need the quality value
    $outputStats = prepare_output("SKYCELL.STATS.UPDATE", $outroot, 1);
    $configuration = $ipprc->filename("PPSUB.CONFIG", $outroot);
    # XXX: Work around problem with updating old diff runs. Their config files are incompatible
    # with the current code.
    if ($camera ne 'GPC1' or $diff_id >= 88268) {
        if (!$ipprc->file_exists($configuration)) {
	    print STDERR "WARNING: Config dump file $configuration is missing. Using current recipes and file rules.\n";
	    $configuration = undef;
	}
    } else {
    	print STDERR "WARNING: Using new recipes because config dump file is too old\n";
    	$configuration = undef;
    }

    # use WARPSTATS if we're updating, as we don't care about the new stuff from the STACK and DIFF STATS recipes.
    # The only value that we use is -quality
    $recipe_ppstats = 'WARPSTATS';
}

my $outputName = prepare_output("PPSUB.OUTPUT", $outroot, 1);
my $outputMask = prepare_output("PPSUB.OUTPUT.MASK", $outroot, 1);
my $outputVariance = prepare_output("PPSUB.OUTPUT.VARIANCE", $outroot, 1);
my $outputSources = prepare_output("PPSUB.OUTPUT.SOURCES", $outroot, 1) if $do_photom;
my $jpeg1Name = prepare_output("PPSUB.OUTPUT.JPEG1", $outroot, 1);
my $jpeg2Name = prepare_output("PPSUB.OUTPUT.JPEG2", $outroot, 1);
my $outputKernel = prepare_output("PPSUB.OUTPUT.KERNELS", $outroot, 0);
my $outputPsf = prepare_output("PSPHOT.PSF.SKY.SAVE", $outroot, 0);

# do we always need to prepare this file?
if ($reduction ne 'NOCONVDIFF') {
    my $refConv = prepare_output("PPSUB.REF.CONV", $outroot, 1);
}

my ($inverseName, $inverseMask, $inverseVariance, $inverseSources);
if ($inverse) {
    $inverseName = prepare_output("PPSUB.INVERSE", $outroot, 1);
    $inverseMask = prepare_output("PPSUB.INVERSE.MASK", $outroot, 1);
    $inverseVariance = prepare_output("PPSUB.INVERSE.VARIANCE", $outroot, 1);
    $inverseSources = prepare_output("PPSUB.INVERSE.SOURCES", $outroot, 1) if $do_photom;
}

my $cmdflags;

# Perform subtraction
{
    my $command = "$ppSub $outroot";
    $command .= " -updatemode" if $updateMode;
    $command .= " -inimage $input";
    $command .= " -refimage $template";
    $command .= " -inmask $inputMask";
    $command .= " -refmask $templateMask";
    $command .= " -invariance $inputVariance";
    $command .= " -refvariance $templateVariance";
    $command .= " -insources $inputSources";
    $command .= " -refsources $templateSources";
    $command .= " -stats $outputStats";
    $command .= " -threads $threads" if defined $threads;
    if ($dump_config) {
        $command .= " -dumpconfig $configuration";
    } elsif ($configuration) {
        $command .= " -ipprc $configuration";
    }
    $command .= " -save-inconv" if defined $saveInConv and $run_state eq "new";
    $command .= " -save-refconv" if defined $saveRefConv and $run_state eq "new";
    $command .= " -recipe PPSUB $recipe_ppSub";
    $command .= " -recipe PSPHOT $recipe_psphot";
    $command .= " -recipe PPSTATS $recipe_ppstats";
    $command .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE";
    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    $command .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF";
    if ($do_photom) {
        $command .= " -photometry";
    } else {
        $command .= " -Db PHOTOMETRY FALSE";
    }
    $command .= " -inverse" if $inverse;
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -image_id $diff_skyfile_id" if defined $diff_skyfile_id;
    $command .= " -source_id $source_id" if defined $source_id;

    unless ($no_op) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppSub: $error_code", $diff_id, $skycell_id, $error_code);
        }

        check_output($outputStats, 1);
        my $outputStatsReal = $ipprc->file_resolve($outputStats);

        # measure chip stats
        $command = "$ppStatsFromMetadata $outputStatsReal - DIFF_SKYCELL";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppStatsFromMetadata: $error_code", $diff_id, $skycell_id, $error_code);
        }
        foreach my $line (@$stdout_buf) {
            $cmdflags .= " $line";
        }
        chomp $cmdflags;

        my ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag

        if (!$quality) {
            check_output($outputName, 0);
            check_output($outputMask, 0);
            check_output($outputVariance, 0);
            check_output($outputSources, 1) if $do_photom;
            check_output($jpeg1Name, 1);
            check_output($jpeg2Name, 1);
            check_output($outputKernel, 1);
            check_output($outputPsf, 1);
            if ($inverse) {
                check_output($inverseName, 0);
                check_output($inverseMask, 0);
                check_output($inverseVariance, 0);
                check_output($inverseSources, 1) if $do_photom;
                if ($run_state eq 'new') {
                    check_output($configuration, 1);
                }
            }
	    if ($reduction eq 'NOCONVDIFF') {
		my $refConv = prepare_output("PPSUB.REF.CONV", $outroot, 0);
		my $templateFile = $ipprc->file_resolve($template,0);
		$command = "$nebInsert $refConv $templateFile";
		( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		    run(command => $command, verbose => $verbose);
		unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		    &my_die("Unable to perform neb-insert: $error_code", $diff_id, $skycell_id, $error_code);
		}
	    }


        } elsif ($run_state eq 'update') {
            &my_die("Update resulted in poor quality image: $quality", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR);
        }
    } else {
        print "Not executing: $command\n";
    }
}

unless ($no_update) {

    # Add the subtraction result
    {
        my $command = "$difftool -diff_id $diff_id -skycell_id $skycell_id";
        $command .= " -magicked $magicked" if $magicked;
        if ($run_state eq 'new') {
            $command .= " -adddiffskyfile -path_base $outroot";
            $command .= " $cmdflags";
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
            $command .= " -hostname $host" if defined $host;
        } else {
            $command .= " -tofullskyfile";
        }
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            my $err_message = $run_state eq "update" ?
                "Unable to perform difftool -adddiffskyfile" :
                "Unable to perform difftool -tofullskyfile";
            &my_die("$err_message: $error_code", $diff_id, $skycell_id, $error_code);
        }
    }
}
exit 0;


# Prepare to write to an output file
#   Lookup the filename in the rules.
#   Make sure that if file exists and is a nebulous file that there is only one instance
#   Deal with files that have been lost.
sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, undef, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $diff_id, $skycell_id, $error);
    return $output;
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;

    if (!defined $file) {
        return;
    }

    &my_die("Couldn't find expected output file: $file",  $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($file);

    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file", $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR);
	}
    }
    #####


    if ($replicate and $neb) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $diff_id, $skycell_id, $PS_EXIT_SYS_ERROR);
    }
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $diff_id = shift;        # Diff identifier
    my $skycell_id = shift;     # Skycell identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn($msg);
    if (defined $diff_id and defined $skycell_id and not $no_update) {
        my $command = "$difftool -diff_id $diff_id -skycell_id $skycell_id -fault $exit_code";
        if ($run_state eq 'new') {
            $command .= " -adddiffskyfile";
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
            $command .= " -hostname $host" if defined $host;
            $command .= " -path_base $outroot" if defined $outroot;
        } else {
            $command .= " -updatediffskyfile";
        }
        $command .= " -dbname $dbname" if defined $dbname;
        run(command => $command, verbose => $verbose);
    }
    exit $exit_code;
}

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}

__END__
