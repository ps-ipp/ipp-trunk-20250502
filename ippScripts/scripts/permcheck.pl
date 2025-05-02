#!/usr/bin/env perl

use strict;
use warnings FATAL => qw( all );

use vars qw( $VERSION );
$VERSION = '0.01';

use Nebulous::Client;

use Getopt::Long qw( GetOptions :config auto_help auto_version );
use Pod::Usage qw( pod2usage );
use IPC::Cmd 0.36 qw( can_run run);
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

use Digest::MD5;
use URI;

my $missing_tools = 0;
my $regtool  = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $warptool = can_run('warptool') or (warn "Can't find warptool" and $missing_tools = 1);
my $stacktool = can_run('stacktool') or (warn "Can't find stacktool" and $missing_tools = 1);
my $staticskytool = can_run('staticskytool') or (warn "Can't find staticskytool" and $missing_tools = 1);
my $fftool = can_run('fftool') or (warn "Can't find fftool" and $missing_tools = 1);
my $difftool = can_run('difftool') or (warn "Can't find difftool" and $missing_tools = 1);

my ($server,$dbname,$stage,$stage_id,$do_cull,$save_log);

$server = $ENV{'NEB_SERVER'} unless $server;
# $dbname = 'gpc1'; 
GetOptions(
    'server|s=s'     => \$server,
    'dbname=s'       => \$dbname,
    'stage=s'        => \$stage,
    'stage_id|x=s'     => \$stage_id,
    'save_log'       => \$save_log,
    'cull'           => \$do_cull,
) || pod2usage( 2 );

unless(defined($do_cull)) {
    $do_cull = 0;
}

# Option parsing
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --server", -exitval => 2 )
    unless $server;
pod2usage( -msg => "Required options: --dbname", -exitval => 2 )
    unless $dbname;
pod2usage( -msg => "Required options: --stage [camera|warp|stack|skycal]", -exitval => 2 )
    unless $stage;
pod2usage( -msg => "missing key", exitval => 2 )
    unless defined $stage_id;

my $ipprc = PS::IPP::Config->new( "GPC1" ) or die "Could not create config object.\n";

if ($save_log) {
    my $time = time();
    my $logDest = "neb://any/perm_check/${stage}/${stage}_${stage_id}";
    $ipprc->redirect_to_logfile($logDest) or die "Could not redirect output to logfile ${logDest}\n";
}


# Global options:
## Define the configuration.  Ideally, this would be retrieved from the nebulous                                                      
## database, but there are some issues with that (such as grouping the b nodes                                                        
## into a less restrictive "offsite" location).                                                                                       
my $do_ops = 1;
my $i;

# Set up nebulous db interface
my $neb = Nebulous::Client->new(
    proxy => "$server",
);
die "can't connected to Nebulous Server: $server"
    unless defined $neb;

## This new implementation is somewhat messy, but is more general and adaptable.                                                      
## First, we set up a requirement mapping, explaining where we want copies, and                                                       
## how many copies we want at each site.                                                                                              
my %requirement_map = ();
$requirement_map{ITC} = 1;
$requirement_map{OFFSITE} = 0;
$requirement_map{MRTCB} = 1;

## Second, construct a list of volumes, mapped to their site location, using the                                                      
## same site locations as in the requirement map.                                                                                     
my %volume_map = ();
for ($i = 4; $i <= 21; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'MRTCB';
}
for ($i = 23; $i <= 32; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'MRTCB';
}
for ($i = 54; $i <= 97; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'MRTCB';
}
for ($i = 100; $i <= 104; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'MRTCB';
    $vol = sprintf("ipp%03d.1",$i);
    $volume_map{$vol} = 'MRTCB';
}
for ($i = 105; $i <= 117; $i++) {
#    if ($i == 115) { next; }                                                                                                         
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'ITC';
    $vol = sprintf("ipp%03d.1",$i);
    $volume_map{$vol} = 'ITC';
}
for ($i = 118; $i <= 122; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'MRTCB';
    $vol = sprintf("ipp%03d.1",$i);
    $volume_map{$vol} = 'MRTCB';
}


