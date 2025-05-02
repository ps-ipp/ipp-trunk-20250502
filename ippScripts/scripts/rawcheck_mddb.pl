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

my ($neb_server,$md_server,$dbname,$dbuser,$dbpass,$exp_id,$do_cull,$do_revalidate,$save_log);


my $do_ops = 1;
my $i;

my $ipprc = PS::IPP::Config->new( "GPC1" ) or die "Could not create config object.\n";
my $siteConfig = $ipprc->{_siteConfig};
$dbuser = metadataLookupStr($ipprc->{_siteConfig}, "DBUSER");
$dbpass = metadataLookupStr($ipprc->{_siteConfig}, "DBPASSWORD");

$neb_server = $ENV{'NEB_SERVER'} unless $neb_server;

# $dbname = 'gpc1'; 
GetOptions(
    'neb_server|s=s' => \$neb_server, # This is the apache server
    'md_server|s=s'  => \$md_server,  # This is the actual database server
    'dbname=s'       => \$dbname,
    'dbuser=s'       => \$dbuser,
    'dbpass=s'       => \$dbpass,
    'exp_id|x=s'     => \$exp_id,
    'save_log'       => \$save_log,
    'cull'           => \$do_cull,
    'revalidate'     => \$do_revalidate,
) || pod2usage( 2 );

unless(defined($do_cull)) {
    $do_cull = 0;
}
unless(defined($do_revalidate)) {
    $do_revalidate = 0;
}
if (($do_cull == 1)&&($do_revalidate == 1)) {
    pod2usage( -msg => "Only one of --cull and --revalidate may be chosen.", -exitval => 2 );
}    


# Option parsing
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --neb_server (apache server)", -exitval => 2 )
    unless $neb_server;
pod2usage( -msg => "Required options: --md_server (database server)", -exitval => 2 )
    unless $md_server;
pod2usage( -msg => "Required options: --dbname", -exitval => 2 )
    unless defined $dbname;
pod2usage( -msg => "Required options: --dbuser", -exitval => 2 )
    unless defined $dbuser;
pod2usage( -msg => "Required options: --dbpass", -exitval => 2 )
    unless defined $dbpass;
pod2usage( -msg => "missing key", exitval => 2 )
    unless defined $exp_id;

if ($save_log) {
    my $logDest = "neb://any/raw_check_mddb/exp_${exp_id}";
    if ($do_cull) {
	$logDest .= ".cull";
    }
    elsif ($do_revalidate) {
	$logDest .= ".revalidate";
    }
    else {
	$logDest .= ".check";
    }

    $ipprc->redirect_to_logfile($logDest) or die "Could not redirect output to logfile ${logDest}\n";
}

# Global options:
## Define the configuration.  Ideally, this would be retrieved from the nebulous
## database, but there are some issues with that (such as grouping the b nodes 
## into a less restrictive "offsite" location).

## Set up nebulous db interface, and pull processing information to consider.
my $neb = Nebulous::Client->new(
    proxy => "$neb_server",
);
die "can't connected to Nebulous Server: $neb_server"
    unless defined $neb;

## Set up md5 db interface
my $mddb = DBI->connect("DBI:mysql:database=otaMD5;host=${md_server};" .
			"mysql_socket=/var/run/mysqld/mysqld.sock",
			${dbuser},${dbpass},
			{ RaiseError => 1, AutoCommit => 1}
    ) or die "Unable to connect to database $DBI::errstr\n";

## This new implementation is somewhat messy, but is more general and adaptable.
## First, we set up a requirement mapping, explaining where we want copies, and
## how many copies we want at each site.
my %requirement_map = ();
$requirement_map{ITC}     = 1;
$requirement_map{OFFSITE} = 1;

## Second, construct a list of volumes, mapped to their site location, using the
## same site locations as in the requirement map.
my %volume_map = ();
for ($i = 32; $i <= 32; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'ITC';
}
for ($i = 54; $i <= 97; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'ITC';
}
for ($i = 100; $i <= 126; $i++) {
    my $vol = sprintf("ipp%03d.0",$i);
    $volume_map{$vol} = 'ITC';
    $vol = sprintf("ipp%03d.1",$i);
    $volume_map{$vol} = 'ITC';
}

