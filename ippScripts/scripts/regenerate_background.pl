#! /usr/bin/env perl

# Script to call the appropriate commands to regenerate the background models
# for stages subsequent to the chip stage.  

use warnings;
use strict;
use Carp;

use IPC::Cmd 0.36 qw( can_run run );
use File::Spec;
use File::Temp qw( tempfile );
use PS::IPP::Config 1.01 qw( :standard );
use Getopt::Long qw (GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );



my $ppConfigDump = can_run('ppConfigDump');
my $pswarp       = can_run('pswarp');

# Parse command line options.
my ($stage, $camera, $stage_id, $skycell_id, $dbname, $verbose, $logfile, $save_temps);
GetOptions('stage=s'         => \$stage,       # which analysis stage to generate bkg model
	   'camera|i=s'      => \$camera,      # user-supplied camera name
	   'stage_id=s'      => \$stage_id,    # id for this stage
	   'skycell_id=s'    => \$skycell_id,  # skycell_id
	   'verbose'         => \$verbose,     # verbose commands
	   'logfile=s'       => \$logfile,     # destination for stdout and stderr  
	   'save_temps'      => \$save_temps,  # save temporary files.
	   'dbname=s'        => \$dbname       # database name
    ) or pod2usage ( 2 );

pod2usage( -msg => "Usage: $0 --camera (name) --stage (stage) --stage_id (stage_id) [--dbname (dbname)]",
	   -exitval => 2 ) if scalar @ARGV;

pod2usage( -msg => "Required options: --camera (name) --stage (stage) --stage_id (stage_id)",
	   -exitval => 3 ) unless
    defined $camera and
    defined $stage and
    defined $stage_id;

pod2usage( -msg => "Required options: --camera (name) --stage (stage) --stage_id (stage_id) --skycell_id (skycell_id)",
	   -exitval => 3) if (($stage eq 'warp')&&(!defined($skycell_id)));


my $ipprc = PS::IPP::Config->new ( $camera ) or 
    &my_die("Unable to set up", $stage, $stage_id, $PS_EXIT_CONFIG_ERROR); # used for path/neb info

# We can only regenerate a background for certain stages.
unless (($stage eq "warp") || ($stage eq "stack") || ($stage eq "warpstack")) {
    &my_die("Invalid stage to regenerate", $stage, $stage_id, $PS_EXIT_CONFIG_ERROR);
}

$ipprc->redirect_output($logfile) or
    &my_die("Unable to redirect output to logfile", $stage, $stage_id, $PS_EXIT_UNKNOWN_ERROR) if $logfile;

my $mdcParser = PS::IPP::Metadata::Config->new; # parser for metadata config files


