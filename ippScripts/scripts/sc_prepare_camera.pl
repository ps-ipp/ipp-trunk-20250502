#! /usr/bin/env perl

# generate the input & output files lists and commands for a single camRun

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
use File::Temp qw( tempfile );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $remotetool = can_run('remotetool') or (warn "Can't find remotetool" and $missing_tools = 1);
my $camtool    = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $ppStatsFromMetadata = can_run('ppStatsFromMetadata') or (warn "Can't find ppStatsFromMetadata" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my ($remote_id,$cam_id,$camera,$dbname,$verbose,$path_base,$no_update,$cmd_recipe);
GetOptions(
    'remote_id=s'    => \$remote_id,
    'cam_id=s'       => \$cam_id,
    'camera|c=s'     => \$camera,
    'dbname|d=s'     => \$dbname,
    'recipe=s'       => \$cmd_recipe,
    'path_base=s'    => \$path_base,
    'no_update'      => \$no_update,
    'verbose'        => \$verbose,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id --cam_id --camera --dbname --path_base --recipe", -exitval => 3) unless
    defined($remote_id) and
    defined($cam_id) and
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


# my $remote_root   = '/lustre/scratch1/turquoise/watersc1/ps1/'; # Far side destination base location
my $remote_root     = $remote_recipe{REMOTE_ROOT};
my $remote_hostname = $remote_recipe{REMOTE_HOSTNAME};         # Name of the remote node.
my $threads_req     = 0;                      # How many threads are we going to use?

my $fail_state = "prep_fail";


my $ipprc = PS::IPP::Config->new( $camera ) or &my_die( "Unable to set up", $remote_id, $cam_id, $PS_EXIT_CONFIG_ERROR, $fail_state);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my @return_component_list = ("DBINFO.EXP", "PSASTRO.CONFIG", "PSASTRO.OUTPUT", "LOG.EXP", "PSASTRO.STATS");

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

open(TRANSFER, ">$disk_transfer")  || &my_die("Couldn't open file? $disk_transfer",$remote_id, $cam_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CHECK,    ">$disk_check")     || &my_die("Couldn't open file? $disk_check",   $remote_id, $cam_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CONFIG,   ">$disk_config")    || &my_die("Couldn't open file? $disk_config",  $remote_id, $cam_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(GENERATE, ">$disk_generate")  || &my_die("Couldn't open file? $disk_generate",$remote_id, $cam_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(RETURN,   ">$disk_return")    || &my_die("Couldn't open file? $disk_return",  $remote_id, $cam_id, $PS_EXIT_SYS_ERROR, $fail_state);
my %file_filter = ();

my $job_index = 0;

# STEP 1: Get exposure level information for this camRun
my ($workdir,$exp_tag,$reduction);
{

###    $command    = "$camtool -processedexp -cam_id $cam_id "; ### THIS LINE ONLY TO TEST!  
    my $command = "$camtool -pendingexp -cam_id $cam_id ";
    $command   .= " -dbname $dbname " if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 0);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);

        &my_die("Unable to run camtool to determine stage parameters.", $remote_id, $cam_id, $error_code, $fail_state);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine cam component information.", $remote_id, $cam_id, $PS_EXIT_PROG_ERROR, $fail_state);
    my $metadata = parse_md_list($MDlist);
    my $camEntry = $metadata->[0];

    $workdir   = $camEntry->{workdir};
    $exp_tag   = $camEntry->{exp_tag};
    $reduction = $camEntry->{reduction};
    $reduction = 'DEFAULT' unless defined $reduction;
    unless (defined($workdir)) {
        while( my ($k, $v) = each %$camEntry ) {
            print "key: $k, value: $v.\n";
        }
        print "%{ $camEntry }\n";
        die;
    }
}

my $ipp_outroot = "${workdir}/${exp_tag}/${exp_tag}.cm.${cam_id}";
my $remote_outroot = uri_local_to_remote($ipp_outroot);

# Step 3: Iterate over the sub-components
{
    my $command = "$camtool -pendingimfile -cam_id $cam_id ";
    $command   .= " -dbname $dbname " if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => 0);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run camtool -pendingimfile ", $remote_id, $cam_id, $error_code, $fail_state);
    }

    # We don't actually care about the input cam data other than to know which mask files to instantiate.
    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine cam component information.", $remote_id, $cam_id, $PS_EXIT_PROG_ERROR, $fail_state);

    my $metadata = parse_md_list($MDlist);
    my $chipProto = $metadata->[0];
    my $chip_path = $chipProto->{path_base};
    my $remote_chip_path = &uri_local_to_remote( $chip_path );
    my $pre_cmd_cmfs = "ls -1 ${remote_chip_path}*.cmf > ${remote_outroot}.cmflist";
    my $pre_cmd_masks= "ls -1 ${remote_chip_path}*.mk.fits > ${remote_outroot}.masklist";

    # Despite the previous comment, we do care about the output of pendingimfile, as that 
    # contains information for the exposure.
    my ($statFile, $statName) = tempfile( "/tmp/cm.$cam_id.stats.XXXX", UNLINK => 1 );
    foreach my $line (@$stdout_buf) {
        print $statFile $line;
    }
    close $statFile;
    
    # parse the stats in the metadata file
    $command = "$ppStatsFromMetadata $statName - CAMERA_EXP_IMFILE";
    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform ppStatsFromMetadata: $error_code\n");
        exit($error_code);
    }

    my $results = '';
    foreach my $line (@$stdout_buf) {
        $results .= " $line";
    }
    chomp($results);



    my $recipe_psastro = $ipprc->reduction($reduction, 'PSASTRO'); # Recipe to use
    if ($recipe_psastro eq "") {
        &my_die("Unable to determine PSASTRO recipe", $cam_id, $PS_EXIT_PROG_ERROR);
    }

    my $psastro_command = " psastro -list ${remote_outroot}.cmflist ";
    $psastro_command   .= " -masklist ${remote_outroot}.masklist ${remote_outroot} ";