for ($i = 0; $i <= 15; $i++) {
    my $loc = 'OFFSITE';
#    if ($i == 6) { 
#	$loc = 'ITC';
#    } # This isn't "offsite", it's with the rest of the maui cluster.
#    if ($i == 9) { next; } # Not online

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
my %volume_fill_factors = ();
my %down_volume = ();
foreach my $vol_row (@$mounts) {
    my ($mount_point, $total, $used, $vol_id, $name, $host, $path, $allocate, $available, $xattr) = @{ $vol_row };
    if (($allocate == 1)&&($available == 1)&&( $used / $total < 0.98)) {
	$acceptable_volume{$name} = 1;
    }
    else {
#	print "## $name $allocate $available $used $total\n";
	$acceptable_volume{$name} = 0;
    }

    if (($available != 1)&&($xattr != 5)) {
	$down_volume{$name} = 1;
    }
    else {
	$down_volume{$name} = 0;
    }

    if (($name =~ /ippb05/)||($name =~ /ippb02/)) {
	$acceptable_volume{$name} = 0;
    }
    
    $volume_fill_factors{$name} = $used / $total;

    # This should prioritize hosts that we wish to deprecate.
    if ($total < 52885172852) {
	$volume_fill_factors{$name} = 1.0;
    }
}

## Finally, generate lists containing which volumes are located at which site.  
## This allows us to randomly select a volume from the list for the site that 
## we plan on replicating to.
my %volume_lists = ();
foreach my $vol_key (keys %requirement_map) {
    @{ $volume_lists{$vol_key} } = grep { $acceptable_volume{$_} == 1 } (
	grep { $volume_map{$_} eq $vol_key } (keys %volume_map)
    );
    print "#$vol_key " . join(' ', @{ $volume_lists{$vol_key} }) . "\n";

    if ($#{ $volume_lists{$vol_key} } == -1) {
	die "No acceptable volume found for site $vol_key!\n";
    }
}

# die;
# Pull data from the gpc1 database about the exposure to consider

my $verbose = 0;
my $mdcParser = PS::IPP::Metadata::Config->new;

print("#$regtool -processedimfile -exp_id $exp_id -dbname $dbname\n");
my $regtool_cmd = "$regtool -processedimfile -exp_id $exp_id -dbname $dbname";
my ($success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) = 
    run(command => $regtool_cmd, verbose => 0);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform regtool -processedimfile: $error_code", $exp_id);
}
my $imfiles = $mdcParser->parse_list(join "", @$stdout_buf) or
    &my_die("Unable to parse metadata from regtool -processedimfile", $exp_id);