for ($i = 0; $i <= 15; $i++) {
    my $loc = 'OFFSITE';
    if ($i == 6) { 
	$loc = 'MRTCB';
    } # This isn't "offsite", it's with the rest of the maui cluster.
    if ($i == 9) { next; } # Not online                                                                                               

    my $vol = sprintf("ippb%02d.0",$i);
    $volume_map{$vol} = $loc;
    $vol = sprintf("ippb%02d.1",$i);
    $volume_map{$vol} = $loc;
    if ($i <= 6) {
	$vol = sprintf("ippb%02d.2",$i);
	$volume_map{$vol} = $loc;
    }
}

## Next, get disk space values, and check which hosts are listed as available.
my %acceptable_volume = ();
my $mounts = $neb->mounts();
foreach my $vol_row (@$mounts) {
    my ($mount_point, $total, $used, $vol_id, $name, $host, $path, $allocate, $available, $xattr) = @{ $vol_row };
    if (($allocate == 1)&&($available == 1)&&( $used / $total < 0.98)) {
        $acceptable_volume{$name} = 1;
    }
    else {
        print "## $name $allocate $available $used $total\n";
        $acceptable_volume{$name} = 0;
    }
    if ($name =~ /ippb05/) {
        $acceptable_volume{$name} = 0;
    }
#    if (($name eq 'ipp106.0')||($name eq 'ipp106.1')) {                                                                              
#       $acceptable_volume{$name} = 0;                                                                                                
#    }                                                                                                                                
}

## Finally, generate lists containing which volumes are located at which site.                                                        
## This allows us to randomly select a volume from the list for the site that                                                         
## we plan on replicating to.                                                                                                         
my %volume_lists = ();
foreach my $vol_key (keys %requirement_map) {
    @{ $volume_lists{$vol_key} } = grep { $acceptable_volume{$_} == 1 } (
        grep { $volume_map{$_} eq $vol_key } (keys %volume_map)
    );
    print "$vol_key " . join(' ', @{ $volume_lists{$vol_key} }) . "\n";

    if ($#{ $volume_lists{$vol_key} } == -1) {
        die "No acceptable volume found for site $vol_key!\n";
    }
}

# Pull data from the gpc1 database
my $verbose = 0;
my $mdcParser = PS::IPP::Metadata::Config->new;

# Not technically imfiles, but the nebulous check block already uses files as a variable.
my $imfiles;
if (($stage eq 'camera')||($stage eq 'cam')) {
    $stage = 'camera';
    my $cmd = "$camtool -processedexp -cam_id $stage_id -dbname $dbname";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $cmd, verbose => 0);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stagetool: $error_code", $stage_id);
    }
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stagetool", $stage_id);
}
elsif ($stage eq 'stack') {
    my $cmd = "$stacktool -sumskyfile -stack_id $stage_id -dbname $dbname";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $cmd, verbose => 0);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stagetool: $error_code", $stage_id);
    }
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stagetool", $stage_id);
}
elsif ($stage eq 'skycal') {
    my $cmd = "$staticskytool -skycalresult -skycal_id $stage_id -dbname $dbname";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $cmd, verbose => 0);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stagetool: $error_code", $stage_id);
    }
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stagetool", $stage_id);
}
elsif ($stage eq 'warp') {
    my $cmd = "$warptool -warped -warp_id $stage_id -dbname $dbname";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $cmd, verbose => 0);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stagetool: $error_code", $stage_id);
    }
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stagetool", $stage_id);
}
elsif ($stage eq 'ff') {

    my $cmd = "$fftool -summary -ff_id $stage_id -dbname $dbname";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $cmd, verbose => 0);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stagetool: $error_code", $stage_id);
    }
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stagetool", $stage_id);
}
elsif ($stage eq 'diff') {
    my $cmd = "$difftool -diffskyfile -diff_id $stage_id -dbname $dbname";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $cmd, verbose => 0);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stagetool: $error_code", $stage_id);
    }
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stagetool", $stage_id);
}
elsif ($stage eq 'burntool') {
    my $cmd = "$regtool -processedimfile -exp_id $stage_id -dbname $dbname";
    my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
	run(command => $cmd, verbose => 0);
    unless ($success) {
	$error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
	&my_die("Unable to perform stagetool: $error_code", $stage_id);
    }
    $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
	&my_die("Unable to parse metadata from stagetool", $stage_id);
}

