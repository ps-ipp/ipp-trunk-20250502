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


my ($server,$dbname,$exp_id,$do_cull,$save_log);

$server = $ENV{'NEB_SERVER'} unless $server;
# $dbname = 'gpc1'; 
GetOptions(
    'server|s=s'     => \$server,
    'dbname=s'       => \$dbname,
    'exp_id|x=s'     => \$exp_id,
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
pod2usage( -msg => "missing key", exitval => 2 )
    unless defined $exp_id;

if ($save_log) {
    my $time = time();
    my $logDest = "neb://any/raw_check/exp_${exp_id}";
    my $ipprc = PS::IPP::Config->new( "GPC1" ) or die "Could not create config object.\n";
    $ipprc->redirect_to_logfile($logDest) or die "Could not redirect output to logfile ${logDest}\n";
}

# Global options:
## Define the configuration.  Ideally, this would be retrieved from the nebulous
## database, but there are some issues with that (such as grouping the b nodes 
## into a less restrictive "offsite" location).

my $do_ops = 1;
my $i;

## Set up nebulous db interface, and pull processing information to consider.
my $neb = Nebulous::Client->new(
    proxy => "$server",
);
die "can't connected to Nebulous Server: $server"
    unless defined $neb;

## This new implementation is somewhat messy, but is more general and adaptable.
## First, we set up a requirement mapping, explaining where we want copies, and
## how many copies we want at each site.
my %requirement_map = ();
# CZW: 2017-01-10: I've changed this to MRTCB=1, OFFSITE=0, from MRTCB=0, OFFSITE=1 to attempt to speed
#                  up the shuffle (which is OFFSITE limited).  Setting MRTCB=1 to ensure two copies in 
#                  in the system (not totally necessary for everything, but a precaution in case this
#                  is run in cull mode without an edit back).
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
#	$acceptable_volume{$name} = 0;
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

# die;
# Pull data from the gpc1 database about the exposure to consider

my $verbose = 0;
my $mdcParser = PS::IPP::Metadata::Config->new;

print("$regtool -processedimfile -exp_id $exp_id -dbname $dbname\n");
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
	
	my $stat = $neb->stat($key);
	die "nebulous key: $key not found" unless $stat;   
	my $instances = $neb->find_instances($key, 'any');
	die "no instances found" unless $instances;   
	my @files = map {URI->new($_)->file if $_} @$instances;
	
	print ("\n#PRE: $key $data_state $md5sum $hostname $class_id T: $timer\n");
	
	my $has_itc_copy = 0;
	my @validation = ();
	for (my $i = 0; $i <= $#files; $i++) {
	    # Check for itc copy.
	    my ($instance_exists,$instance_md5sum,$instance_host,$instance_volume,$instance_site, $is_good);
	    ($instance_host,$instance_volume) = parse_volume($files[$i]);
	    $instance_site = $volume_map{$instance_volume};
	    
	    if ($instance_site eq 'ITC') {
		$has_itc_copy = 1;
	    }
	    
	    
	    $validation[$i] = sprintf("    % 3d %d %32s %s %s %s %s\n",
				      -1,-1,"UNCHECKED",
				      $files[$i],$instance_host,$instance_volume,$instance_site);
	    
	}
	my $val_string = join('',@validation);
	print "$val_string";
	
	if ($append == 0) {
	    if ($has_itc_copy == 0) { 
		if (defined($prior_imfile)) { 
		    push @to_do_imfiles, $prior_imfile;
		}
		$append = 1; 
	    }
	}
	
	if ($append == 1) {
	    push @to_do_imfiles, $imfile;
	}
	printf( "#PRE_RESULT: %2d %2d %2d %2d\n",$has_itc_copy,$append,$#to_do_imfiles,$#{ $imfiles });
	$prior_imfile = $imfile;
    }
    
    @$imfiles = @to_do_imfiles;	    
}
# End prescan checks.

# Loop over the imfiles of this exposure.
foreach my $imfile (@$imfiles) {
    my $key        = $imfile->{uri};
    my $data_state = $imfile->{data_state};
    my $md5sum     = $imfile->{md5sum};
    my $hostname   = $imfile->{hostname};
    my $class_id   = $imfile->{class_id};

    if (!(defined($hostname))) { $hostname = 'ipp004'; }
    $timer = time() - $timer_start;
    print ("\n# $key $data_state $md5sum $hostname $class_id T: $timer\n");

    ## Get instances, and do validation that they have the correct md5sums.
    my $stat = $neb->stat($key);
    die "nebulous key: $key not found" unless $stat;   
    my $instances = $neb->find_instances($key, 'any');
    die "no instances found" unless $instances;   
    
    my $existing_copies = 0;
    my $Ngood = 0;
    my $Nbad  = 0;
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
	    if ($instance_site eq 'ITC') {  # This is here to force checking of ITC copy.
		$continue_checking_md5sums = 1;
	    }
	    if ($continue_checking_md5sums == 1) { #unsafe
		$instance_md5sum = local_md5sum($files[$i]);
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


    # Decide what to do

    ## This block attempts to find an out-of-nebulous instance with a good md5sum, 
    ## and substitutes it for the 0-th listed instance.  This gives us one good
    ## copy to work with.
    if ($Ngood == 0) {
	# DO something to attempt to fix this.
	my $deneb_key = $key; $deneb_key =~ s/.*?gpc/gpc/;

	open(DD,"/home/panstarrs/ipp/local/bin/deneb-locate.py $deneb_key 2> /dev/null |");
	my $good_file = '';
	while (<DD>) {
	    $_ =~ s/^\s+//;
	    my ($z,undef,$ff) = split /\s+/;
	    if (($ff)&&(-e $ff)) {
		my $md_response = `md5sum $ff`;
		if ($md_response =~ /$md5sum/) {
		    $good_file = (split /\s+/,$md_response)[1];
		}
	    }
	}
	close(DD);
	if ($good_file eq '') {
	    die "No valid instance of key: $key";
	}
	else {
	    print "cp $good_file $files[0]\n";
	    if ($do_ops) {
		system("cp $good_file $files[0]");
	    }
	}
        # Begin my best validation thought
	{
	    my $tmpmd5 = local_md5sum($files[0]);
	    if ($tmpmd5 ne $md5sum) { 
		die "Post-replication md5sum does not match! $tmpmd5 != $md5sum";
	    }
	}
        # End my best validation thought.

	$Ngood = 1;  # We now hand off this single valid instance object to be handled by the Ngood=1 case.
        ## We've done work here, so we can't do a cull this iteration.
	if ($do_cull == 1) { $do_cull = -1; }
    }

    ## We have more than zero bad copies.  We may cull some of these in the future, but we should try to
    ## leave everything in the best state possible.
    printf(">> %d %d\n",$Ngood, $#files + 1);
#    if ($Ngood != $#files + 1) {
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

} ## End loop over imfiles.

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