my $timer_start = time();
my $timer = time();
print "## rawcheck.pl: $exp_id $dbname $do_cull $do_ops $timer_start\n";
# Do prescan checks.
if ($#{ $imfiles } > 0) {
    my @to_do_imfiles = ();
    my $prior_imfile;
    my $append = 0;

    foreach my $imfile (@$imfiles) {
	my $key        = $imfile->{uri};
	my $data_state = $imfile->{data_state};
	my $md5sum     = $imfile->{md5sum};
	my $hostname   = $imfile->{hostname};
	my $class_id   = $imfile->{class_id};

	unless(defined($key))        { die "No database uri: $exp_id"; }
	unless(defined($data_state)) { $data_state = 'no_data_state'; }
	if ($data_state eq 'corrupt') { next; } # This has already been marked as bad, so we can move on.
	unless(defined($md5sum))     { die "No database md5sum: $exp_id"; }
	unless(defined($hostname))   { $hostname = 'ipp004'; }
	unless(defined($class_id))   { die "No database class_id: $class_id"; }

	# This hash contains all the information about the instances, indexed by the full filename of the instance.
	my %unified_list = ();
	my %orphans      = ();

	$timer = time();
	print ("\n# $key $data_state $md5sum $hostname $class_id T: $timer\n");
	
	# Get nebulous results for this key, and parse them
	my $stat = $neb->stat($key);
	die "nebulous key: $key not found" unless $stat;   
	my $instances = $neb->find_instances($key, 'any');
	die "no instances found" unless $instances;   
	my @files = map {URI->new($_)->file if $_} @$instances;

	for (my $i = 0; $i <= $#files; $i++) {
	    my $f = $files[$i];
	    $f =~ s%//%/%g;
	    my ($instance_exists,$instance_host,$instance_volume,$instance_site);
	    ($instance_host,$instance_volume) = parse_volume($files[$i]);   
	    $instance_site = $volume_map{$instance_volume};
	    if (-e $f) {
		$instance_exists = 1;
	    }
	    else {
		$instance_exists = 0;
	    }
	    $unified_list{$f}{neb_host} = $instance_host;
	    $unified_list{$f}{neb_vol}  = $instance_volume;
	    $unified_list{$f}{neb_site} = $instance_site;
	    $unified_list{$f}{exists}   = $instance_exists;
	}

	# Get md database results for this exp_id/class_id
	my $md_query = "select file_id,ins_id,mvol_name,site_name,file,disk_sum,(STRCMP(md5sum,disk_sum) = 0) AS is_valid FROM obj LEFT JOIN ins USING(file_id) LEFT JOIN vol USING(mvol_id) WHERE exp_id = $exp_id AND class_id = '${class_id}'";
	my $md_instances = $mddb->selectall_arrayref( $md_query );
	
	$timer = time();
#	print "##NEB $timer\n";
	my $md_file_id = -1;
	foreach my $r (@{ $md_instances }) {
	    my ($file_id,$ins_id,$vol_name,$site_name,$f,$disk_sum,$is_valid) = @{ $r };

	    if ((defined($f))&&(defined($ins_id))) { 
		$f =~ s%//%/%g;
		unless (exists($unified_list{$f})) {
		    if (-e $f) {
			$unified_list{$f}{exists} = 1;
		    }
		    else {
			$unified_list{$f}{exists} = 0;
		    }
		}
		# This should only happen if the same scan was ingested twice.
		# Be paranoid and refuse to cull if this is the case.
		if (exists($unified_list{$f}{md_ins_id})) {
		    if (($file_id   == $unified_list{$f}{md_file_id})&&
			($vol_name  eq $unified_list{$f}{md_vol})&&
			($site_name eq $unified_list{$f}{md_site})&&
			($disk_sum  eq $unified_list{$f}{disk_sum})&&
			($is_valid  == $unified_list{$f}{is_valid})) {
			print "#DUPLICATE INS: $ins_id have $unified_list{$f}{md_ins_id} already\n";
			mddb_delete_instance($mddb,$ins_id);
			$do_cull = -1;
			next;
		    }
		    else {
			# The data is inconsistent.  Make a human fix it.
			print "INS ERROR: $f -> \n";
			print "\t($file_id $ins_id $vol_name $site_name $disk_sum $is_valid)\n";
			print "\t($unified_list{$f}{md_file_id} $unified_list{$f}{md_ins_id} $unified_list{$f}{md_vol} $unified_list{$f}{md_site} $unified_list{$f}{disk_sum} $unified_list{$f}{is_valid})\n";
			die;
		    }
		}
		
		$unified_list{$f}{md_file_id} = $file_id;
		$unified_list{$f}{md_ins_id}  = $ins_id;
		$unified_list{$f}{md_vol}     = $vol_name;
		$unified_list{$f}{md_site}    = $site_name;
		$unified_list{$f}{disk_sum}   = $disk_sum;
		$unified_list{$f}{is_valid}   = $is_valid;	    
	    }

	    if (defined($file_id)) {
		if ($md_file_id == -1) {
		    $md_file_id = $file_id;
		}
		elsif ($file_id != $md_file_id) {
		    die "Instance $ins_id has bad file_id $file_id expected $md_file_id";
		}
	    }
	}
	if ($md_file_id == -1) {
	    # There is no md information for this exposure
	    $md_file_id = mddb_create_file($mddb,$key,$exp_id,$class_id,$md5sum);
	    $do_cull = -1;
	}

	$timer = time();
#	print "##MD $timer\n";
	
	# Check that evrything is consistent.
	my $good_copies = 0;
	my $all_copies = scalar(keys(%unified_list));
	my $a_good_copy = '';

	my %instance_count = ();
	foreach my $site (keys(%requirement_map)) {
	    $instance_count{$site} = 0;
	}

	foreach my $f (keys(%unified_list)) {
	    $unified_list{$f}{good_instance} = 0;
	    print "$f  ";
	    
	    # Identify orphans, and push their handling until later.
	    unless (exists($unified_list{$f}{neb_host})) {
		if ($down_volume{$unified_list{$f}{md_vol}} != 1) {
		    # Handle this.
		    $orphans{$f} = $unified_list{$f};
		    warn "No neb entry for $orphans{$f}{md_ins_id} $exp_id $class_id $f \n";
		}
		else {
		    warn "No neb entry for likely due to host issue $exp_id $class_id $f \n";
		}
		delete($unified_list{$f});
		next;
	    }
	    print "$unified_list{$f}{neb_host} $unified_list{$f}{neb_vol} $unified_list{$f}{neb_site} $unified_list{$f}{exists}  ";


	    # Create an entry in the mddb database if that doesn't already exist.
	    unless (exists($unified_list{$f}{md_vol})) {
		# Insert a new row in the mddb ins table for this instance.  
		if ($unified_list{$f}{exists} != 1) {
		    # Ensure that the file exists, so an md5sum can be performed.
		    system("touch $f");
		}
		my $tmpmd5 = local_md5sum($f);
		my ($ins_id, $site_name) = mddb_insert_instance($mddb,$md_file_id,$f, $unified_list{$f}{neb_vol}, $tmpmd5);

		$unified_list{$f}{md_vol}     = $unified_list{$f}{neb_vol};
		$unified_list{$f}{md_site}    = $site_name;
		$unified_list{$f}{disk_sum}   = $tmpmd5;
		$unified_list{$f}{md_file_id} = $md_file_id;
		$unified_list{$f}{md_ins_id}  = $ins_id;

		if ($tmpmd5 ne $md5sum) { 
		    # Insert even if we fail so we can try fixing next time.
		    # This may cause a problem if the first instance we checked has a bad md5sum.
		    warn "Post-replication md5sum does not match! $tmpmd5 != $md5sum";
		    $unified_list{$f}{is_valid}   = 0;
		}
		else {
		    $unified_list{$f}{is_valid}   = 1;
		}
	    }

	    # This will force md5sum calculations to everything, so disk<->mddb consistency can be checked.
	    if ($do_revalidate) {
		my $tmpmd5 = local_md5sum($f);
		print "REVALIDATE: ($tmpmd5) ";
		if ($tmpmd5 ne $unified_list{$f}{disk_sum}) {
		    mddb_update_instance($mddb,$unified_list{$f}{md_ins_id}, $tmpmd5);
		    warn "Updated instance $unified_list{$f}{md_ins_id} to $tmpmd5\n";
		}
		if ($tmpmd5 ne $md5sum) {
		    warn "Revalidated md5sum does not match!";
		}
	    }

	    print "$unified_list{$f}{md_file_id} $unified_list{$f}{md_ins_id} $unified_list{$f}{md_vol} $unified_list{$f}{md_site} $unified_list{$f}{disk_sum} $unified_list{$f}{is_valid}  ";


	    if ($unified_list{$f}{exists} != 1) {
		warn "Instance does not exist $exp_id $class_id $f\n";
		next;
	    }
	    if ($unified_list{$f}{neb_vol} ne $unified_list{$f}{md_vol}) {
		die "Inconsistent volumes for $exp_id $class_id $f $unified_list{$f}{neb_vol} $unified_list{$f}{md_vol}\n";
	    }
	    if ($unified_list{$f}{is_valid} != 1) {
		warn "Instance is not valid $exp_id $class_id $f\n";
		next;
	    }
	    if ($unified_list{$f}{disk_sum} ne $md5sum) {
		# This is a die, because if the gpc1 md5sum and the mddb md5sum don't match, there's a problem
		die "Inconsistent md5sum values $exp_id $class_id $f $md5sum $unified_list{$f}{disk_sum}\n";
	    }
	    
	    # If none of that stopped us, then this instance is probably fine.
	    $unified_list{$f}{good_instance} = 1;
	    $instance_count{$unified_list{$f}{neb_site}} ++;
	    $good_copies++;
	    if ($a_good_copy eq '') {
		$a_good_copy = $f;
	    }
	    print "$unified_list{$f}{good_instance}";
	    print "\n";
	}

	if ($good_copies == 0) {
	    my $orphan_copy = '';
	    foreach my $orph (keys (%orphans)) {
		if ($orphans{$orph}{is_valid} == 1) {
		    $orphan_copy = $orph;
		}
	    }

	    if ($orphan_copy eq '') {
		die "No good copies available for $exp_id $class_id\n";
	    }
	    else {
		die "No good copies available for $exp_id $class_id (but $orphan_copy may exist)\n";
	    }
	}

	$timer = time();
#	print "##CHECK $timer\n";

	# Attempt to repair any bad instance.
	print "COUNT CHECK $good_copies $all_copies\n";
	if ($good_copies != $all_copies) {
	    foreach my $f (keys(%unified_list)) {
		if ($unified_list{$f}{good_instance} != 1) {
		    system("cp $a_good_copy $f");
		    my $tmpmd5 = local_md5sum($f);
		    if ($tmpmd5 ne $md5sum) { 
			die "Post-replication md5sum does not match! $tmpmd5 != $md5sum";
		    }
		    
		    mddb_update_instance($mddb,$unified_list{$f}{md_ins_id}, $tmpmd5);

		    $do_cull = -1;
		    $unified_list{$f}{good_instance} = 2;
		    $unified_list{$f}{disk_sum} = $tmpmd5;
		    $instance_count{$unified_list{$f}{neb_site}} ++;
		    $good_copies++;

		    # Print out the same information as before, but updated to show the repair.
		    print "$f  ";
		    print "$unified_list{$f}{neb_host} $unified_list{$f}{neb_vol} $unified_list{$f}{neb_site} $unified_list{$f}{exists}  ";
		    print "$unified_list{$f}{md_file_id} $unified_list{$f}{md_ins_id} $unified_list{$f}{md_vol} $unified_list{$f}{md_site} $unified_list{$f}{disk_sum} $unified_list{$f}{is_valid}  ";
		    print "$unified_list{$f}{good_instance}";
		    print "\n";
		}
	    }
	}
	
	# We cannot reach this point if good_copies != all_copies.

	# Do replications
	foreach my $site (keys %requirement_map) {
	    print "#$site $instance_count{$site} $requirement_map{$site}\n";
	    if ($instance_count{$site} < $requirement_map{$site}) {
		my $rep_vol = get_random_site_volume($site);
		print "neb-replicate --volume $rep_vol  $key\n";
		$do_cull = -1;

		if ($do_ops) {
		    $neb->replicate($key,$rep_vol) or die "failed to replicate $key to $rep_vol";
		    if ($@) { die $@; }
		    
		    # Begin my best validation thought
		    system("sync") == 0 or die "Couldn't sync?";
		    my $uris = $neb->find_instances($key,$rep_vol);
		    @$uris = map {URI->new($_)->file if $_} @$uris;
		    my $f = ${ $uris }[0];
		    my $tmpmd5 = local_md5sum($f);
		    if ($tmpmd5 ne $md5sum) { 
			die "Post-replication md5sum does not match! $tmpmd5 != $md5sum";
		    }
		    # End my best validation thought.

		    # Report on the replicated instance.
		    my ($ins_id, $site_name) = mddb_insert_instance($mddb,$md_file_id,$f, $rep_vol, $tmpmd5);
		    $unified_list{$f}{md_vol}     = $rep_vol;
		    $unified_list{$f}{md_site}    = $site_name;
		    $unified_list{$f}{disk_sum}   = $tmpmd5;
		    $unified_list{$f}{is_valid}   = 1;
		    $unified_list{$f}{md_file_id} = $md_file_id;
		    $unified_list{$f}{md_ins_id}  = $ins_id;

		    my ($instance_host,$instance_volume) = parse_volume($f);
		    my $instance_site = $volume_map{$instance_volume};
		    $unified_list{$f}{neb_host}   = $instance_host;
		    $unified_list{$f}{neb_vol}    = $instance_volume;
		    $unified_list{$f}{neb_site}   = $instance_site;
		    $unified_list{$f}{exists}     = 1;
		    $unified_list{$f}{good_instance} = 1;

		    print "$f  ";
		    print "$unified_list{$f}{neb_host} $unified_list{$f}{neb_vol} $unified_list{$f}{neb_site} $unified_list{$f}{exists}  ";
		    print "$unified_list{$f}{md_file_id} $unified_list{$f}{md_ins_id} $unified_list{$f}{md_vol} $unified_list{$f}{md_site} $unified_list{$f}{disk_sum} $unified_list{$f}{is_valid}  ";
		    print "$unified_list{$f}{good_instance}";
		    print "\n";
 		}	    
	    }
	} # End replicating

	# Do culls
	if ($do_cull == 1) {
	    foreach my $site (keys %requirement_map) {
		if ($instance_count{$site} > $requirement_map{$site}) {
		    my @cull_order = ();
		    my %sorting_hash = ();
		    for my $f (keys %unified_list) {
			if ($unified_list{$f}{neb_site} eq $site) {
			    push @cull_order, $f;
			    $sorting_hash{$f} = $volume_fill_factors{$unified_list{$f}{neb_vol}};
			}
		    }
		    @cull_order = sort { $sorting_hash{$b} <=> $sorting_hash{$a} } @cull_order;
		    
#		    my $tmp = join ' ', @cull_order;
#		    print "$exp_id $site $tmp\n";
		    for ($i = 0; $i < ($instance_count{$site} - $requirement_map{$site}); $i++) {
			my $f = $cull_order[$i];
			my $instance_volume = $unified_list{$f}{neb_vol};
			print "neb-cull --volume $instance_volume $key\n";
			print "$f $unified_list{$f}{md_ins_id}\n";
 			if ($do_ops) {
 			    # The tilde here is to force hard volumes.  Don't touch it.
 			    # Also: the 2 is a "minimum number of copies" restriction.  Let's not be crazy here.
 			    $neb->cull($key,"~${instance_volume}",2) or die "failed to cull a superfluous instance";
 			    if ($@) { die "$@"; }
			    mddb_delete_instance($mddb,$unified_list{$f}{md_ins_id});
 			}
		    }
		}
	    }

	    # If we're here, then we've sucessfully sorted out this instance.
	    foreach my $f (keys %orphans) {
		print "ORPHAN $f $orphans{$f}{md_ins_id}\n";
		mddb_delete_instance($mddb,$orphans{$f}{md_ins_id});
#		unlink($f) or warn "Could not unlink $f: $!";
	    }
	    
	} # End culling

    } # End scan over imfiles
} # End check for gpc1 database entry
# End prescan checks.
if ($do_cull == -1) {
    die "Could not cull after repairing a file.\n";
}

