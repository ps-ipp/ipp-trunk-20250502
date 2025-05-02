#! /usr/bin/env perl

# generate the input & output files lists and commands for a single staticskyRun.

use Carp;
use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use DateTime;
use Data::Dumper;
use File::Basename;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $remotetool = can_run('remotetool') or (warn "Can't find remotetool" and $missing_tools = 1);
my $stacktool    = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $staticskytool= can_run('staticskytool') or (warn "Can't find staticskytool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($remote_id,$sky_id,$camera,$dbname,$verbose,$path_base,$no_update,$cmd_recipe);
GetOptions(
    'remote_id=s'    => \$remote_id,
    'sky_id=s'       => \$sky_id,
    'camera|c=s'     => \$camera,
    'dbname|d=s'     => \$dbname,
    'recipe=s'       => \$cmd_recipe,
    'path_base=s'    => \$path_base,
    'no_update'      => \$no_update,
    'verbose'        => \$verbose,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id --sky_id --camera --dbname --path_base --recipe", -exitval => 3) unless
    defined($remote_id) and
    defined($sky_id) and
    defined($camera) and
    defined($path_base) and
    defined($cmd_recipe) and
    defined($dbname);

# Hard coded values
# Now accessible from a recipe
my %remote_recipe = ();
{
    my $verbose = 0;
    my $conf_cmd = "$ppConfigDump -dump-recipe REMOTE -";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $conf_cmd, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform ppConfigDump: $error_code", -1, $PS_EXIT_SYS_ERROR);
    }
    my $mdcParser = PS::IPP::Metadata::Config->new;
    my $metadata = $mdcParser->parse(join "", @$stdout_buf);

    my $active_recipe = '';
    my %recipes = ();
    
#    print Dumper($metadata);
    foreach my $entry (@{ $metadata }) {
        if (${ $entry }{name} eq 'ACTIVE') {
            $active_recipe = ${ $entry }{value}; # Not actually used
        }
        else {
            if (${ $entry }{class} eq 'metadata') { # A real recipe
                my $name = ${ $entry }{name};
                foreach my $tentry (@{ ${ $entry }{value} }) {
                    if (${ $tentry }{class} eq 'scalar') { # A recipe value
                        $recipes{$name}{${ $tentry }{name}} = ${ $tentry }{value};
                    }
                    elsif (${ $tentry }{class} eq 'metadata') { # A recipe array 
                        foreach my $arr_entry (@{ ${ $tentry }{value} }) {
                            push @{ $recipes{$name}{${ $tentry }{name}} }, ${ $arr_entry }{value};
			}
		    }
		}
	    }
	}
    }
        
    unless (exists($recipes{$cmd_recipe})) { &my_die("Cannot find recipe $cmd_recipe", -1, $PS_EXIT_CONFIG_ERROR) };
#    print Dumper(%recipes);
    %remote_recipe = %{ $recipes{$cmd_recipe} }; # Select the appropriate recipe.
#    print Dumper(\%remote_recipe);
}


my $remote_root     = $remote_recipe{REMOTE_ROOT};  # Far side destination base location
my $remote_hostname = $remote_recipe{REMOTE_HOSTNAME};         # Name of the remote node.
my $threads_req     = 4;                      # How many threads are we going to use?

my $fail_state = "prep_fail";


my $ipprc = PS::IPP::Config->new( $camera ) or &my_die( "Unable to set up", $remote_id, $sky_id, $PS_EXIT_CONFIG_ERROR, $fail_state);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my @return_component_list = ("LOG.EXP","PSPHOT.STACK.CONFIG","DBINFO.EXP");
			     
			 #    "PSPHOT.CHISQ.IMAGE","PSPHOT.CHISQ.MASK","PSPHOT.CHISQ.VARIANCE");
                             
my @return_component_list_per_input = (#"PSPHOT.STACK.OUTPUT.IMAGE","PSPHOT.STACK.OUTPUT.MASK","PSPHOT.STACK.OUTPUT.VARIANCE","PSPHOT.STACK.OUTPUT");
				       "PSPHOT.STACK.OUTPUT",
				       "PSPHOT.STACK.PSF.SAVE",
				       "PSPHOT.STACK.BACKMDL");

# STEP 0: Open output files
my $uri_transfer= $path_base . ".transfer";
my $uri_check   = $path_base . ".check";
my $uri_config  = $path_base . ".config";
my $uri_generate= $path_base . ".generate";
my $uri_return  = $path_base . ".return";

my $disk_transfer= $ipprc->file_resolve($uri_transfer,1);
my $disk_check   = $ipprc->file_resolve($uri_check,1);
my $disk_config  = $ipprc->file_resolve($uri_config,1);
my $disk_generate= $ipprc->file_resolve($uri_generate,1);
my $disk_return  = $ipprc->file_resolve($uri_return,1);

my (undef, $remote_config) = &uri_convert($uri_config); # Needs to be done after we've created it.

open(TRANSFER, ">$disk_transfer")  || &my_die("Couldn't open file? $disk_transfer",$remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CHECK,    ">$disk_check")     || &my_die("Couldn't open file? $disk_check",   $remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CONFIG,   ">$disk_config")    || &my_die("Couldn't open file? $disk_config",  $remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(GENERATE, ">$disk_generate")  || &my_die("Couldn't open file? $disk_generate",$remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(RETURN,   ">$disk_return")    || &my_die("Couldn't open file? $disk_return",  $remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);

my $job_index = 0;

# STEP 1: Get exposure level information from the stackRun we're working from.
my ($reduction, $workdir, $tess_id, $skycell_id);
{
    my $command = "$staticskytool -todo -sky_id $sky_id";
    $command .= " -dbname $dbname " if defined($dbname);
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run staticskytool -inputs ", $remote_id, $sky_id, $error_code, $fail_state);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine stack component information.", $remote_id, $sky_id, $PS_EXIT_PROG_ERROR, $fail_state);
    my $metadata = parse_md_list($MDlist);

    my $stack = $metadata->[0];

    $reduction = $stack->{reduction};
    $reduction = 'DEFAULT' unless defined($reduction);
    $workdir   = $stack->{workdir};
    $tess_id   = $stack->{tess_id};
    $skycell_id= $stack->{skycell_id};
}
my $ipp_outroot    = "${workdir}/${tess_id}/${skycell_id}/${tess_id}.${skycell_id}.sky.${sky_id}";
my $remote_outroot = &uri_local_to_remote($ipp_outroot);
my $remote_outdir  = &uri_local_to_remote("${workdir}/${tess_id}/${skycell_id}");

# STEP 2: Get the list of inputs
my ($inputData);
{
    # This actually returns all the individual stack/skyfiles that comprise this run.  Because consistency.
    my $command = "$staticskytool -inputs -sky_id $sky_id ";
    $command .= " -dbname $dbname " if defined($dbname);
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run stacktool -inputskyfile ", $remote_id, $sky_id, $error_code, $fail_state);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine stack component information.", $remote_id, $sky_id, $PS_EXIT_PROG_ERROR, $fail_state);
    $inputData = parse_md_list($MDlist);
}

# STEP 3: Iterate over the sub-components
{
    # Generate a file mapping the symbolic and "disk" representations of each needed file.
    my $uri_compmap   = "${ipp_outroot}.compmap";
    my $component_map = $ipprc->file_resolve($uri_compmap,1);    
    my ($ipp_compmap, $remote_compmap) = &uri_to_outputs($component_map);
    open(COMPMAP,">$ipp_compmap") || &my_die("Couldn't open file? $ipp_compmap",  $remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);
    
    # Loop over all needed components for this stack.
    my $nInputs = 0;
    my $input_path_base_string = "";
    foreach my $inputEntry ( @{ $inputData } ) {
	my $input_stack_id  = $inputEntry->{stack_id};

        my $input_path_base = $inputEntry->{path_base};
        my $remote_path_base = &uri_local_to_remote($input_path_base);
        $input_path_base_string .= " $remote_path_base ";
	$input_path_base_string .= " $input_stack_id ";
        if ($verbose) { print STDERR "$input_path_base_string\n"; }

        # Append file names to transfer lists so we can recover if inputs are deleted.
	my $remote_file;
	my $ipp_disk;
	#  ${remote_root}/tmp/${ipp_disk}
        (undef,$remote_file) = &uri_to_outputs($input_path_base . ".unconv.fits");
	($ipp_disk,undef)    = &uri_convert($input_path_base . ".unconv.fits");
	print COMPMAP "$remote_file ${remote_root}/tmp/${ipp_disk}\n";
        (undef,$remote_file) = &uri_to_outputs($input_path_base . ".unconv.wt.fits");
	($ipp_disk,undef)    = &uri_convert($input_path_base . ".unconv.wt.fits");
	print COMPMAP "$remote_file ${remote_root}/tmp/${ipp_disk}\n";
        (undef,$remote_file) = &uri_to_outputs($input_path_base . ".unconv.mask.fits");
	($ipp_disk,undef)    = &uri_convert($input_path_base . ".unconv.mask.fits");
	print COMPMAP "$remote_file ${remote_root}/tmp/${ipp_disk}\n";
        (undef,$remote_file) = &uri_to_outputs($input_path_base . ".unconv.num.fits");
	($ipp_disk,undef)    = &uri_convert($input_path_base . ".unconv.num.fits");
	print COMPMAP "$remote_file ${remote_root}/tmp/${ipp_disk}\n";
#         (undef,$remote_file) = &uri_to_outputs($input_path_base . ".cmf");
# 	($ipp_disk,undef)    = &uri_convert($input_path_base . ".cmf");
# 	print COMPMAP "$remote_file ${remote_root}/tmp/${ipp_disk}\n";
# 	(undef,$remote_file) = &uri_to_outputs($input_path_base . ".psf");
# 	($ipp_disk,undef)    = &uri_convert($input_path_base . ".psf");
# 	print COMPMAP "$remote_file ${remote_root}/tmp/${ipp_disk}\n";

	$nInputs++;
    }

    close(COMPMAP);

    my $mk_mdc_command = "mkdir -p $remote_outdir && sc_mk_staticsky_mdc.pl --remote_root ${remote_root} --compmap ${remote_root}/tmp/${component_map} $input_path_base_string > ${remote_outroot}.in.mdc";
    my $recipe_psphot = $ipprc->reduction($reduction, 'STACKPHOT_PSPHOT');
    my $recipe_ppsub  = $ipprc->reduction($reduction, 'STACKPHOT_PPSUB');
    my $recipe_ppstack= $ipprc->reduction($reduction, 'STACKPHOT_PPSTACK'); # Recipe to use

    my $pps_command  = " psphotStack -input ${remote_outroot}.in.mdc ";
    $pps_command    .= " ${remote_outroot} ";
    $pps_command    .= " -recipe PSPHOT $recipe_psphot ";
    $pps_command    .= " -recipe PPSUB  $recipe_ppsub ";
    $pps_command    .= " -recipe PPSTACK $recipe_ppstack ";
    $pps_command    .= " -tracedest ${remote_outroot}.trace -log ${remote_outroot}.log ";
    $pps_command    .= " -threads $threads_req ";
    $pps_command    .= " -dumpconfig ${remote_outroot}.psphotStack.mdc >& ${remote_outroot}.log2 ";

    my $post_cmd_echo = " echo -n \"staticskytool  -addresult -sky_id $sky_id ";
    $post_cmd_echo   .= " -path_base $ipp_outroot -num_inputs $nInputs ";
    $post_cmd_echo   .=  " -dbname $dbname " if defined $dbname;
    $post_cmd_echo   .= " -hostname $remote_hostname -dtime_script 0 \" > ${remote_outroot}.dbinfo ";

# CZW: 2014-09-26 We currently do not save any information into the database here, so I'm leaving the dummy from stack.
#    my $post_cmd_SfM  = " ppStatsFromMetadata ${remote_outroot}.stats - STACK_SKYCELL >> ${remote_outroot}.dbinfo ";
#    print CONFIG "${mk_mdc_command} && ${pps_command} && ${post_cmd_echo} && ${post_cmd_SfM} ";

    print CONFIG "${mk_mdc_command} && ${pps_command} && ${post_cmd_echo} ";
    $job_index++;

    # Determine which output files need to be returned
    foreach my $component(@return_component_list) {
        my $filename = $ipprc->filename($component,$ipp_outroot);
        my ($ipp_disk, $remote_disk) = &uri_to_outputs_for_return( $filename);
        my $remote_outroot_dir = dirname($ipp_disk);
        print CONFIG " && mkdir -p ${remote_root}/tmp/${remote_outroot_dir} && ln -sf $remote_disk ${remote_root}/tmp/${ipp_disk} && touch $remote_disk ";
    }
    # But this creates outputs for each input stack, so iterate over those as well.
    foreach my $inputEntry (@{ $inputData }) {
	my $stack_id = $inputEntry->{stack_id};
	foreach my $component (@return_component_list_per_input) {
	    my $filename = $ipprc->filename($component,$ipp_outroot,$stack_id);
	    my ($ipp_disk, $remote_disk) = &uri_to_outputs_for_return( $filename );
	    my $remote_outroot_dir = dirname($ipp_disk);
	    print CONFIG " && mkdir -p ${remote_root}/tmp/${remote_outroot_dir} && ln -sf $remote_disk ${remote_root}/tmp/${ipp_disk} && touch $remote_disk ";
	}
    }
	
    print CONFIG "\n";
}
close(CONFIG);
close(TRANSFER);
close(CHECK);
close(RETURN);
close(GENERATE);

unless($no_update) {
    my $command = "remotetool -updatecomponent -remote_id $remote_id -stage_id $sky_id ";
    $command .= " -set_jobs $job_index";
    $command .= " -set_path_base $path_base";
    $command .= " -set_state prep_done";
    $command .= " -dbname $dbname " if defined $dbname;

    system($command);
}

## Common SC routines

# KEEP
sub uri_convert { # (ipp_disk,remote_disk) = uri_convert(neb_uri);
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri );
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

# KEEP
sub uri_convert_and_create { # (ipp_disk,remote_disk) = uri_convert_and_create(neb_uri); ipp_disk is created if it doesn't exist
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri , 1);
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $sky_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

# KEEP
sub uri_to_outputs { # (ipp_disk,remote_disk) = uri_to_output(neb_uri); Appends to TRANSFER and CHECK filehandles
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert( $neb_uri );

    print TRANSFER "$ipp_disk\n";
    print CHECK    "$remote_disk\n";
    return($ipp_disk,$remote_disk);
}

# KEEP
sub uri_to_outputs_for_return { # (ipp_disk,remote_disk) = uri_to_outputs_for_return(neb_uri); create ipp_disk, append to RETURN and GENERATE
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert_and_create( $neb_uri );

    print RETURN "$ipp_disk\n";
    print GENERATE "$remote_disk\n";
    return($ipp_disk,$remote_disk);
}

# KEEP
sub uri_local_to_remote { #(remote_uri) = uri_local_to_remote(local_neb_uri);
    # This needs to replace the nebulous tag with the remote root.
    my $local_uri = shift;
    $local_uri =~ s%^.*?/%%; # neb:/
    $local_uri =~ s%^.*?/%%; # /
    $local_uri =~ s%^.*?/%%; # @HOST@.0/
    my $remote_uri = "${remote_root}/" . $local_uri;

    return($remote_uri);
}

sub my_die { # exit with status; my_die(message,stage_id,exit_code,exit_status);
    my $msg = shift;
    my $remote_id  = shift;
    my $stage_id  = shift;
    my $exit_code = shift;
    my $exit_state = shift;

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    if (defined $remote_id and defined $stage_id and not $no_update) {
        my $command = "remotetool -updatecomponent -remote_id $remote_id -stage_id $stage_id";
        $command .= " -set_state $exit_state " if defined $exit_state;
        $command .= " -dbname $dbname " if defined $dbname;

        system($command);
    }

    exit($exit_code);
}
