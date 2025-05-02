#!/usr/bin/env perl

use warnings;
use strict;
use Carp;

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
use File::Temp qw( tempfile );
use File::Basename;
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $stacktool = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $ppStack = can_run('ppStack') or (warn "Can't find ppStack" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($stack_id, $dbname, $outroot, $debug, $run_state, $threads, $reduction, $verbose, $no_update, $no_op, $redirect, $save_temps, $delete_convolved_images);
GetOptions(
    'stack_id|d=s'      => \$stack_id, # Stack identifier
    ## XXX future addition: if stack gets multi-skyfile option 'stack_skyfile_id|d=s' => \$stack_skyfile_id, # Stack identifier
    'dbname|d=s'        => \$dbname, # Database name
    'outroot=s'         => \$outroot, # Output root name
    'run-state=s'       => \$run_state,
    'debug'             => \$debug,   # Print to stdout
    'threads=s'         => \$threads,   # Number of threads to use for ppStack
    'reduction=s'       => \$reduction, # Reduction class
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Don't update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    'delete-convolved'  => \$delete_convolved_images,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --stack_id --outroot --run-state",
    -exitval => 3,
          ) unless defined $stack_id
    and defined $outroot
    and defined $run_state;

# lists of file rules which we expect to produce output when convolving
my @outputList = qw(
PPSTACK.OUTPUT
PPSTACK.OUTPUT.MASK
PPSTACK.OUTPUT.VARIANCE
PPSTACK.OUTPUT.EXP          
PPSTACK.OUTPUT.EXPNUM       
PPSTACK.OUTPUT.EXPWT
PPSTACK.TARGET.PSF          
);

# produced if we run photometry
my @outputListPhotom = qw(
PSPHOT.OUT.CMF.MEF
);

# list of file rules produced if convolution is false
my @outputListUnconv = qw(
PPSTACK.UNCONV
PPSTACK.UNCONV.MASK
PPSTACK.UNCONV.VARIANCE
PPSTACK.UNCONV.EXP          
PPSTACK.UNCONV.EXPNUM       
PPSTACK.UNCONV.EXPWT
PPSTACK.OUTPUT.JPEG1        
PPSTACK.OUTPUT.JPEG2        
PPSTACK.CONFIG              
);


my $ipprc = PS::IPP::Config->new() or my_die( "Unable to set up", $stack_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration
$| = 1;
print "I've set up: $stack_id\n";

# XXX camera is not known here; cannot use filerules...
# my $logDest = $ipprc->filename("LOG.EXP", $outroot);
my $logDest = "$outroot.log";

if ($run_state eq 'update') {
    $logDest .= ".update";
}

$ipprc->redirect_to_logfile($logDest) or my_die( "Unable to redirect output", $stack_id, $PS_EXIT_SYS_ERROR ) if $redirect;

my $neb;
my $scheme = file_scheme($outroot);
if ($scheme and $scheme eq 'neb') {
    $neb = $ipprc->nebulous();
}


my $temp_images_exist = 0;
my $image_id = $stack_id;
my $source_id = $ipprc->source_id($dbname, $PS_TABLE_ID_STACK);

# Get list of components for stacking
my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files
my $files;
{
    my $command = "$stacktool -inputskyfile -stack_id $stack_id";
    $command .= " -dbname $dbname" if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform stacktool -inputskyfile: $error_code", $stack_id, $error_code);
    }

    if (@$stdout_buf == 0) {
        &my_die("No input skyfiles selected", $stack_id, $PS_EXIT_PROG_ERROR);
    }
    my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $stack_id, $PS_EXIT_PROG_ERROR);
    $files = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $stack_id, $PS_EXIT_PROG_ERROR);
}

&my_die("Stack list contains less than two elements", $stack_id, $PS_EXIT_DATA_ERROR) unless
    scalar @$files >= 2;

print "I've loaded my inputs: $stack_id\n";

# Parse the list of input files to get the tesselation, skycell identifiers and camera
my $skycell_id;                 # Skycell identifier
my $tess_id;                    # Tesselation identifier
my $camera;                     # Camera
foreach my $file (@$files) {
    # skip warps which are specified as 'ignored'
    if ($file->{ignored}) { next; }
    if (defined $tess_id) {
        &my_die("Tesselation identifiers don't match", $stack_id, $PS_EXIT_DATA_ERROR) unless
            $file->{tess_id} eq $tess_id;
    } else {
        $tess_id = $file->{tess_id};
    }
    if (defined $skycell_id) {
        &my_die("Skycell identifiers don't match", $stack_id, $PS_EXIT_DATA_ERROR) unless
            $file->{skycell_id} eq $skycell_id;
    } else {
        $skycell_id = $file->{skycell_id};
    }
    if (defined $camera) {
        &my_die("Cameras don't match", $stack_id, $PS_EXIT_DATA_ERROR) unless $file->{camera} eq $camera;
    } else {
        $camera = $file->{camera};
    }
}

