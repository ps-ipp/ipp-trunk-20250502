#! /usr/bin/env perl

# generate the input & output files lists and commands for a single diffRun

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
my $difftool    = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my @ARGS = @ARGV;

my ($remote_id,$diff_id,$camera,$dbname,$verbose,$path_base,$no_update,$cmd_recipe);
GetOptions(
    'remote_id=s'    => \$remote_id,
    'diff_id=s'      => \$diff_id,
    'camera|c=s'     => \$camera,
    'dbname|d=s'     => \$dbname,
    'recipe=s'       => \$cmd_recipe,
    'path_base=s'    => \$path_base,
    'no_update'      => \$no_update,
    'verbose'        => \$verbose,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id -diff_id --camera --dbname --path_base --recipe", -exitval => 3) unless
    defined($remote_id) and
    defined($diff_id) and
    defined($camera) and
    defined($path_base) and
    defined($cmd_recipe) and
    defined($dbname);

print "FULL COMMAND: $0 @ARGS\n\n";

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

# my $remote_root = '/lustre/scratch1/turquoise/watersc1/ps1/'; # Far side destination base location
my $remote_root     = $remote_recipe{REMOTE_ROOT};
my $remote_hostname = $remote_recipe{REMOTE_HOSTNAME};         # Name of the remote node.
my $threads_req     = 4;                      # How many threads are we going to use?

my $fail_state = "prep_fail";


my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $remote_id, $diff_id, $PS_EXIT_CONFIG_ERROR, $fail_state);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my @return_component_list = ("DBINFO.EXP", "PPSUB.CONFIG", "LOG.EXP","TRACE.EXP","SKYCELL.STATS","PPSUB.OUTPUT.KERNELS",
			     "PPSUB.OUTPUT.SOURCES", "PPSUB.INVERSE.SOURCES",
			     "PSPHOT.BACKMDL.MEF", "PSPHOT.PSF.SKY.SAVE");

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

open(TRANSFER, ">$disk_transfer")  || &my_die("Couldn't open file? $disk_transfer",$remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CHECK,    ">$disk_check")     || &my_die("Couldn't open file? $disk_check",   $remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CONFIG,   ">$disk_config")    || &my_die("Couldn't open file? $disk_config",  $remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(GENERATE, ">$disk_generate")  || &my_die("Couldn't open file? $disk_generate",$remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(RETURN,   ">$disk_return")    || &my_die("Couldn't open file? $disk_return",  $remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state);

my $job_index = 0;

# STEP 1: Get exposure level information from the diffRun we're working from.
my ($diffRun,$diffData,%diffHash);
{
    # Get the Run level information first.
    my $command = "$difftool -todiffskyfile -diff_id $diff_id ";
    $command   .= " -dbname $dbname " if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run difftool -todiffskyfile ", $remote_id, $diff_id, $error_code, $fail_state);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine diff run information.", $remote_id, $diff_id, $PS_EXIT_PROG_ERROR, $fail_state);
    $diffRun = parse_md_list($MDlist);
    

    # This actually returns all the individual diff/skyfiles that comprise this run.  Because consistency.
    $command = "$difftool -inputskyfile -diff_id $diff_id ";
    $command   .= " -dbname $dbname " if defined($dbname);

    ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to run difftool -pendingimfile ", $remote_id, $diff_id, $error_code, $fail_state);
    }

    $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine diff component information.", $remote_id, $diff_id, $PS_EXIT_PROG_ERROR, $fail_state);
    $diffData = parse_md_list($MDlist);

    # Because this query returns a horrible decision, link the warp element and the stack element together.
    foreach my $diffEntry ( @{ $diffData } ) {
	my $skycell_id = $diffEntry->{skycell_id};
	my $template_v = $diffEntry->{template};

	if (defined($diffHash{$skycell_id}{$template_v})) {
	    &my_die("I have found too many entries for skycell: $skycell_id and template: $template_v.", 
		    $remote_id, $diff_id, $PS_EXIT_PROG_ERROR, $fail_state);
	}
	
	$diffHash{$skycell_id}{$template_v} = $diffEntry;
    }
}