## warp stage
if ($stage eq 'warp') {
    my $warp_id = $stage_id; # Because I think I used the wrong id in places.
    &my_die("--stage_id required for stage warp", $stage, $stage_id, $PS_EXIT_CONFIG_ERROR) if !$stage_id;
    &my_die("--skycell_id required for stage warp", $stage, $stage_id, $PS_EXIT_CONFIG_ERROR) if !$skycell_id;

    # Configuration stuff
    my $warptool = can_run('warptool') or 
	&my_die("Can't find warptool",$stage,$stage_id, $PS_EXIT_UNKNOWN_ERROR);

    my $astromSource;               # The astrometry source
    {
	my $command = "$ppConfigDump -camera $camera -recipe PSWARP BACKGROUND -dump-recipe PSWARP -";
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run(command => $command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform ppConfigDump: $error_code", 
		    $stage, $stage_id, $error_code);
	}
	my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
	    &my_die("Unable to parse metadata config doc", 
		    $stage, $stage_id, $PS_EXIT_PROG_ERROR);
	$astromSource = metadataLookupStr($metadata, 'ASTROM.SOURCE');
    }

    
    my $imfiles;   # Array of component files
    my $command = "$warptool -warped -warp_id $stage_id -skycell_id $skycell_id";
    $command .= " -dbname $dbname " if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderrr_buf ) = 
	run(command => $command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8)  or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform warptool: $error_code", $stage, $stage_id, $error_code);
    }

    if (@$stdout_buf == 0) {
	# No results
    }

    my $in_md = $mdcParser->parse(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata config doc", $stage, $stage_id, $PS_EXIT_PROG_ERROR);
    $imfiles  = parse_md_list($in_md) or
	&my_die("Unable to parse metadata", $stage, $stage_id, $PS_EXIT_PROG_ERROR);
    
    foreach my $imfile (@$imfiles) {
	my $skycell_id = $imfile->{skycell_id};
	my $path_base  = $imfile->{path_base};
	my $tess_id    = $imfile->{tess_id};

	# Parse the results to determine if there are any problems that prevent this from
	# being processed.  I think I might need to think about this a bit more.
	my $status = 1;
	$status = 0 unless defined $path_base and $path_base ne "NULL";
	my $poor_quality = $imfile->{quality} > 0;
	
# 	my $bkg_file = $ipprc->filename("PSWARP.OUTPUT.BKGMODEL",$path_base,$skycell_id);
# 	my $bkg_exists = $ipprc->file_exists($bkg_file);
	my $bkg_exists = $imfile->{background_model};
	if ($bkg_exists) { $status = 0; } # We do not need to remake this.	

	# Construct temp files
	my ($bkgList_file,$bkgList_name) = tempfile("/tmp/bkgreg.warp.bkg.${warp_id}.${skycell_id}.XXXX",
						    UNLINK => !$save_temps);
	my ($astromList_file,$astromList_name) = tempfile("/tmp/bkgreg.warp.astrom.${warp_id}.${skycell_id}.XXXX",
							 UNLINK => !$save_temps);
	my $chipFiles;
	{
	    my $inputs_command = "$warptool -scmap -warp_id $warp_id ";
	    $inputs_command   .= " -skycell_id $skycell_id ";
	    $inputs_command   .= " -dbname $dbname " if defined $dbname;
	    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		run (command => $inputs_command, verbose => $verbose);
	    unless ($success) {
		$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		&my_die("Unable to perform warptool -scmap: $error_code", 
			$skycell_id,$warp_id,$PS_EXIT_SYS_ERROR);
	    }
	    my $md = $mdcParser->parse(join "", @$stdout_buf) or
		&my_die("Unable to parse warptool -scmap: $error_code", 
			$skycell_id,$warp_id,$PS_EXIT_SYS_ERROR);
	    $chipFiles = parse_md_list($md) or
		&my_die("Unable to parse warptool -scmap: $error_code", 
			$skycell_id,$warp_id,$PS_EXIT_SYS_ERROR);
	}
	my $wrote_astrom = 0;
	foreach my $chipFile (@$chipFiles) {
	    my $bkgModel = $ipprc->filename("PSPHOT.BACKMDL",
					    $chipFile->{chip_path_base},
					    $chipFile->{class_id});
	    my $astrom   = $ipprc->filename($astromSource,$chipFile->{cam_path_base});
	    print $bkgList_file "$bkgModel\n";
	    if (!$wrote_astrom) {
		print $astromList_file "$astrom\n";
		$wrote_astrom = 1;
	    }
	}
	close ($bkgList_file);
	close ($astromList_file);

	# Construct skyfile if needed
	my $skyCell_name = $ipprc->filename("SKYCELL.TEMPLATE",$path_base,$skycell_id);
	my $skyCell_exists = $ipprc->file_exists($skyCell_name);
	if (!$skyCell_exists) { # Generate it
	    my $skyFile = prepare_output("SKYCELL.TEMPLATE", $path_base, $skycell_id, 1);
	    $ipprc->skycell_file( $tess_id, $skycell_id, $skyFile, $verbose ) or
		&my_die("Unable to generate template skycell",$skycell_id,$warp_id,$PS_EXIT_SYS_ERROR);
	}
	my $bkgOut_log = prepare_output("LOG.EXP",$path_base . ".BKG_REG",$skycell_id,1);

	# Construct pswarp command
	my $pswarp_command = "$pswarp ";
	$pswarp_command .= " -list $bkgList_name -astromlist $astromList_name -bkglist $bkgList_name ";
	$pswarp_command .= " ${path_base} ";
	$pswarp_command .= " $skyCell_name ";
	$pswarp_command .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF ";
	$pswarp_command .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF ";
	$pswarp_command .= " -F SOURCE.PLOT.MOMENTS SOURCE.PLOT.SKY.MOMENTS ";
	$pswarp_command .= " -F SOURCE.PLOT.PSFMODEL SOURCE.PLOT.SKY.PSFMODEL ";
	$pswarp_command .= " -F SOURCE.PLOT.APRESID SOURCE.PLOT.SKY.APRESID ";
	$pswarp_command .= " -recipe PSWARP WARP -Db PSF FALSE -Db BACKGROUND.MODEL TRUE ";
	$pswarp_command .= " -Db SOURCES FALSE ";
	$pswarp_command .= " -log $bkgOut_log -threads 1 ";
	$pswarp_command .= " -dbname $dbname " if defined $dbname;

	# Execute
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderrr_buf ) = 
	    run(command => $pswarp_command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8)  or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform pswarp: $error_code", $stage, $stage_id, $error_code);
	}
	
	my $update_command = "$warptool ";
	$update_command .= " -updateskyfile -warp_id $warp_id -skycell_id $skycell_id ";
	$update_command .= " -set_background_model 1 ";
	$update_command .= " -dbname $dbname " if defined $dbname;

	( $success, $error_code, $full_buf, $stdout_buf, $stderrr_buf ) = 
	    run(command => $update_command, verbose => $verbose);
	unless ($success) {
	    $error_code = (($error_code >> 8)  or $PS_EXIT_PROG_ERROR);
	    &my_die("Unable to perform warptool: $error_code", $stage, $stage_id, $error_code);
	}
	

    }
}
## stack stage
elsif ($stage eq 'stack') {
    &my_die("--stage_id required for stage stack", $stage, $stage_id, $PS_EXIT_CONFIG_ERROR) if !$stage_id;
    my $stack_id = $stage_id; # Same as above.  Alias this so I don't make mistakes.
    
    # Configuration stuff
    my $stacktool = can_run('stacktool') or
	&my_die("Can't find stacktool",$stage,$stage_id,$PS_EXIT_UNKNOWN_ERROR);
    my $ppStackMedian = can_run('ppStackMedian') or
	&my_die("Can't find ppStackMedian",$stage,$stage_id,$PS_EXIT_UNKNOWN_ERROR);

    # Get the information about this run
    my $st_command = "$stacktool -sumskyfile -stack_id $stack_id";
    $st_command   .= " -dbname $dbname " if defined $dbname;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $st_command, verbose => $verbose);
    unless($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -sumskyfile",$stage,$stage_id,$error_code);
    }
    if (@$stdout_buf == 0) {
	# Nothing to do;
    }
    my $in_md = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $stage, $stage_id, $PS_EXIT_PROG_ERROR);
    my $stacks  = parse_md_list($in_md) or
        &my_die("Unable to parse metadata", $stage, $stage_id, $PS_EXIT_PROG_ERROR);
    my $stack = shift(@$stacks);
    my $path_base = $stack->{path_base};

    # Get inputs
    my $imfiles;  # Array of component files
    my $command = "$stacktool -inputskyfile -stack_id $stack_id";
    $command .= " -dbname $dbname " if defined $dbname;
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool -inputskyfile",$stage,$stage_id,$error_code);
    }
    if (@$stdout_buf == 0) {
	# Nothing to do;
    }
    $in_md = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse metadata config doc", $stage, $stage_id, $PS_EXIT_PROG_ERROR);
    $imfiles  = parse_md_list($in_md) or
        &my_die("Unable to parse metadata", $stage, $stage_id, $PS_EXIT_PROG_ERROR);

    my $num = 0;
    my $expTime;
    my ($inputMDC_file, $inputMDC_name) = tempfile("/tmp/bkgreg.stack.mdc.${stage_id}.XXXX",
						   UNLINK => !$save_temps);
    foreach my $imfile (@$imfiles) {
	if ($imfile->{ignored}) {next; }
	unless ($imfile->{background_model}) {
	    &my_die("Not all inputs have valid background models",
		    $stage,$stage_id,$PS_EXIT_PROG_ERROR);
	}
	print $inputMDC_file "INPUT${num}\tMETADATA\n";
	
	print $inputMDC_file "\tIMAGE\tSTR\t" . $ipprc->filename( "PSWARP.OUTPUT.BKGMODEL",
								  $imfile->{path_base} ) . "\n";
	print $inputMDC_file "\tSOURCES\tSTR\t" . $ipprc->filename( "PSWARP.OUTPUT.SOURCES",
								    $imfile->{path_base} ) . "\n";
	print $inputMDC_file "END\n\n";
	$num++;

	$expTime = $imfile->{exp_time}; # I need something here.
    }
    close($inputMDC_file);

    my $bkgOut_log = prepare_output("LOG.EXP",$path_base . ".BKG_REG",1);    

    my $ppStack_command = "$ppStackMedian -input $inputMDC_name ${path_base}.mdl ";
    $ppStack_command   .= " -recipe PPSTACK STACK -recipe PPSUB STACK -recipe PSPHOT STACK ";
    $ppStack_command   .= " -recipe PPSTATS STACKSTATS -stack-type DEEP_STACK ";