# mddb_create_file($mddb,$key,$exp_id,$class_id,$md5sum);
sub mddb_create_file {
    my $db = shift;
    my $key = shift;
    my $exp_id = shift;
    my $class_id = shift;
    my $md5sum = shift;

    my $query = "INSERT INTO obj (neb_key, exp_id, class_id, md5sum) VALUES (?, ?, ?, ?)";
    $db->do("BEGIN");
    my $stmt  = $db->prepare($query);
    $stmt->execute($key,$exp_id,$class_id,$md5sum) or
	&my_die("failed to insert obj with new file $key $exp_id $class_id $md5sum");
    $stmt->finish();
    $db->do("COMMIT");

    $query = "SELECT file_id FROM obj WHERE exp_id = $exp_id AND class_id = '$class_id'";
    my $md_instances = $mddb->selectall_arrayref( $query );
    my $row = ${ $md_instances }[0];
    my $file_id = ${ $row }[0];
    return($file_id);
}

sub mddb_update_instance {
    my $db = shift;
    my $ins_id = shift;
    my $disk_sum = shift;

    my $query = "UPDATE ins SET disk_sum = ? WHERE ins_id = ?";
    $db->do("BEGIN");
    my $stmt  = $db->prepare($query);
    $stmt->execute($disk_sum,$ins_id) or
	&my_die("failed to update ins with new sum $ins_id $disk_sum");
    $stmt->finish();
    $db->do("COMMIT");
}

