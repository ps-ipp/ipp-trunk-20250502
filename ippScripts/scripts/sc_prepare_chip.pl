#! /usr/bin/env perl

# generate the input & output files lists and commands for a single chipRun

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
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $detselect= can_run('detselect') or (warn "Can't find detselect" and $missing_tools = 1);
my $remotetool = can_run('remotetool') or (warn "Can't find remotetool" and $missing_tools = 1);
my $ipp_burntool_fix = can_run('ipp_apply_burntool_fix.pl') or (warn "Can't find ipp_apply_burntool_fix.pl" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Options
my ($remote_id,$chip_id,$camera,$dbname,$path_base,$no_update,$verbose,$dbverbose,$cmd_recipe);
GetOptions(
    'remote_id=s'    => \$remote_id,
    'chip_id=s'      => \$chip_id,
    'camera|c=s'     => \$camera,
    'dbname|d=s'     => \$dbname,
    'recipe=s'       => \$cmd_recipe,
    'path_base=s'    => \$path_base,
    'no_update'      => \$no_update,
    'verbose'        => \$verbose,
    'dbverbose'      => \$dbverbose,
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --remote_id --chip_id --camera --dbname --path_base --recipe", -exitval => 3) unless
    defined($remote_id) and
    defined($chip_id) and
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
my $remote_raw      = "${remote_root}/tmp/";  # Directory to find raw data in.
my $threads_req     = 4;

my $fail_state = "prep_fail";
my $hard_fail_state = "fail";

my $ipprc = PS::IPP::Config->new( $camera ) or &my_die( "Unable to set up", $remote_id, $chip_id, $PS_EXIT_CONFIG_ERROR, $fail_state);

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Detrend concept holders (name, ppImage option)
my %det_types = (
    'MASK'      => '-mask',
    'FLAT'      => '-flat',
    'DARK'      => '-dark',
    'VIDEODARK' => '-dark',
    'LINEARITY' => '-linearity',
    'FRINGE'    => '-fringe',
    'NOISEMAP'  => '-noisemap'
    );

my @return_component_list = ("DBINFO.IMFILE","PPIMAGE.STATS","LOG.IMFILE","PPIMAGE.BACKMDL","PPIMAGE.PATTERN");

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

my (undef, $remote_config) = uri_convert($uri_config); # Needs to be done after we've created it.

open(TRANSFER, ">$disk_transfer")  || &my_die("Couldn't open file? $disk_transfer", $remote_id, $chip_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CHECK,    ">$disk_check")     || &my_die("Couldn't open file? $disk_check",    $remote_id, $chip_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(CONFIG,   ">$disk_config")    || &my_die("Couldn't open file? $disk_config",   $remote_id, $chip_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(GENERATE, ">$disk_generate")  || &my_die("Couldn't open file? $disk_generate", $remote_id, $chip_id, $PS_EXIT_SYS_ERROR, $fail_state);
open(RETURN,   ">$disk_return")    || &my_die("Couldn't open file? $disk_return",   $remote_id, $chip_id, $PS_EXIT_SYS_ERROR, $fail_state);

my $job_index = 0;

# STEP 1 : Get exposure level information for this chipRun
my ($exp_id, $filter, $altfilt, $dateobs);
{
    my $command = "$chiptool -listrun -chip_id $chip_id ";
    $command   .= " -dbname $dbname " if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $dbverbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);

        &my_die("Unable to run chiptool to determine stage parameters.", $remote_id, $chip_id, $error_code, $fail_state);
    }

    # Parse chipRun level data to determine exposure information
    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to determine chip component information.", $remote_id, $chip_id, $PS_EXIT_CONFIG_ERROR, $fail_state);
    my $metadata = parse_md_list($MDlist);
    my $chipEntry = $metadata->[0];

    $exp_id  = $chipEntry->{exp_id};
    $filter  = $chipEntry->{filter};
    $altfilt = $filter;
    $altfilt =~ s/.00000//;
    $dateobs = $chipEntry->{dateobs};
}

# STEP 2 : select the det_ids for each desired det_type (appropriate to this exposure)
my %det_ids = ();
my %det_iters = ();

foreach my $det_type (keys (%det_types)) {
    if (($filter !~ /y/) && ($det_type eq 'FRINGE')) { next; } # We can skip fringe for all but y

    my $command = "detselect -search -det_type $det_type -time $dateobs";
    $command   .= " -dbname $dbname " if defined($dbname);

    if (($det_type eq 'FLAT') || ($det_type eq 'FRINGE')) {
        $command .= " -filter $altfilt";
    }

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $dbverbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);

        &my_die("No valid detrend available for this image: $det_type", $remote_id, $chip_id, $error_code, $fail_state);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Could not parse detrend information for this image: $det_type", $remote_id, $chip_id, $PS_EXIT_CONFIG_ERROR, $fail_state);
    my $metadata = parse_md_list($MDlist);
    my $detEntry = $metadata->[0];

    if ($verbose) {
        print STDERR "det_id: $detEntry->{det_id}\n";
        print STDERR "det_iter: $detEntry->{iteration}\n";
    }

    # save det_id and iteration
    $det_ids{$det_type} = $detEntry->{det_id};
    $det_iters{$det_type} = $detEntry->{iteration};
}

# Step 3: Iterate over the sub-components
{
    # select chipProcessedImfile entries
    my $command = "$chiptool -pendingimfile -chip_id $chip_id";
    $command   .= " -dbname $dbname "   if defined($dbname);

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $dbverbose);

    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);

        &my_die("Unable to run chiptool -pendingimfile", $remote_id, $chip_id, $error_code, $fail_state);
    }

    my $MDlist = $mdcParser->parse(join "", @$stdout_buf) or
        &my_die("Unable to parse chiptool -pendingimfile information.", $remote_id, $chip_id, $PS_EXIT_CONFIG_ERROR, $fail_state);
    my $metadata = parse_md_list($MDlist);

    # Iterate over the chipProcessedImfile level data.
    foreach my $chipEntry (@$metadata) {
        # Get information we need to pass to ppImage
        my $uri            = $chipEntry->{uri};
        my $class_id       = $chipEntry->{class_id};
        my $video_cells    = $chipEntry->{video_cells};
        my $reduction      = $chipEntry->{reduction};
        my $exp_tag        = $chipEntry->{exp_tag};
        my $workdir        = $chipEntry->{workdir};
        my $chip_imfile_id = $chipEntry->{chip_imfile_id};

        # Process the image and burntool table
	# IMAGE CHECK: Look up the image location, prove it exists, and if not, abort the prep for this object.
        my ($ipp_uri, $remote_uri) = uri_convert($uri);
	unless (-e $ipp_uri) {
	    # Try to fix it.
	    system("neb-repair $uri");
	    ($ipp_uri, $remote_uri) = uri_convert($uri);
	    unless (-e $ipp_uri) {
		# This image file is missing, so it will require manual intervention.
		&my_die("Required image file $ipp_uri ($chip_id, $class_id) not found.  Stopping this prep.",
			$remote_id,$chip_id,$PS_EXIT_CONFIG_ERROR,$hard_fail_state);
	    }
	}


        # Determine the value of a "good" burntool run.
        my $config_cmd = "$ppConfigDump -camera $camera -get-key BURNTOOL.STATE.GOOD";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run ( command => $config_cmd, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform ppConfigDump: $error_code", $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
        }

        my $recipeData = $mdcParser->parse(join "", @$stdout_buf) or
            &my_die("Unable to parse metadata config doc", $exp_id, $class_id, $PS_EXIT_SYS_ERROR);

        my $burntoolStateGood = 999;
        foreach my $cfg (@$recipeData) {
            if ($cfg->{name} eq 'BURNTOOL.STATE.GOOD') {
                $burntoolStateGood = $cfg->{value};
            }
        }
        if ($burntoolStateGood == 999) {
            &my_die("Failed to determine BURNTOOL.STATE.GOOD", $exp_id, $class_id, $PS_EXIT_SYS_ERROR);
        }


	# The image exists, so continue.  This resets the variables, but also outputs the file locations.
	($ipp_uri, $remote_uri) = uri_to_outputs_raw($uri);
        my $btt = $uri;
        if($burntoolStateGood == 15) {
            $btt =~ s/fits$/burn.v15.tbl/;    
        } else {
            $btt =~ s/fits$/burn.tbl/;	    
        }
	
	# Check burntool table for existance, and if it doesn't, regenerate it.
        my ($ipp_btt, $remote_btt) = uri_convert($btt);
	unless (-e $ipp_btt) {
	    my $exp_name = $chipEntry->{exp_name};
	    my $burntool_command = "$ipp_burntool_fix --exp_name $exp_name --class_id $class_id";
	    $burntool_command   .= " -dbname $dbname" if defined($dbname);
	    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		run(command => $burntool_command, verbose => $dbverbose);
	    # That command repairs the tables, but the instance we learned about may not be correct.  
	    # Look up the new one.
	    ($ipp_btt, $remote_btt) = uri_convert($btt);
	    
	    unless (-e $ipp_btt) {
		&my_die("Attempted regeneration of burntool table ($chip_id, $class_id) has failed.  Stopping this prep.", 
			$remote_id,$chip_id, $PS_EXIT_CONFIG_ERROR, $hard_fail_state);
	    }
	}
        ($ipp_btt, $remote_btt) = uri_to_outputs_raw($btt);

        # Initialize the ppI command
        my $ppImage_command = "ppImage -file $remote_uri";
        $ppImage_command   .= " -burntool $remote_btt ";

        foreach my $det_type (keys (%det_types)) {
            if (( $video_cells) && ($det_type eq 'DARK')) { next; }
            if ((!$video_cells) && ($det_type eq 'VIDEODARK')) { next; }

            my $det_id   = $det_ids{$det_type};
            my $det_iter = $det_iters{$det_type};

            if (not defined $det_id) { next; }

            if ($verbose) { print STDERR "det_type, det_id, det_iter: $det_type, $det_id, $det_iter\n"; }

            # Add detrend information to the command line
            my $detselect_command = "detselect -select ";
            $detselect_command   .= " -det_id $det_id";
            $detselect_command   .= " -iteration $det_iter";
            $detselect_command   .= " -class_id $class_id";
            $detselect_command   .= " -dbname $dbname " if defined($dbname);

            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $detselect_command, verbose => $dbverbose);

            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);

                &my_die("No valid detrend available for this image: $det_type $det_id $det_iter", $remote_id, $chip_id, $error_code, $fail_state);
            }

            my $detMDlist = $mdcParser->parse(join "", @$stdout_buf) or
                &my_die("Could not parse detrend information for this image: $det_type $det_id $det_iter", $remote_id, $chip_id, $PS_EXIT_CONFIG_ERROR, $fail_state);
            my $detImfileMD = parse_md_list($detMDlist);
            my $detImfile = $detImfileMD->[0];
            my $duri = $detImfile->{uri};

            my $det_code = $det_types{$det_type};
            my ($ipp_det_uri, $remote_det_uri) = uri_to_outputs_raw($duri);
            $ppImage_command .= " $det_code $remote_det_uri ";
        }

        # Add output root
        my $ipp_outroot = "${workdir}/${exp_tag}/${exp_tag}.ch.${chip_id}";
        my $remote_outroot = uri_local_to_remote($ipp_outroot);
        $ppImage_command .= " $remote_outroot ";
        print STDERR "$remote_outroot $ipp_outroot $class_id\n";
        # Complete reduction information.
        $reduction = 'DEFAULT' unless defined $reduction;
        my $recipe_ppImage = $ipprc->reduction($reduction, 'CHIP_PPIMAGE'); # Recipe to use for ppImage
        my $recipe_psphot  = $ipprc->reduction($reduction, 'CHIP_PSPHOT'); # Recipe to use for psphot

        $ppImage_command .= " -recipe PPIMAGE $recipe_ppImage ";
        $ppImage_command .= " -recipe PSPHOT $recipe_psphot ";
        $ppImage_command .= " -recipe PPSTATS CHIPSTATS -stats ${remote_outroot}.${class_id}.stats ";
        $ppImage_command .= " -threads $threads_req ";
        $ppImage_command .= " -image_id $chip_imfile_id ";
        $ppImage_command .= " -tracedest ${remote_outroot}.${class_id}.trace ";
        $ppImage_command .= " -log ${remote_outroot}.${class_id}.log ";

        # Calculate pre and post commands
        my $remote_outroot_dir = dirname($remote_outroot);
        my $pre_command =  "mkdir -p $remote_outroot_dir";

        my $post_commandA = "chiptool -addprocessedimfile -exp_id $exp_id -chip_id $chip_id -class_id $class_id ";
        $post_commandA   .= " -uri ${ipp_outroot}.ch.${class_id}.ch.fits -path_base $ipp_outroot ";
        $post_commandA   .= " -magicked 0 -hostname $remote_hostname -dtime_script 0 ";
        $post_commandA   .= " -dbname $dbname " if defined $dbname;

        my $post_commandB = "echo -n \"$post_commandA\" > ${remote_outroot}.${class_id}.dbinfo  ";
        my $post_commandC = "ppStatsFromMetadata ${remote_outroot}.${class_id}.stats - CHIP_IMFILE >> ${remote_outroot}.${class_id}.dbinfo";

        $job_index++;

        print CONFIG "${pre_command} && ${ppImage_command} && ${post_commandB} && ${post_commandC}";

        # Determine which output files need to be returned.
        foreach my $component (@return_component_list) {
            my $filename = $ipprc->filename($component, $ipp_outroot, $class_id);
            my ($ipp_disk, $remote_disk) = uri_to_outputs_for_return( $filename );
            my $remote_outroot_dir = dirname($ipp_disk);
            print CONFIG " && mkdir -p ${remote_root}/tmp/${remote_outroot_dir} && ln -sf $remote_disk ${remote_root}/tmp/${ipp_disk} && touch $remote_disk ";
        }
        print CONFIG "\n";
    }
}

