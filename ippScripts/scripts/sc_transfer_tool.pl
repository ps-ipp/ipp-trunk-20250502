#!/usr/bin/env perl

use Carp;
use warnings;
use strict;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use File::Basename;
use IPC::Cmd 0.36 qw( can_run run );
use Sys::Hostname;

my $scp_cmd = "/usr/projects/cosmo/amd6100/bin/scp";
my $ssh_cmd = "/usr/projects/cosmo/amd6100/bin/ssh";

my $remote_root = '/lustre/scratch1/turquoise/watersc1/ps1/';
   $remote_root = '/scratch3/watersc1/';

#my $local_raw = "${remote_root}/tmp/";
my $local_tmp = "${remote_root}/tmp/";
my $threads = 11;
my @hosts = (
    'ippc20.ipp.ifa.hawaii.edu',
    'ippc21.ipp.ifa.hawaii.edu',
    'ippc22.ipp.ifa.hawaii.edu',
    'ippc23.ipp.ifa.hawaii.edu',
    'ippc24.ipp.ifa.hawaii.edu',
    'ippc25.ipp.ifa.hawaii.edu',
    'ippc26.ipp.ifa.hawaii.edu',
    'ippc27.ipp.ifa.hawaii.edu',
    'ippc28.ipp.ifa.hawaii.edu',	
#'ippc29.ipp.ifa.hawaii.edu', (mark's hi-mem test machine)
#'ippc30.ipp.ifa.hawaii.edu', (postage stamp server)	
    'ippc31.ipp.ifa.hawaii.edu',	
    'ippc32.ipp.ifa.hawaii.edu'    
    );
@hosts = (@hosts, @hosts, @hosts, @hosts, @hosts);
my %server_options = (
    'ippc20.ipp.ifa.hawaii.edu' => '',
    'ippc21.ipp.ifa.hawaii.edu' => '',
    'ippc22.ipp.ifa.hawaii.edu' => '',
    'ippc23.ipp.ifa.hawaii.edu' => '',
    'ippc24.ipp.ifa.hawaii.edu' => '',
    'ippc25.ipp.ifa.hawaii.edu' => '',
    'ippc26.ipp.ifa.hawaii.edu' => '', # -o NoneSwitch=yes -o NoneEnabled=yes',
    'ippc27.ipp.ifa.hawaii.edu' => '', # -o NoneSwitch=yes -o NoneEnabled=yes',
    'ippc28.ipp.ifa.hawaii.edu' => '', # -o NoneSwitch=yes -o NoneEnabled=yes',
    'ippc29.ipp.ifa.hawaii.edu' => '', # -o NoneSwitch=yes -o NoneEnabled=yes',
    'ippc30.ipp.ifa.hawaii.edu' => '', # -o NoneSwitch=yes -o NoneEnabled=yes',
    'ippc31.ipp.ifa.hawaii.edu' => '', # -o NoneSwitch=yes -o NoneEnabled=yes',
    'ippc32.ipp.ifa.hawaii.edu' => '', # -o NoneSwitch=yes -o NoneEnabled=yes'
    );

my $input_path;
my $local_file;
my $remote_file;

my $verbose = 0;
my $fetch = 0;
my $offset = 0;
my $retry = 0;
my $quickfetch = 0;

GetOptions(
    'threads=s'   => \$threads,
    'input=s'     => \$input_path,
    'offset=s'    => \$offset,
    'fetch'       => \$fetch,
    'verbose'     => \$verbose,
    'retry'       => \$retry,
    'quickfetch'  => \$quickfetch,
    ) or pod2usage( 2 );
pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2) if @ARGV;
pod2usage( -msg => "Required options: --input", -exitval => 3) unless
    defined($input_path);

my $hostname = hostname;
print STDERR "Running on $hostname\n";

# split in input file list
unless(-d $local_tmp) { system("mkdir -p $local_tmp"); }
#unless(-d $local_raw) { system("mkdir -p $local_raw"); }

# we have two modes: fetch (-> LANL) and return (-> IPP)

if ($fetch) {
    $local_file = "$input_path.check";
    $remote_file = "$input_path.transfer";
} else {
    $local_file = "$input_path.generate";
    $remote_file = "$input_path.return";
}
my $input_base = basename($local_file);