&my_die("Can't find camera", $stack_id, $PS_EXIT_SYS_ERROR) unless defined $camera;
$ipprc->define_camera($camera);

print "I've configured everything: $stack_id\n";

# Recipes to use based on reduction class
$reduction = 'DEFAULT' unless defined $reduction;
my $recipe_ppStack = $ipprc->reduction($reduction, 'STACK_PPSTACK'); # Recipe to use for ppStack
my $recipe_ppSub = $ipprc->reduction($reduction, 'STACK_PPSUB'); # Recipe to use for ppSub
my $recipe_psphot  = $ipprc->reduction($reduction, 'STACK_PSPHOT'); # Recipe to use for psphot
my $recipe_ppstats = 'STACKSTATS';
if ($
run_state eq 'update') {
     $recipe_ppstats = 'WARPSTATS';
}
unless ($recipe_ppStack and $recipe_ppSub and $recipe_psphot) {
    &my_die("Couldn't find selected reduction for STACK_PPSTACK, STACK_PPSUB and STACK_PSPHOT: $reduction\n", $stack_id, $PS_EXIT_CONFIG_ERROR);
}

my $recipe;
{
    my $command = "$ppConfigDump -camera $camera -recipe PPSTACK $recipe_ppStack -dump-recipe PPSTACK -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", $stack_id, $error_code);
    }
    $recipe = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $stack_id, $PS_EXIT_PROG_ERROR);
}
my $convolve = metadataLookupBool($recipe, 'CONVOLVE'); # Convolve inputs?
my $photometry = metadataLookupBool($recipe, 'PHOTOMETRY'); # perform photometry?
my $output_nocomp = metadataLookupBool($recipe, 'OUTPUT.NOCOMP'); # change filerules to produced uncompressed output images
my $output_logflux = metadataLookupBool($recipe, 'OUTPUT.LOGFLUX'); # change filerules to produce logflux compressed output images.
my $output_deepexp = metadataLookupBool($recipe, 'OUTPUT.DEEPEXP'); # change filerules to produce exptime map with more depth
my $replicate_outputs = (defined($neb) and metadataLookupBool($recipe, 'OUTPUT.REPLICATE')); # replicate output images
my $skip_missing_inputs = metadataLookupBool($recipe, 'SKIP.MISSING.INPUTS'); # ignore missing inputs 
if ($output_nocomp and $output_logflux) {
    &my_die("Unable to not compress and logflux compress simultaneously. Check config.",$stack_id, $PS_EXIT_CONFIG_ERROR);
}
if ($output_nocomp and $output_deepexp) {
    &my_die("Unable to not compress and use deepexp simultaneously. Check config.",$stack_id, $PS_EXIT_CONFIG_ERROR);
}
my $stack_type = metadataLookupStr($recipe, 'STACK.TYPE');
&my_die("STACK.TYPE not found in recipe. Check config.",$stack_id, $PS_EXIT_CONFIG_ERROR) unless $stack_type;