close(CONFIG);
close(TRANSFER);
close(CHECK);
close(RETURN);
close(GENERATE);

## We're done here. The execution and handling are done elsewhere.
# Quick review:
# new -> pending -> run -> full
# auth
unless($no_update) {
    my $command = "remotetool -updatecomponent -remote_id $remote_id -stage_id $chip_id ";
    $command .= " -set_jobs $job_index";
    $command .= " -set_path_base $path_base";
    $command .= " -set_state prep_done";
    $command .= " -dbname $dbname " if defined $dbname;

    system($command);
}
exit (0);

sub uri_convert {
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri );
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $chip_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

sub uri_convert_and_create {
    my $neb_uri = shift;
    my $ipp_disk= $ipprc->file_resolve( $neb_uri , 1);
    my $remote_disk = $ipp_disk;

    unless(defined($ipp_disk)) {
        &my_die( "Unable to generate file for $neb_uri ", $remote_id, $chip_id, $PS_EXIT_SYS_ERROR, $fail_state);
    }

    $remote_disk =~ s%^.*/%%;   # Remove nebulous path
    $remote_disk =~ s%^\d+\.%%; # Remove ins_id
    $remote_disk =~ s%:%/%g;    # Replace colons with directories
    $remote_disk = "${remote_root}/${remote_disk}";
    return($ipp_disk,$remote_disk);
}

