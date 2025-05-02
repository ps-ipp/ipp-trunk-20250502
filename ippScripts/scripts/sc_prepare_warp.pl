#! /usr/bin/env perl

# generate the input & output files lists and commands for a single warpRun

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
my $warptool    = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

my ($remote_id,$warp_id,$camera,$dbname,$verbose,$path_base,$no_update,$cmd_recipe);
GetOptions(
    'remote_id=s'    => \$remote_id,
    'warp_id=s'      => \$warp_id,
    'camera|c=s'     => \$camera,
    'dbname|d=s'     => \$dbname,
    'recipe=s'       => \$cmd_recipe,
    'path_base=s'    => \$path_base,
    'no_update'      => \$no_update,
    'verbose'        => \$verbose,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id -warp_id --camera --dbname --path_base --recipe", -exitval => 3) unless
    defined($remote_id) and
    defined($warp_id) and
    defined($camera) and
    defined($path_base) and
    defined($cmd_recipe) and
    defined($dbname);

print "FULL COMMAND: $0 @ARGS\n\n";

# Hard coded values
# my $remote_root = '/lustre/scratch1/turquoise/watersc1/ps1/'; # Far side destination base location
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

my $remote_root = $remote_recipe{REMOTE_ROOT};
my $hostname    = $remote_recipe{REMOTE_HOSTNAME};
my $have_warps  = $remote_recipe{TRANSFER_WARP_IMAGES};
my $threads_req     = 4;                      # How many threads are we going to use?

my $fail_state = "prep_fail";


my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $remote_id, $warp_id, $PS_EXIT_CONFIG_ERROR, $fail_state);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my @return_component_list = ("DBINFO.EXP", "PSWARP.CONFIG", "PSWARP.OUTPUT.SOURCES","PSPHOT.PSF.SKY.SAVE","LOG.EXP");
if ($have_warps) {
    push @return_component_list, ("PSWARP.OUTPUT", "PSWARP.OUTPUT.MASK", "PSWARP.OUTPUT.VARIANCE");
}

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