# Generate MDC file with the inputs
my $tess_base = basename($tess_id);
my ($listFile, $listName) = tempfile("/tmp/$tess_base.$skycell_id.stk.$stack_id.list.XXXX", UNLINK => !$save_temps );
my $num = 0;
my $inputSources;               # Sources to use as stamps
foreach my $file (@$files) {
    if ($file->{ignored}) { next; }

    # check for the input files
    my $image    = $ipprc->filename("PSWARP.OUTPUT", $file->{path_base} ); # Image name
    my $mask     = $ipprc->filename( "PSWARP.OUTPUT.MASK", $file->{path_base} ); # Mask name
    my $weight   = $ipprc->filename( "PSWARP.OUTPUT.VARIANCE", $file->{path_base} ); # Weight name
    my $psf      = $convolve ? $ipprc->filename( "PSPHOT.PSF.SKY.SAVE", $file->{path_base} ) : undef; # PSF name
    my $sources  = $ipprc->filename("PSWARP.OUTPUT.SOURCES", $file->{path_base}); # Sources name
    my $bkgmodel = $ipprc->filename("PSWARP.OUTPUT.BKGMODEL", $file->{path_base});

    my $have_image   = $ipprc->file_exists( $image );
    my $have_mask    = $ipprc->file_exists( $mask );
    my $have_weight  = $ipprc->file_exists( $weight );
    my $have_sources = $ipprc->file_exists( $sources );
    my $have_psf;
    if ($convolve) { $have_psf = $ipprc->file_exists( $psf ); }

    if ($skip_missing_inputs) {
	my $missing_inputs = 0;
	unless ($have_image)  { print "WARNING: MISSING INPUT $image  \n"; $missing_inputs = 1; }
	unless ($have_mask)   { print "WARNING: MISSING INPUT $mask   \n"; $missing_inputs = 1; }
	unless ($have_weight) { print "WARNING: MISSING INPUT $weight \n"; $missing_inputs = 1; }
	unless ($have_sources){ print "WARNING: MISSING INPUT $sources\n"; $missing_inputs = 1; }
	if ($convolve) { unless ($have_psf) { print "WARNING: MISSING INPUT $psf\n"; $missing_inputs = 1;}}
	if ($missing_inputs) { next; }
    } else {
	&my_die("Image $image does not exist", $stack_id, $PS_EXIT_SYS_ERROR) unless $have_image;
	&my_die("Mask $mask does not exist", $stack_id, $PS_EXIT_SYS_ERROR) unless $have_mask;
	&my_die("Weight $weight does not exist", $stack_id, $PS_EXIT_SYS_ERROR) unless $have_weight;
	&my_die("Sources $sources does not exist", $stack_id, $PS_EXIT_SYS_ERROR) unless $have_sources;
	&my_die("PSF $psf does not exist", $stack_id, $PS_EXIT_SYS_ERROR) if ($convolve and not $have_psf);
    }

    print $listFile "INPUT$num\tMETADATA\n";

    print $listFile "\tIMAGE\tSTR\t"    . $image    . "\n";
    print $listFile "\tMASK\tSTR\t"     . $mask     . "\n";
    print $listFile "\tVARIANCE\tSTR\t" . $weight   . "\n";
    print $listFile "\tPSF\tSTR\t"      . $psf      . "\n" if $convolve;
    print $listFile "\tSOURCES\tSTR\t"  . $sources  . "\n";
    print $listFile "\tBKGMODEL\tSTR\t" . $bkgmodel . "\n" if $ipprc->file_exists( $bkgmodel );

    print $listFile "END\n\n";
    $num++;
}
close($listFile);

print "I've checked everything: $stack_id\n";

# Prepare the output files
my @outputFiles;
prepare_outputs(\@outputFiles, \@outputList,       $outroot) if $convolve;
prepare_outputs(\@outputFiles, \@outputListPhotom, $outroot) if $photometry;
prepare_outputs(\@outputFiles, \@outputListUnconv, $outroot); # unconvolved output always go here

# we need the output image name for the database
my $outputName = $outputFiles[0];

## use an explicit stack name for psphot output objects
my $do_stats = 1;   
my $outputStats;
my $traceDest;
my $configuration = prepare_output("PPSTACK.CONFIG", $outroot, 1);
if ($run_state eq 'new') {
    $outputStats = prepare_output("SKYCELL.STATS", $outroot, 1);
    $traceDest = prepare_output("TRACE.EXP", $outroot, 1);
    push @outputFiles, $configuration;
    push @outputFiles, $outputStats;
} else {
    # we need to do stats regardless because we need the quality flag
    # $do_stats = 0;
    $outputStats = prepare_output("SKYCELL.STATS.UPDATE", $outroot, 1) if $do_stats;
    $traceDest = prepare_output("TRACE.EXP.UPDATE", $outroot, 1);
}

$temp_images_exist = 1;  # failures after this point should attempt to delete the temp images

my $cmdflags;