#    $ppStack_command   .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE ";
#    $ppStack_command   .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF ";
#    $ppStack_command   .= " -F SOURCE.PLOT.MOMENTS SOURCE.PLOT.SKY.MOMENTS ";
#    $ppStack_command   .= " -F SOURCE.PLOT.PSFMODEL SOURCE.PLOT.SKY.PSFMODEL ";
#    $ppStack_command   .= " -F SOURCE.PLOT.APRESID SOURCE.PLOT.SKY.APRESID ";
    $ppStack_command   .= " -Db PHOTOMETRY F -Db VARIANCE F -Db CONVOLVE F ";
    $ppStack_command   .= " -Df DEFAULT.EXPTIME $expTime ";
    $ppStack_command   .= " -threads 1 -log $bkgOut_log ";
    $ppStack_command   .= " -dbname $dbname " if defined $dbname;

    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $ppStack_command, verbose => $verbose);
    unless($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform ppStackMedian",$stage,$stage_id,$error_code);
    }

    my $update_command = "$stacktool ";
    $update_command .= " -updatesumskyfile -stack_id $stack_id ";
    $update_command .= " -set_background_model 1 -fault 0 ";
    $update_command .= " -dbname $dbname " if defined $dbname;
    
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $update_command, verbose => $verbose);
    unless ($success) {
	$error_code = (($error_code >> 8)  or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stacktool: $error_code", $stage, $stage_id, $error_code);
    }
    


}
## do both warp and stack
elsif ($stage eq 'warpstack') {

}

sub prepare_output
{
    my $filerule = shift;
    my $outroot  = shift;
    my $skycell_id = shift;
    my $delete = shift;
    $delete = 0 if !defined $delete;

    my $error;
    my $output = $ipprc->prepare_output($filerule, $outroot, $skycell_id, $delete, \$error)
	or &my_die("failed to prepare output file for: $filerule", $skycell_id, $stage_id, $error);
    return $output;
}


sub my_die {
    my $msg = shift;       # Warning message on die
    my $stage = shift;     # stage name
    my $stage_id = shift;  # identifier
    my $exit_code = shift; # exit code

    carp($msg);
    exit $exit_code;
}




	
    