#   $psastro_command   .= " -refmasklist ${remote_outroot}.masklist ${remote_outroot} ";
    $psastro_command   .= " -refmasklist /turquoise/usr/projects/ps1/watersc1/references/gpc1.refmask.list ";
    $psastro_command   .= " -kh-correct /turquoise/usr/projects/ps1/watersc1/references/khcorrect.20140606.v0.fits ";
#   $psastro_command   .= " -astrommodel /turquoise/usr/projects/ps1/watersc1/references/gpc1.20080909.asm ";
    $psastro_command   .= " -astrommodel /turquoise/usr/projects/ps1/watersc1/references/gpc1.20140505.asm ";
    $psastro_command   .= " -recipe PSASTRO $recipe_psastro ";
    $psastro_command   .= " -tracedest ${remote_outroot}.trace -log ${remote_outroot}.log ";
    $psastro_command   .= " -dumpconfig ${remote_outroot}.psastro.mdc -stats ${remote_outroot}.stats ";
    $psastro_command   .= " -recipe PPSTATS CAMSTATS ";

    my $camtool_post_cmd = "camtool -cam_id $cam_id -addprocessedexp -uri UNKNOWN ";
    $camtool_post_cmd  .=  " -dbname $dbname " if defined $dbname;
    $camtool_post_cmd  .=  " -path_base $ipp_outroot -hostname $remote_hostname -dtime_script 0 ";
    $camtool_post_cmd  .=  " $results "; # Add exposure information
    my $post_cmd_echo = " echo -n \"$camtool_post_cmd\" > ${remote_outroot}.dbinfo ";
    my $post_cmd_SfM  = " ppStatsFromMetadata ${remote_outroot}.stats - CAMERA_EXP_FPA >> ${remote_outroot}.dbinfo ";

    print CONFIG "${pre_cmd_cmfs} && ${pre_cmd_masks} && ${psastro_command} && ${post_cmd_echo} && ${post_cmd_SfM} ";
    $job_index++;

    # Determine which output files need to be returned
    foreach my $component(@return_component_list) {
        my $filename = $ipprc->filename($component,$ipp_outroot);
        my ($ipp_disk, $remote_disk) = &uri_to_outputs_for_return( $filename);
        my $remote_outroot_dir = dirname($ipp_disk);
        print CONFIG " && mkdir -p ${remote_root}/tmp/${remote_outroot_dir} && ln -sf $remote_disk ${remote_root}/tmp/${ipp_disk} && touch $remote_disk ";
    }
    foreach my $chipInfo (@{ $metadata }) {
        my $filename = $ipprc->filename("PSASTRO.OUTPUT.MASK",$ipp_outroot,$chipInfo->{class_id});
        my ($ipp_disk, $remote_disk) = &uri_to_outputs_for_return( $filename);
        my $remote_outroot_dir = dirname($ipp_disk);
        print CONFIG " && mkdir -p ${remote_root}/tmp/${remote_outroot_dir} && ln -sf $remote_disk ${remote_root}/tmp/${ipp_disk} && touch $remote_disk ";
    }
    print CONFIG "\n";
}
close(CONFIG);
close(TRANSFER);
close(CHECK);
close(RETURN);
close(GENERATE);