# Perform stacking
unless ($no_op) {
    my $command = "$ppStack -input $listName $outroot";
    $command .= " -stats $outputStats" if $do_stats;
    $command .= " -recipe PPSTACK $recipe_ppStack";
    $command .= " -recipe PPSUB $recipe_ppSub";
    $command .= " -recipe PSPHOT $recipe_psphot";
    $command .= " -recipe PPSTATS $recipe_ppstats" if $do_stats;;
    $command .= " -stack-type $stack_type";
    $command .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE";
    $command .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF";
    $command .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF";
    $command .= " -F SOURCE.PLOT.MOMENTS SOURCE.PLOT.SKY.MOMENTS";
    $command .= " -F SOURCE.PLOT.PSFMODEL SOURCE.PLOT.SKY.PSFMODEL";
    $command .= " -F SOURCE.PLOT.APRESID SOURCE.PLOT.SKY.APRESID";
    if ($output_nocomp) {
        $command .= " -F PPSTACK.OUTPUT PPSTACK.OUTPUT.NOCOMP";
        $command .= " -F PPSTACK.OUTPUT.VARIANCE PPSTACK.OUTPUT.VARIANCE.NOCOMP";
        $command .= " -F PPSTACK.OUTPUT.EXPWT PPSTACK.OUTPUT.EXPWT.NOCOMP";
        $command .= " -F PPSTACK.OUTPUT.EXP PPSTACK.OUTPUT.EXP.NOCOMP";
        $command .= " -F PPSTACK.UNCONV PPSTACK.UNCONV.NOCOMP";
        $command .= " -F PPSTACK.UNCONV.VARIANCE PPSTACK.UNCONV.VARIANCE.NOCOMP";
        $command .= " -F PPSTACK.UNCONV.EXPWT PPSTACK.UNCONV.EXPWT.NOCOMP";
        $command .= " -F PPSTACK.UNCONV.EXP PPSTACK.UNCONV.EXP.NOCOMP";
    }
    if ($output_logflux) {
	$command .= " -R PPSTACK.OUTPUT          FITS.TYPE STK_UNIONS_COMP";
	$command .= " -R PPSTACK.OUTPUT.VARIANCE FITS.TYPE STK_UNIONS_COMP"; 
        $command .= " -R PPSTACK.UNCONV          FITS.TYPE STK_UNIONS_COMP";
	$command .= " -R PPSTACK.UNCONV.VARIANCE FITS.TYPE STK_UNIONS_COMP";
    }
    # if the total exposure time is long (> 6500 sec), need to use
    # a different compression options.
    if ($output_deepexp) {
	$command .= " -R PPSTACK.OUTPUT.EXP FITS.TYPE EXP_UNIONS ";
	$command .= " -R PPSTACK.UNCONV.EXP FITS.TYPE EXP_UNIONS ";
    }
    $command .= " -threads $threads" if defined $threads;
    $command .= " -debug-stack" if defined $debug;
    $command .= " -tracedest $traceDest -log $logDest";
    $command .= " -dbname $dbname" if defined $dbname;
    $command .= " -image_id $image_id" if defined $image_id;
    $command .= " -source_id $source_id" if defined $source_id;

    $command .= " -stack_id   $stack_id"   if defined $stack_id;
    $command .= " -skycell_id $skycell_id" if defined $skycell_id;
    $command .= " -tess_id    $tess_base"  if defined $tess_base;

    if ($run_state eq 'new') {
        $command .= " -dumpconfig $configuration";
    } else {
        $command .= " -ipprc $configuration";
    }


    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppStack: $error_code", $stack_id, $error_code);
    }

    my $quality;                # Quality flag
    if ($do_stats) {
        my $outputStatsReal = $ipprc->file_resolve($outputStats);
        &my_die("Couldn't find expected output file: $outputStats", $stack_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($outputStatsReal);

        # measure stats
        $command = "$ppStatsFromMetadata $outputStatsReal - STACK_SKYCELL";
        ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppStatsFromMetadata: $error_code", $stack_id, $error_code);
        }
        foreach my $line (@$stdout_buf) {
            $cmdflags .= " $line";
        }
        chomp $cmdflags;

        ($quality) = $cmdflags =~ /-quality (\d+)/; # Quality flag
    }

    if (!$quality) {
	

        check_outputs(\@outputFiles, $replicate_outputs);

	if (($convolve)&&($delete_convolved_images)) { # We made convolved products, but do not wish to keep them anymore
	    my @products_to_clear = ('PPSTACK.OUTPUT',
				     'PPSTACK.OUTPUT.MASK',
				     'PPSTACK.OUTPUT.VARIANCE',
				     'PPSTACK.OUTPUT.EXP',
				     'PPSTACK.OUTPUT.EXPNUM',
				     'PPSTACK.OUTPUT.EXPWT');
	    foreach my $product (@products_to_clear) {
		my $file = $ipprc->filename($product,$outroot,$skycell_id);
		print "Deleting unwanted convolved product: $file\n";
		my $error= $ipprc->kill_file($file);
		if ($error_code) {
		    print "Failed to delete unwanted convolved product: $error_code\n";
		}
	    }
	}

    }

    print "I've stacked $listName: $stack_id\n";
}

delete_temps() unless ($save_temps);