my $timer_start = time();
my $timer = time();
my %components = ('camera' => ['PSASTRO.OUTPUT','PSASTRO.OUTPUT.MASK','PSPHOT.BACKMDL','PSPHOT.PSF.RAW.SAVE'],
		  'warp'   => ['PSWARP.OUTPUT','PSWARP.OUTPUT.MASK','PSWARP.OUTPUT.VARIANCE'],
		  'stack'  => ['PPSTACK.UNCONV.COMP','PPSTACK.UNCONV.MASK.COMP','PPSTACK.UNCONV.VARIANCE.COMP',
			       'PPSTACK.UNCONV.EXP','PPSTACK.UNCONV.EXPNUM','PPSTACK.UNCONV.EXPWT.COMP','PPSTACK.TARGET.PSF'],
		  'skycal' => ['PSASTRO.OUTPUT.CMF','PSPHOT.OUTPUT.CFF','PSPHOT.STACK.PSF.SAVE','PSPHOT.STACK.BACKMDL'],
		  'ff'     => ['PSPHOT.OUT.CMF.MEF','PSPHOT.OUTPUT.CFF','PSPHOT.FULLFORCE.OUTPUT','PSPHOT.PSF.SKY.SAVE'],
		  'diff'   => ['PPSUB.OUTPUT.SOURCES','PPSUB.INVERSE.SOURCES','PSPHOT.PSF.SKY.SAVE','PSPHOT.BACKMDL.MEF','PPSUB.OUTPUT.KERNELS'],
		  'burntool' => ['BURN.TABLE.DOESNT.HAVE.A.PRODUCT']
    );