sub mddb_insert_instance {
    my $db = shift;
    my $file_id = shift;
    my $file = shift;
    my $volume = shift;
    my $disk_sum = shift;

    my $query = "INSERT INTO ins (file_id, file, mvol_id, disk_sum) SELECT ?, ?, mvol_id, ? FROM vol WHERE vol.mvol_name = ?";
    $db->do("BEGIN");
    my $stmt  = $db->prepare($query);
    $stmt->execute($file_id,$file,$disk_sum,$volume) or
	&my_die("failed to insert ins with new instance $file_id $file $volume $disk_sum");
    $stmt->finish();
    $db->do("COMMIT");
    
    $query = "SELECT ins_id, site_name FROM ins JOIN vol USING(mvol_id) WHERE file_id = $file_id AND file = '$file'";
    my $md_instances = $mddb->selectall_arrayref( $query );
    my $row = ${ $md_instances }[0];
    my $ins_id = ${ $row }[0];
    my $site_name = ${ $row }[1];
    return($ins_id, $site_name);
}

sub mddb_delete_instance {
    my $db = shift;
    my $ins_id = shift;

    print "DELETE $ins_id\n";
    my $query = "DELETE FROM ins WHERE ins_id = ?";
    $db->do("BEGIN");
    my $stmt  = $db->prepare($query);
    $stmt->execute($ins_id) or
	&my_die("failed to delete ins entry $ins_id");
    $stmt->finish();
    $db->do("COMMIT");
}

sub local_md5sum {
    my $filename = shift;
    my $volume   = (split /\//, $filename)[2];
    my $host     = $volume;
    $host =~ s/\.\d//;
#    print "$filename $host $volume\n";
    my $response = `ssh $host remote_md5sum.pl $filename`;
    chomp($response);
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