open(TRANSFER, ">$disk_transfer")  || &my_die("Couldn't open file? $disk_transfer",$remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CHECK,    ">$disk_check")     || &my_die("Couldn't open file? $disk_check",   $remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CONFIG,   ">$disk_config")    || &my_die("Couldn't open file? $disk_config",  $remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(GENERATE, ">$disk_generate")  || &my_die("Couldn't open file? $disk_generate",$remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(RETURN,   ">$disk_return")    || &my_die("Couldn't open file? $disk_return",  $remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);

my $job_index = 0;

# STEP 1: Get exposure level information from the warpRun we're working from.
my ($warpData);
{
    # This actually returns all the individual warp/skyfiles that comprise this run.  Because consistency.
    my $command = "$warptool -towarped -warp_id $warp_id ";
    $command   .= " -dbname $dbname " if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run warptool -pendingimfile ", $remote_id, $warp_id, $error_code, $fail_state);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine warp component information.", $remote_id, $warp_id, $PS_EXIT_PROG_ERROR, $fail_state);
    $warpData = parse_md_list($MDlist);
}

# STEP 2: Iterate over the sub-components
{
    foreach my $warpEntry ( @{ $warpData } ) {
        my $workdir = $warpEntry->{workdir};
        my $exp_tag = $warpEntry->{exp_tag};
        my $skycell_id = $warpEntry->{skycell_id};
        my $tess_id = $warpEntry->{tess_id};

        my $reduction = $warpEntry->{reduction};
        $reduction = 'DEFAULT' unless defined $warpEntry->{reduction};

        my $recipe_pswarp = $ipprc->reduction($reduction, 'WARP_PSWARP'); # Recipe to use
        if ($recipe_pswarp eq "") {
            &my_die("Unable to determine PSWARP recipe for $reduction", $remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);
        }

        my $ipp_outroot = "${workdir}/${exp_tag}/${exp_tag}.wrp.${warp_id}.${skycell_id}";
        print "$ipp_outroot\n";
        my $remote_outroot = &uri_local_to_remote($ipp_outroot);

        my $command = "$warptool -scmap -warp_id $warp_id -skycell_id ${skycell_id}";
        $command   .= " -dbname $dbname " if defined($dbname);

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to run warptool -pendingimfile: $command ", $remote_id, $warp_id, $error_code, $fail_state);
        }

        my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
            &my_die("Unable to determine warp component information.", $remote_id, $warp_id, $PS_EXIT_PROG_ERROR, $fail_state);
        my $inData = parse_md_list($MDlist);

        my $pre_cmd_ims = "ls -1 ";
        my $pre_cmd_masks= "ls -1 ";
        my $pre_cmd_vars = "ls -1 ";
        my $pre_cmd_astrom = "";

        foreach my $imfile (@{ $inData }) {
            my $image = $ipprc->filename("PPIMAGE.CHIP", $imfile->{chip_path_base}, $imfile->{class_id});
            my $mask  = $ipprc->filename("PSASTRO.OUTPUT.MASK", $imfile->{cam_path_base}, $imfile->{class_id});
            my $var   = $ipprc->filename("PPIMAGE.CHIP.VARIANCE", $imfile->{chip_path_base}, $imfile->{class_id});

            my $astrom= $ipprc->filename("PSASTRO.OUTPUT", $imfile->{cam_path_base});

            my $remote_image = &uri_local_to_remote($image);
            my $remote_mask  = &uri_local_to_remote($mask);
            my $remote_var   = &uri_local_to_remote($var);
            my $remote_astrom= &uri_local_to_remote($astrom);

            $pre_cmd_ims     .= " $remote_image ";
            $pre_cmd_masks   .= " $remote_mask  ";
            $pre_cmd_vars    .= " $remote_var   ";
            $pre_cmd_astrom   = "echo $remote_astrom > ${remote_outroot}.astrom ";
        }
        $pre_cmd_ims   .= "  > ${remote_outroot}.imlist ";
        $pre_cmd_masks .= "  > ${remote_outroot}.masklist ";
        $pre_cmd_vars  .= "  > ${remote_outroot}.varlist ";

        my $skycell_command = " dvoImageExtract -D CATDIR /turquoise/usr/projects/ps1/watersc1/tess/${tess_id} $skycell_id -o ${remote_outroot}.skyfile ";

        my $pswarp_command  = " pswarp -list ${remote_outroot}.imlist ";
        $pswarp_command    .= " -masklist ${remote_outroot}.masklist -variancelist ${remote_outroot}.varlist ";
        $pswarp_command    .= " -astromlist ${remote_outroot}.astrom ";
        $pswarp_command    .= " ${remote_outroot} ${remote_outroot}.skyfile ";
        $pswarp_command    .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE ";
        $pswarp_command    .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF ";
        $pswarp_command    .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF ";
        $pswarp_command    .= " -F SOURCE.PLOT.MOMENTS SOURCE.PLOT.SKY.MOMENTS ";
        $pswarp_command    .= " -F SOURCE.PLOT.PSFMODEL SOURCE.PLOT.SKY.PSFMODEL ";
        $pswarp_command    .= " -F SOURCE.PLOT.APRESID SOURCE.PLOT.SKY.APRESID ";
        $pswarp_command    .= " -recipe PSWARP $recipe_pswarp ";
        $pswarp_command    .= " -tracedest ${remote_outroot}.trace -log ${remote_outroot}.log ";
        $pswarp_command    .= " -threads $threads_req "; # -image_id ${image_id} -source_id ${source_id} ";
        $pswarp_command    .= " -recipe PPSTATS WARPSTATS ";
        $pswarp_command    .= " -dumpconfig ${remote_outroot}.pswarp.mdc -stats ${remote_outroot}.stats ";

#       print "$pswarp_command \n";

        my $post_cmd_echo = " echo -n \"warptool  -addwarped -warp_id $warp_id -skycell_id $skycell_id -tess_id $tess_id ";
        $post_cmd_echo   .=  " -dbname $dbname " if defined $dbname;
        $post_cmd_echo   .= " -uri ${ipp_outroot}.fits ";
        $post_cmd_echo   .= " -path_base $ipp_outroot -hostname $remote_hostname -dtime_script 0 \" > ${remote_outroot}.dbinfo ";

        my $post_cmd_SfM  = " ppStatsFromMetadata ${remote_outroot}.stats - WARP_SKYCELL >> ${remote_outroot}.dbinfo ";

        print CONFIG "${pre_cmd_ims} && ${pre_cmd_masks} && ${pre_cmd_vars} && ${pre_cmd_astrom} && ${skycell_command} && ${pswarp_command} && ${post_cmd_echo} && ${post_cmd_SfM} ";
        $job_index++;

        # Determine which output files need to be returned
        foreach my $component(@return_component_list) {
            my $filename = $ipprc->filename($component,$ipp_outroot,$skycell_id);
            my ($ipp_disk, $remote_disk) = &uri_to_outputs_for_return( $filename);
            my $remote_outroot_dir = dirname($ipp_disk);
            print CONFIG " && mkdir -p ${remote_root}/tmp/${remote_outroot_dir} && ln -sf $remote_disk ${remote_root}/tmp/${ipp_disk} && touch $remote_disk ";
        }
        print CONFIG "\n";
#       die();
    }
}
close(CONFIG);
close(TRANSFER);
close(CHECK);
close(RETURN);
close(GENERATE);

unless($no_update) {
    my $command = "remotetool -updatecomponent -remote_id $remote_id -stage_id $warp_id ";
    $command .= " -set_jobs $job_index";
    $command .= " -set_path_base $path_base";
    $command .= " -set_state prep_done";
    $command .= " -dbname $dbname " if defined $dbname;

    system($command);
}
exit (0);

## Common SC routines

sub uri_convert { # (ipp_disk,remote_disk) = uri_convert(neb_uri);
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri );
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);
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
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $warp_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

sub uri_to_outputs { # (ipp_disk,remote_disk) = uri_to_output(neb_uri); Appends to TRANSFER and CHECK filehandles
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = &uri_convert( $neb_uri );

    print TRANSFER "$ipp_disk\n";
    print CHECK    "$remote_disk\n";
    return($ipp_disk,$remote_disk);
}

sub uri_to_outputs_for_return { # (ipp_disk,remote_disk) = uri_to_outputs_for_return(neb_uri); create ipp_disk, append to RETURN and GENERATE
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = &uri_convert_and_create( $neb_uri );

    print RETURN "$ipp_disk\n";
    print GENERATE "$remote_disk\n";
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