print "## permcheck.pl: $stage $stage_id $dbname $do_cull $do_ops $timer_start\n";
foreach my $entry (@$imfiles) {
    my $path_base = $entry->{path_base};
    my $data_state = $entry->{state};
    my $hostname = $entry->{hostname};
    my $quality  = $entry->{quality};
    
    # Fixes burntool issues.
    unless(defined($path_base)) { 
	$path_base = 'NO_PB';
    }
    unless(defined($data_state)) {
	$data_state = $entry->{data_state};
    }

    $timer = time() - $timer_start;
    print "# $path_base $data_state $hostname $quality T: $timer\n";
    if ($quality != 0) { next; }

    my @keys = ();
    foreach my $product (@{ $components{$stage} }) {
	print "# $product\n";
	my $is_done = 0;
	if ($stage eq 'camera') {
	    if ($product eq 'PSASTRO.OUTPUT.MASK') {
		my @otas = ('XY01','XY02','XY03','XY04','XY05','XY06',
			    'XY10','XY11','XY12','XY13','XY14','XY15','XY16','XY17',
			    'XY20','XY21','XY22','XY23','XY24','XY25','XY26','XY27',
			    'XY30','XY31','XY32','XY33','XY34','XY35','XY36','XY37',
			    'XY40','XY41','XY42','XY43','XY44','XY45','XY46','XY47',
			    'XY50','XY51','XY52','XY53','XY54','XY55','XY56','XY57',
			    'XY60','XY61','XY62','XY63','XY64','XY65','XY66','XY67',
			    'XY71','XY72','XY73','XY74','XY75','XY76');
		foreach my $ota (@otas) {
		    push @keys, $ipprc->filename($product,$path_base,$ota);
		}	    
		$is_done = 1;
	    }
	    elsif (($product eq 'PSPHOT.BACKMDL')||($product eq 'PSPHOT.PSF.RAW.SAVE')) {
		my $chip_id = $entry->{chip_id};
		my $chip_cmd = "$chiptool -processedimfile -chip_id $chip_id -dbname $dbname";
		my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
		    run(command => $chip_cmd, verbose => 0);
		unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		    &my_die("Unable to perform stagetool: $error_code", $stage_id);
		}
		my $chip_files = $mdcParser->parse_list(join "", @$stdout_buf) or
		    &my_die("Unable to parse metadata from stagetool", $stage_id);
		
		foreach my $chip_entry (@$chip_files) {
		    my $chip_path_base = $chip_entry->{path_base};
		    my $class_id       = $chip_entry->{class_id};
		    push @keys, $ipprc->filename($product,$chip_path_base,$class_id);
		}
		$is_done = 1;
	    }	    
	}
	elsif ($stage eq 'ff') {
	    if ($product ne 'PSPHOT.FULLFORCE.OUTPUT') {
		my $ffsum_cmd = "$fftool -result -ff_id $stage_id -dbname $dbname";
		my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
		    run(command => $ffsum_cmd, verbose => 0);
		unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		    &my_die("Unable to perform stagetool: $error_code", $stage_id);
		}
		my $ff_files = $mdcParser->parse_list(join "", @$stdout_buf) or
		    &my_die("Unable to parse metadata from stagetool", $stage_id);
		
		foreach my $ff_entry (@$ff_files) {
		    my $ff_path_base = $ff_entry->{path_base};
		    my $ff_skycell   = $ff_entry->{skycell_id};
		    my $warp_id      = $ff_entry->{warp_id};
		    if ($product ne 'PSPHOT.PSF.SKY.SAVE') {
			push @keys, $ipprc->filename($product,$ff_path_base);
		    }
		    else {
			my $warptool_cmd = "$warptool -warped -skycell_id $ff_skycell -warp_id $warp_id -dbname $dbname";
			($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
			    run(command => $warptool_cmd, verbose => 0);
			unless ($success) {
			    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
			    &my_die("Unable to perform stagetool: $error_code", $stage_id);
			}
			my $warp_files = $mdcParser->parse_list(join "", @$stdout_buf) or
			    &my_die("Unable to parse metadata from stagetool", $stage_id);
			foreach my $wt_entry (@$warp_files) {
			    my $wt_quality   = $wt_entry->{quality};
			    if ($quality == 0) {
				my $wt_path_base = $wt_entry->{path_base};
				push @keys, $ipprc->filename($product,$wt_path_base,$ff_skycell);
			    }
			}
		    }		    
		}
		$is_done = 1;
	    }
	}	    
	elsif ($stage eq 'burntool') {
	    if ($product eq 'BURN.TABLE.DOESNT.HAVE.A.PRODUCT') {
		my $bt_state = $entry->{burntool_state};
		if ($bt_state > -14) { next; }
		my $bt_file = $entry->{uri};

                if($bt_state <= -15) {
                  $bt_file =~ s/fits$/burn.v15.tbl/;	    
                } else {
                  $bt_file =~ s/fits$/burn.tbl/;  	    
                }
		push @keys, $bt_file;

		$is_done = 1;
	    }

	}
	elsif ($stage eq 'skycal') {
	    if (($product eq 'PSPHOT.STACK.PSF.SAVE')||($product eq 'PSPHOT.STACK.BACKMDL')) {
		my $sky_id = $entry->{sky_id};
		my $stack_id = $entry->{stack_id};
		
		my $ss_cmd = "$staticskytool -result -sky_id $sky_id -dbname $dbname";
		my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
		    run(command => $ss_cmd, verbose => 0);
		unless ($success) {
		    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
		    &my_die("Unable to perform stagetool: $error_code", $stage_id);
		}
		my $stack_files = $mdcParser->parse_list(join "", @$stdout_buf) or
		    &my_die("Unable to parse metadata from stagetool", $stage_id);
		foreach my $ss_entry (@$stack_files) {
		    my $ss_path_base = $ss_entry->{path_base};
		    push @keys, $ipprc->filename($product,$ss_path_base,$stack_id);
		}
		$is_done = 1;
	    }

	}		

	if ($is_done == 0) {
	    push @keys, $ipprc->filename($product,$path_base);
#	    print "$product\n";
#	    print ">> " . $ipprc->filename($product,$path_base) . "\n";
	}
# Do validation
    }

    $timer = time() - $timer_start;
    
    printf("# Identified %d : %s\n",$#keys + 1, $timer);
    foreach my $key (@keys) {
	# Testing code.
#	print "$key\n";
#	next;
#       }

 	# neb-stat level handling
 	my $stat = $neb->stat($key);
	unless ($stat) {
	    if (($stage eq 'camera')&&($key =~ /psf/)) {
		warn "nebulous key: $key not found";
	    }
	    elsif (($stage eq 'ff')&&($key =~ /cff/)) {
		warn "nebulous key: $key not found";
	    }
	    else {
		die  "nebulous key: $key not found";
	    }
	}
 	my $instances;
 	my $md5sum = '';

# 	# This needs to be in an eval, because although we expect things to exist, they
# 	# may not.  This is a fatal error in rawcheck, but need not be here.
 	eval {
 	    $instances = $neb->find_instances($key, 'any');
 	};
 	unless (defined($instances)) { print "## skipping due to zero instances\n"; next; }
 	die "no instances found" unless $instances;   

	my $existing_copies = 0;
 	my $Ngood = 0;
	my $Nbad = 0;
 	my %good_instances = ();
 	my %bad_instances  = ();

	#unsafe
	my $continue_checking_md5sums = 1;
	my %good_instances_unchecked  = ();
	#end unsafe

 	my @files = map {URI->new($_)->file if $_} @$instances;
 	my @validation = ();

 	for (my $i = 0; $i <= $#files; $i++) {
 	    my ($instance_exists,$instance_md5sum,$instance_host,$instance_volume,$instance_site, $is_good);
 	    ($instance_host,$instance_volume) = parse_volume($files[$i]);
 	    $instance_site = $volume_map{$instance_volume};
 	    $is_good = 0;

 	    if (-e $files[$i]) {
 		$instance_exists = 1;
 		$existing_copies++;
		if ($continue_checking_md5sums == 1) { #unsafe
		    $instance_md5sum = local_md5sum($files[$i]);

		    # This is bad, but I don't know what the right solution is.  We don't have 
		    # the md5sum a priori.  I think this is also the only major change needed 
		    # from rawcheck.pl
		    if (($md5sum eq '')&&($instance_md5sum ne 'd41d8cd98f00b204e9800998ecf8427e')) {
			$md5sum = $instance_md5sum;
		    }
		    
		    if ($instance_md5sum eq $md5sum) {
			push @{ $good_instances{$instance_site} }, $i;
			$is_good = 1;
			$Ngood++;
		    }               
		    else {
			push @{ $bad_instances{$instance_site} }, $i;
			$Nbad++;
		    }
		} #unsafe
		else { #unsafe
		    $instance_md5sum = 'CHECKING_STOPPED';
		    push @{ $good_instances_unchecked{$instance_site} }, $i;
		    $is_good = 1;
		    $Ngood++;
		} #end unsafe
	    }
	    else {
		$instance_exists = 0;
		$instance_md5sum = 'NON-EXISTANT';
		push @{ $bad_instances{$instance_site} }, $i;
	    }
	    
	    $validation[$i] = sprintf("    % 3d %d %32s %s %s %s\n",
				      $instance_exists,$is_good,$instance_md5sum,
				      $files[$i],$instance_host,$instance_volume);

	    #unsafe
	    # Remove this section when we decide to go back to actually checking everything
	    # to make sure we don't have any secret data corruption
	    if ($Ngood > 0) {
		print "# Breaking from validation check, as a valid copy has been found.\n";
		$continue_checking_md5sums = 0;
	    }
	    #end unsafe
	}

 	$timer = time() - $timer_start;
        # object_id ext_id epoch available existing total timer
 	printf("%s %s %s %d %d %d %d\n",@$stat[0],@$stat[1],@$stat[4],@$stat[6],$existing_copies,@$stat[7],$timer);
 	# instance_exists is_good instance_md5sum file instance_host instance_volume
 	my $val_string = join('',@validation);
 	print "$val_string";

# 	# Decide what to do
	
 	if ($Ngood == 0) {
 	    # If we have no copies, we can't do anything.
 	    die "No valid instance of key: $key\n";
 	}
 	## We have more than zero bad copies.  We may cull some of these in the future, but we should try to
 	## leave everything in the best state possible.
 	printf(">> %d %d\n",$Ngood, $#files + 1);
	if ($Nbad > 0) {
	    my $good_copy;
	    my $good_copy_index;
	    foreach my $site_key (keys %good_instances) {
		if ($#{ $good_instances{$site_key} } != -1) {
		    $good_copy_index = $good_instances{$site_key}[0];
		    $good_copy = $files[$good_copy_index];
		    last;
		}
	    }
	    printf(">> GOOD: $good_copy\n");
	    foreach my $site_key (keys %bad_instances) {
		foreach my $bad_copy_index (@{ $bad_instances{$site_key} }) {
		    print "cp $good_copy $files[$bad_copy_index]\n";
		    if ($do_ops) {
			system("cp $good_copy $files[$bad_copy_index]");
		    }
		    my $tmpmd5 = local_md5sum($files[$bad_copy_index]);
		    if ($tmpmd5 ne $md5sum) { 
                    ## This isn't super critical, so we don't need to die here.
			warn "Post-repair md5sum does not match! $tmpmd5 != $md5sum: $files[$bad_copy_index]";
		    }
		    else {
                    ## success
			push @{ $good_instances{$site_key} }, $bad_copy_index;
		    }
		}
	    }
	    ## We've done work here, so we can't do a cull this iteration.
	    if ($do_cull == 1) { $do_cull = -1; }
	}

	#unsafe
	foreach my $site_key (keys %good_instances_unchecked) {
	    push @{ $good_instances{$site_key} }, @{ $good_instances_unchecked{$site_key} };
	}
	#end unsafe
	
	## We can now attempt to make replicated copies to the sites that require additional copies.
	foreach my $site_key (keys %requirement_map) {
	    my $have_instances = $#{ $good_instances{$site_key} } + 1;
	    print "## $site_key $have_instances $requirement_map{$site_key}\n";
	    if ($#{ $good_instances{$site_key} } + 1 < $requirement_map{$site_key}) {
		my $rep_vol = get_random_site_volume($site_key);
		print "neb-replicate --volume $rep_vol  $key\n";
		if ($do_ops) {
		    $neb->replicate($key,$rep_vol) or die "failed to replicate the single valid copy to the backup node";
		    if ($@) { die $@; }
		    
		    # Begin my best validation thought
		    system("sync") == 0 or die "Couldn't sync?";
		    my $uris = $neb->find_instances($key,$rep_vol);
		    @$uris = map {URI->new($_)->file if $_} @$uris;
		    my $tmpmd5 = local_md5sum(${ $uris }[0]);
		    
		    my $validation_str = sprintf("% 3d %d %32s %s %s %s",
						 -1,-1,$tmpmd5,
						 ${ $uris }[0],"repl",$rep_vol);
		    print
			join("\n" . " " x 4, $validation_str), "\n";
		    
		    if ($tmpmd5 ne $md5sum) { 
			die "Post-replication md5sum does not match! $tmpmd5 != $md5sum";
		    }
		    # End my best validation thought.
		}       
		## We've done work here, so we can't do a cull this iteration.
		if ($do_cull == 1) { $do_cull = -1; }
	    }
	}
	
	## Do culls if that's what we were going to do.
	if ($do_cull == -1) {
	    die "Cull option passed, but files were modified in the scan/repair/replicate phase.  Not running cull!\n";
	}
	elsif ($do_cull == 1) {
	    ## At this point, we should have no files in the bad_instances lists, because we've repaired them.
	    foreach my $site_key (keys %good_instances) {
		if ($#{ $good_instances{$site_key} } + 1 > $requirement_map{$site_key}) {
		    for ($i = $requirement_map{$site_key}; $i <= $#{ $good_instances{$site_key} }; $i++) {
			my $cull_index = ${ $good_instances{$site_key} }[$i];
			my ($instance_host,$instance_volume) = parse_volume($files[$cull_index]);
			print "neb-cull --volume $instance_volume $key\n";
			if ($do_ops) {
			    # The tilde here is to force hard volumes.  Don't touch it.
			    # Also: the 2 is a "minimum number of copies" restriction.  Let's not be crazy here.
			    $neb->cull($key,"~${instance_volume}",2) or die "failed to cull a superfluous instance";
			    if ($@) { die "$@"; }
			}
		    } # End loop over extra instances
		} # End check for sites with extra instances
	    } # End loop over sites.
	} # End cull
	
    } ## End loop over keys.
} ## Endd loop over stage components.
    

sub local_md5sum {
    my $filename = shift;
    my $volume   = (split /\//, $filename)[2];
    my $host     = $volume;
    $host =~ s/\.\d//;
#    print "$filename $host $volume\n";
    my $response;
    my @file_stat = stat($filename);
    if ($file_stat[7] > 1e7) {
	$response = `ssh $host remote_md5sum.pl $filename`;
	chomp($response);
    }
    else {
	$response = $file_stat[7];
    }
    my ($sum, undef) = split /\s+/, $response;
    unless(defined($sum)) {
	my_die("Failed to calculate md5sum locally. $filename $host $volume");
    }
    return($sum);
}

sub parse_volume {
    my $filename = shift(@_);
    my $full_volume   = (split /\//, $filename)[2];
    my ($hostname,undef) = split /\./, $full_volume; # /;
    return($hostname,$full_volume);
}

sub get_random_site_volume {
    my $site_key = shift(@_);
    my $NN = scalar @{ $volume_lists{$site_key} };
    my $backup_volume = ${ $volume_lists{$site_key} }[int(rand($NN))];
    return($backup_volume);
}

sub my_die {
    my $msg = shift(@_);
    print $msg . "\n";
    foreach my $a (@_) {
	print "ARG: $a\n";
    }
    die;
}




















__END__