unless ($retry) {
  # open the input file list
  open (LOCAL,$local_file) || die "Couldn't find input file specified $local_file\n";
  open (REMOTE,$remote_file) || die "Couldn't find input file specified $remote_file\n";
  
  # generate N output files (to be fed to the N rsync / tar threads)
  my @filehandles;
  my $i;
  my $line = 0;
  for ($i = 0; $i < $threads; $i++) {
      open($filehandles[$i], ">${local_tmp}/${input_base}.${i}");
  }
  
  unless ($fetch) {
      my $stat_file = "$input_path.stat";
      print "STATFILE: $stat_file\n";
      open (STATFILE, ">$stat_file");
  }
  
  $i = 0;
  while (my $Lname = <LOCAL>) {
      my $Rname = <REMOTE>;
  
      chomp $Lname;
      chomp $Rname;
  
      $line++;
      if ($line < $offset) { next; } # I think this is off-by-one in a safe direction
  
      if ($line % 250 == 0)  { print STDERR "line $line : $Lname\n"; }
  
      $i = int(rand($#filehandles + 1));
      if ($i >= $threads) {
  	print STDERR "HUH: impossible file handle?\n";
  	die;
      }
  
      # if we are fetching, and 'quickfetch' is requested, skip file if it exists
      # NOTE: this does not check that the transfer was complete or the file is current
  
      if ($fetch) {
  	# We are fetching, and do not already have this file.
  	if ($quickfetch && (-e $Lname)) { next; }
  	print { $filehandles[$i] } "$Rname\n";
      } else { 
  	# We are pushing, but only send back files we actually have
  	# we cannot have the remote file unless the local file exists 
  	# (it is a link to the local file and is generated after the local file)
  	my $haveRemote = -e "$local_tmp/$Rname";
  	if ($haveRemote) {
  	    print { $filehandles[$i] } "$Rname\n";
  	    print STATFILE "$Rname PASS\n";
  	} else {
  	    my $haveLocal = -e $Lname;
  	    if ($haveLocal) {
  		print STATFILE "$Rname PART\n";
  	    } else {
  		print STATFILE "$Rname FAIL\n";
  	    }
  	}
      }
  }
  close(LOCAL);
  close(REMOTE);
  unless ($fetch) { close(STATFILE); }
  
  for ($i = 0; $i < $threads; $i++) {
      close($filehandles[$i]);
  }
}

my $i;
# fork the rsync
my @pids = ();
for ($i = 0; $i < $threads; $i++) {
    $pids[$i] = fork();
    if ($pids[$i] == 0) {
	my $host = $hosts[$i];
	my $code = 0;
	if ($fetch) { 
	    $code = fetch_task($host,"${local_tmp}/${input_base}.${i}", 0);
	} else {
	    $code = transfer_task($host,"${local_tmp}/${input_base}.${i}", 0);
	}
	print STDERR "$host $input_base $i $code\n";
	exit($code);
    }
}

my $global_status = 0;
for ($i = 0; $i < $threads; $i++) {
    waitpid($pids[$i],0);
    my $this_status = $?;
    print "exit status $hosts[$i] : $this_status\n";
    if ($this_status) { $global_status = ($this_status >> 8); }
}

for ($i = 0; $i < $threads; $i++) {
#   unlink("${local_tmp}/${input_base}.${i}");
}

# my $scp_command = "$scp_cmd $option ${local_tmp}/${input_base}.stat $hosts[0]:/tmp/";
# print "$scp_command\n";
# system("$scp_command");
# XXX check return status

print "global status: $global_status\n";
exit ($global_status);

# distribute bundles to nodes

# execute bundle transfer

sub transfer_task {
    my $destination_host = shift;
    my $transfer_filelist= shift;
    my $error = shift;

    my $option = $server_options{$destination_host};

    my $command = "rsync -L --size-only --omit-dir-times -e '$ssh_cmd ${option}' --files-from=${transfer_filelist} ${local_tmp} ${destination_host}:/";
    print STDERR "$command\n";
    
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);

    my $return_value = 0;
    unless ($success) {
	if ($error_code =~ /value 23/) {
	    print STDERR "Some files failed to transfer.  Partial run?\n";
	    $error = 23;
	    $return_value = 23;
	} else {
	    print STDERR "ERROR:  $error_code\n";
	    warn("Transfer of $transfer_filelist to $destination_host failed with error $error_code.");
	    sleep(5);
	    $error++;
	    if ($error < 4) {
		$error_code = transfer_task($destination_host,$transfer_filelist,$error);
	    }
	    else {
		die("Failed too many times $error $destination_host $transfer_filelist");
	    }
	    $return_value = 1;
	}
    }
    return($return_value);
}
    
sub fetch_task {
    my $destination_host = shift;
    my $transfer_filelist= shift;
    my $error = shift;

    my $option = $server_options{$destination_host};

    my $command = "rsync --size-only -e '$ssh_cmd ${option}' --files-from=${transfer_filelist} ${destination_host}:/ ${local_tmp}";
    print STDERR "$command\n";

    my $error_code = 0;
    my ( $success, $error_msg, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
	print "raw error_msg: $error_msg...\n";
	($error_code) = $error_msg =~ m/exited with value (\d+)/;
	warn("Transfer of files in $transfer_filelist from $destination_host failed with error ($error_code) $error_msg.");
	print "*** stdout: *** \n";
	foreach my $line (@$stdout_buf) {
	    print STDERR "stdout: $line\n";
	}
	print "*** stderr: *** \n";
	foreach my $line (@$stderr_buf) {
	    print STDERR "stderr: $line\n";
	}
	if ($error_code == 23) {
	    print STDERR "Some files failed to transfer\n";
	} else {
	    $error++;
	    if ($error < 4) {
		$error_code = fetch_task($destination_host,$transfer_filelist,$error);
	    } else {
		print STDERR "Failed too many times $error $destination_host $transfer_filelist\n";
	    }
	}
    }
    return($error_code);
}