unless($no_update) {
    my $command = "remotetool -updatecomponent -remote_id $remote_id -stage_id $cam_id ";
    $command   .= " -set_jobs $job_index";
    $command   .= " -set_path_base $path_base";
    $command   .= " -set_state prep_done";
    $command   .= " -dbname $dbname " if defined $dbname;
    system($command);
}

exit (0);

## Common SC routines

sub uri_convert { # (ipp_disk,remote_disk) = uri_convert(neb_uri);
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri );
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $cam_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

sub uri_convert_and_create { # (ipp_disk,remote_disk) = uri_convert_and_create(neb_uri); ipp_disk is created if it doesn't exist
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri , 1);
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $cam_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

sub uri_to_outputs { # (ipp_disk,remote_disk) = uri_to_output(neb_uri); Appends to TRANSFER and CHECK filehandles
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert( $neb_uri );

    unless (exists($file_filter{$neb_uri})) {
        $file_filter{$neb_uri} = 1;
        print TRANSFER "$ipp_disk\n";
        print CHECK    "$remote_disk\n";
    }
    return($ipp_disk,$remote_disk);
}

sub uri_to_outputs_for_return { # (ipp_disk,remote_disk) = uri_to_outputs_for_return(neb_uri); create ipp_disk, append to RETURN and GENERATE
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert_and_create( $neb_uri );
    unless (exists($file_filter{$neb_uri})) {
        $file_filter{$neb_uri} = 1;
        print RETURN "$ipp_disk\n";
        print GENERATE "$remote_disk\n";
    }
    return($ipp_disk,$remote_disk);
}

sub uri_local_to_remote { #(remote_uri) = uri_local_to_remote(local_neb_uri);
    # This needs to replace the nebulous tag with the remote root.
    my $local_uri = shift;
    $local_uri =~ s%^.*?/%%; # neb:/
    $local_uri =~ s%^.*?/%%; # /
    $local_uri =~ s%^.*?/%%; # @HOST@.0/
    my $remote_uri = "${remote_root}/" . $local_uri;

    return($remote_uri);
}

sub uri_remote_to_local { #(local_neb_uri) = uri_remote_to_local(remote_uri);
    # This needs to replace the remote root directory with the nebulous tag.
    my $remote_uri = shift;
    $remote_uri =~ s%${remote_root}%%;
    my $local_uri  = "neb:///" . $remote_uri;

    return($local_uri);
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