unless ($no_update) {

    # Add the stack result
    {
        my $command = "$stacktool";
        my $mode;
        if ($run_state eq 'new') {
            $mode = "-addsumskyfile";
            $command .= " -uri $outputName -path_base $outroot";
            $command .= " -hostname $host" if defined $host;
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        } else {
            $mode = "-updaterun -set_state full";
        }
        $command .= " $mode -stack_id $stack_id";
        $command .= " $cmdflags" if $do_stats;
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform stacktool $mode $error_code", $stack_id, $error_code);
        }
    }

}

print "I've updated the database: $stack_id\n";


my $my_die_called = 0;
sub my_die
{
    my $msg = shift;            # Warning message on die
    my $stack_id = shift;       # Stack identifier
    my $exit_code = shift;      # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    unless ($my_die_called) {
        $my_die_called = 1;
        delete_temps() unless ($save_temps);
    }

    if (defined $stack_id and not $no_update) {
        my $command = "$stacktool -stack_id $stack_id -fault $exit_code";
        if ($run_state eq 'new') {
            $command .= " -addsumskyfile";
            $command .= " -hostname $host" if defined $host;
            $command .= " -path_base $outroot" if defined $outroot;
            $command .= (" -dtime_script " . ((DateTime->now->mjd - $mjd_start) * 86400));
        } else {
            $command .= " -updatesumskyfile";
        }
        $command .= " -dbname $dbname" if defined $dbname;
        system ($command);
    }
    exit $exit_code;
}

# Ensure temporary images are deleted
sub delete_temps
{
    unless ($temp_images_exist) {
        print "No temporary images yet generated\n";
        return 1;
    }

    print "Ensuring temporary images are deleted.\n";

    my $temp_delete = defined($recipe) and metadataLookupBool($recipe, 'TEMP.DELETE'); # Delete temp files?
    if ($temp_delete) {
        my $temp_dir = metadataLookupStr($ipprc->{_siteConfig}, 'TEMP.DIR') or &my_die("Unable to find temporary directory in site configuration", $stack_id, $PS_EXIT_CONFIG_ERROR); # Directory with temporary files
        my $temp_image = metadataLookupStr($recipe, 'TEMP.IMAGE'); # Suffix for image
        my $temp_mask = metadataLookupStr($recipe, 'TEMP.MASK'); # Suffix for mask
        my $temp_weight = metadataLookupStr($recipe, 'TEMP.VARIANCE'); # Suffix for weight
        my $temp_root = basename($outroot); # Root name for temporary files

        if (not defined $temp_dir or not defined $temp_image or not defined $temp_mask or
            not defined $temp_weight) {
            &my_die("Unable to find details for temporary images.", $stack_id, $PS_EXIT_CONFIG_ERROR);
        }

        for (my $i = 0; $i < $num; $i++) {
            foreach my $ext ( $temp_image, $temp_mask, $temp_weight ) {
                my $file = $temp_dir . '/' . $temp_root . '.' . $i . '.' . $ext;
                unlink $file if -e $file;
            }
        }
    }
}
sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, undef, $delete, \$error)
                    or &my_die("failed to prepare output file for: $filerule", $stack_id, $error);
    return $output;
}

sub prepare_outputs
{
    my $fileList = shift;
    my $ruleList = shift;
    my $outroot = shift;
    
    foreach my $rule (@$ruleList) {
        push @$fileList, prepare_output($rule, $outroot, 1);
    }
}

sub check_output
{
    my $file = shift;
    my $replicate = shift;

    if (!defined $file) {
        return;
    }

    &my_die("Couldn't find expected output file: $file",  $stack_id, $PS_EXIT_SYS_ERROR) unless $ipprc->file_exists($file);

    # Funpack to confirm we've really made things correctly
    my $diskfile = $ipprc->file_resolve($file);
    if ($diskfile =~ /fits/) {
        my $funpack  = can_run('funpack') or &my_die ("Can't find funpack", $stack_id, $PS_EXIT_SYS_ERROR);
	my $check_command = "$funpack -S $diskfile > /dev/null";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $check_command, verbose => $verbose);
	if (!$success) {
	    &my_die("Output file not a valid fits file: $file", $stack_id, $PS_EXIT_SYS_ERROR);
	}
    }
    #####

    if ($replicate and $neb) {
        $ipprc->replicate_file($file) or &my_die("failed to replicate: $file\n",  $stack_id, $PS_EXIT_SYS_ERROR);
    }
}

sub check_outputs
{
    my $fileList = shift;
    my $replicate = shift;
    foreach my $file (@$fileList) {
        check_output($file, $replicate);
    }
}

print "I've reached the end of processing with a code of $?: $stack_id\n";

END {
    my $exit = $?;
    system("sync") == 0 or die "failed to execute sync: $!";
    $? = $exit;
}


__END__