# STEP 2: Iterate over the sub-components
{
#    foreach my $diffEntry ( @{ $diffData } ) {

    my $diffRun_prototype = ${ $diffRun }[0];
    my $reduction = $diffRun_prototype->{reduction};
    my $workdir   = $diffRun_prototype->{workdir};
    my $bothwaps  = $diffRun_prototype->{bothways};
    my $tess_id   = $diffRun_prototype->{tess_id};


#    my $recipe_ppSub = $ipprc->reduction($reduction, 'DIFF_PPSUB');
#    my $recipe_psphot= $ipprc->reduction($reduction, 'DIFF_PSPHOT');
#    my $recipe_ppstats= 'DIFFSTATS';
#     &my_die("Unable to find recipes for reduction $reduction",
# 	    $remote_id, $diff_id, $PS_EXIT_CONFIG_ERROR, $fail_state) 
# 	unless ((defined($recipe_ppSub))&&(defined($recipe_psphot)));


    foreach my $diffRun_entry (@{ $diffRun }) {
	my $skycell_id = $diffRun_entry->{skycell_id};
	my $path_base  = $diffRun_entry->{path_base};
	my $diff_skyfile_id = $diffRun_entry->{diff_skyfile_id};

	my $input    = $diffHash{$skycell_id}{0};  # This is the warp
	my $template = $diffHash{$skycell_id}{1};  # This is the stack

	# Check that everything matches
	unless ((defined($template))&&
		(defined($input))&&
		($template->{skycell_id} eq $skycell_id)&&
		($input->{skycell_id}    eq $skycell_id)&&
		($template->{tess_id}    eq $tess_id)&&
		($input->{tess_id}       eq $tess_id)&&
		($template->{camera}     eq $input->{camera})) {
	    &my_die("Something doesn't match or isn't defined for $skycell_id", 
		    $remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state) ;
	}

	my $ipp_outroot = "${workdir}/${tess_id}/${skycell_id}/${tess_id}.${skycell_id}.WS.dif.${diff_id}"; 
        print "$ipp_outroot\n";
        my $remote_outroot = &uri_local_to_remote($ipp_outroot);

	my $ipp_inimage = $ipprc->filename("PSWARP.OUTPUT",$input->{path_base});
	my $ipp_inmask  = $ipprc->filename("PSWARP.OUTPUT.MASK",$input->{path_base});
	my $ipp_inwt    = $ipprc->filename("PSWARP.OUTPUT.VARIANCE",$input->{path_base});
	my $ipp_insourc = $ipprc->filename("PSWARP.OUTPUT.SOURCES",$input->{path_base});

	my $remote_inimage = &uri_local_to_remote($ipp_inimage);
	my $remote_inmask  = &uri_local_to_remote($ipp_inmask);
	my $remote_inwt    = &uri_local_to_remote($ipp_inwt);
	my $remote_insourc = &uri_local_to_remote($ipp_insourc);

	my $ipp_templimage = $ipprc->filename("PPSTACK.UNCONV",$template->{path_base});
	my $ipp_templmask  = $ipprc->filename("PPSTACK.UNCONV.MASK",$template->{path_base});
	my $ipp_templwt    = $ipprc->filename("PPSTACK.UNCONV.VARIANCE",$template->{path_base});
	#my $ipp_templsourc = $ipprc->filename("PSPHOT.OUT.CMF.MEF",$input->{path_base});
	my $ipp_templsourc = $ipp_insourc; # Just use that everywhere.

	my $remote_templimage = &uri_local_to_remote($ipp_templimage);
	my $remote_templmask  = &uri_local_to_remote($ipp_templmask);
	my $remote_templwt    = &uri_local_to_remote($ipp_templwt);
	my $remote_templsourc = &uri_local_to_remote($ipp_templsourc);
	
	my $pre_command     = " sc_check_diff.pl --diff_id ${diff_id} --remote_root ${remote_root} --skycell_id ${skycell_id} --dbname ${dbname} --out_path_base ${remote_outroot} $remote_inimage $remote_inmask $remote_inwt $remote_insourc $remote_templimage $remote_templmask $remote_templwt $remote_templsourc ";

	# make any directory we may need
        my $remote_outroot_dir = dirname($remote_outroot);
        my $dir_command =  "mkdir -p $remote_outroot_dir";


	# Convert these here
        my $psdiff_command  = " ppSub ${remote_outroot} ";
        $psdiff_command    .= " -inimage $remote_inimage -refimage $remote_templimage ";
        $psdiff_command    .= " -inmask $remote_inmask -refmask $remote_templmask ";
        $psdiff_command    .= " -invariance $remote_inwt -refvariance $remote_templwt ";
	$psdiff_command    .= " -insources $remote_insourc -refsources $remote_templsourc ";
        $psdiff_command    .= " -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE ";
        $psdiff_command    .= " -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF ";
        $psdiff_command    .= " -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF ";
        $psdiff_command    .= " -recipe PPSUB WARPSTACK_PV3 ";
        $psdiff_command    .= " -recipe PSPHOT DIFF_PV3 ";
        $psdiff_command    .= " -recipe PPSTATS DIFFSTATS ";
        $psdiff_command    .= " -tracedest ${remote_outroot}.trace -log ${remote_outroot}.log ";
        $psdiff_command    .= " -threads $threads_req "; # -image_id ${image_id} -source_id ${source_id} ";
        $psdiff_command    .= " -recipe PPSTATS DIFFSTATS ";
        $psdiff_command    .= " -dumpconfig ${remote_outroot}.ppSub.mdc -stats ${remote_outroot}.stats ";
#	$psdiff_command    .= " -save-inconv "; # I guess? Nope.  It makes images we don't plan to transfer back.
	$psdiff_command    .= " -photometry -inverse ";
	$psdiff_command    .= " -image_id $diff_skyfile_id "; # I have no clue what this does.

#       print "$psdiff_command \n";

        my $post_cmd_echo = " echo -n \"difftool  -adddiffskyfile -diff_id $diff_id -skycell_id $skycell_id ";
        $post_cmd_echo   .=  " -dbname $dbname " if defined $dbname;
#        $post_cmd_echo   .= " -uri ${ipp_outroot}.fits ";
        $post_cmd_echo   .= " -path_base $ipp_outroot -hostname $remote_hostname -dtime_script 0 \" > ${remote_outroot}.dbinfo ";

        my $post_cmd_SfM  = " ppStatsFromMetadata ${remote_outroot}.stats - DIFF_SKYCELL >> ${remote_outroot}.dbinfo ";

        print CONFIG " ${dir_command} && ${pre_command} && ${psdiff_command} && ${post_cmd_echo} && ${post_cmd_SfM} ";
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
    my $command = "remotetool -updatecomponent -remote_id $remote_id -stage_id $diff_id ";
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
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state);
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
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $diff_id, $PS_EXIT_SYS_ERROR, $fail_state);
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