sub uri_to_outputs {
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert( $neb_uri );

    print TRANSFER "$ipp_disk\n";
    print CHECK    "$remote_disk\n";
    return($ipp_disk,$remote_disk);
}

sub uri_to_outputs_raw {
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert( $neb_uri );
    $remote_disk = $remote_raw . $ipp_disk;

    print TRANSFER "$ipp_disk\n";
    print CHECK    "$remote_disk\n";
    return($ipp_disk,$remote_disk);
}

sub uri_to_outputs_for_return {
    my $neb_uri = shift;
    my ($ipp_disk, $remote_disk) = uri_convert_and_create( $neb_uri );
    print RETURN "$ipp_disk\n";
    print GENERATE "$remote_disk\n";
    return($ipp_disk,$remote_disk);
}

sub uri_local_to_remote {
    # This needs to replace the nebulous tag with the remote root.
    my $local_uri = shift;
    $local_uri =~ s%^.*?/%%; # neb:/
    $local_uri =~ s%^.*?/%%; # /
    $local_uri =~ s%^.*?/%%; # @HOST@.0/
    my $remote_uri = "${remote_root}/" . $local_uri;

    return($remote_uri);
}

sub uri_remote_to_local {
    # This needs to replace the remote root directory with the nebulous tag.
    my $remote_uri = shift;
    $remote_uri =~ s%${remote_root}%%;
    my $local_uri  = "neb:///" . $remote_uri;

    return($local_uri);
}

sub my_die {
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

