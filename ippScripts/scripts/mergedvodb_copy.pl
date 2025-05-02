#!/usr/bin/env perl

#This is essentially the same as minidvodb_copy.pl - the tables are essentially identical, and the task (to rsync a copy) is essentially identical.  A new script was created for rsyncing the merges in case they should diverge in the future.

use warnings;
use strict;
use Carp;
 
## report the program and machine
use Sys::Hostname;
my $host = hostname();
print "\n\n";
print "Starting script $0 on $host\n\n";

use DateTime;
my $mjd_start = DateTime->now->mjd;   

my $dtime;
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $rsync = can_run('rsync') or (warn "Can't find rsync" and $missing_tools = 1);
my $mergedvodbtool = can_run('mergedvodbtool') or (warn "Can't find mergedvodbtool" and $missing_tools = 1);
my $ssh = can_run('ssh') or (warn "Can't find ssh" and $missing_tools = 1);
my $df = can_run('df') or (warn "Can't find df" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}



my ( $mergedvodbcopy_id, $merge_id, $mergedvodbrun_path, $mergedvodb_rsync_path, $destination_host, $dbname,$verbose, $logfile, $no_op, $redirect, $save_temps);
GetOptions(
    'mergedvodbcopy_id|w=s'     => \$mergedvodbcopy_id, #minidvodb database
    'merge_id|w=s'  => \$mergedvodb_id, #minidvodb_id
    'mergedvodbrun_path|w=s'  => \$mergedvodbrun_path, #minidvodb_id
    'mergedvodb_rsync_path|w=s' => \$mergedvodb_rsync_path,
    'destination_host|w=s' => \$destination_host, 
    'dbname|d=s'        => \$dbname, # Database name
    'verbose'           => \$verbose,   # Print to stdout
    'no-op'             => \$no_op, # Don't do any operations?
    'logfile=s'         => \$logfile,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --mergedvodbcopy_id --mergedvodb_rsync_path --mergedvodbrun_path --destination_host",
          -exitval => 3,
          ) unless
    defined $mergedvodbcopy_id and
    defined $mergedvodb_rsync_path and
    defined $mergedvodbrun_path and
    defined $destination_host;



my $ipprc = PS::IPP::Config->new();

my @df;


if ($logfile) {
    $ipprc->redirect_output($logfile) or my_die( "Unable to redirect output", $mergedvodbcopy_id, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}


{ #can we rsync?

    my $sizes = `du -sk $mergedvodbrun_path`;
    my $total = 0;
    for(split /[\r\n]+/,$sizes) # split on one or more newline characters
    { 
	my($number,$file) = split /\t/,$_,2; # split on tab ($file not used here)
	$total += $number; 
    }
#print 'Total: '.$total;
{    
    print "Checking available diskspace on $destination_host\n\n";

    my $command = "$ssh $destination_host df $mergedvodb_rsync_path";
    print "$command\n\n";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform '$ssh $destination_host df $mergedvodb_rsync_path' $error_code", $minidvodbcopy_id, $error_code);
    }

    my @fullbuf= split(/\n/, @{$stdout_buf}[0]);
    #   print $#{$stdout_buf};
#     print "xx $#df xx";
#    print $fullbuf[0];
    if ($#fullbuf ==1 ) {
#	print $fullbuf[1];
 	@df = split /\s+/, $fullbuf[1];
	if ($#df == 5) {
#	    
	    #print $df[3], $df[4];
	    
	    $df[4]=~ s/\%//;

	    if ($df[4] <  97 && $df[3]-$total) {
		print "\nThere is enough available space ($total < $df[3]) for the rsync\n";
	    } else {
		&my_die("Not enough available disk space: $df[4]% available, $df[3] on destination, $total free required for copy", $mergedvodbcopy_id, $PS_EXIT_PROG_ERROR);
	    }
#	    

	} else {
	    &my_die("Cannot parse df:\n@{$full_buf}[0]\nCan't figure out %free or available", $mergedvodbcopy_id, $PS_EXIT_PROG_ERROR); 
	} 
    } else {
	&my_die("Cannot parse df:\n@{$full_buf}[0]\nToo many lines ($#fullbuf) vs 1", $mergedvodbcopy_id, $PS_EXIT_PROG_ERROR);
    }
}
}

{#do the rsync
    my $command = "$rsync -rvuaq";
    $command .= " $mergedvodbrun_path";
    $command .= " $destination_host";
    $command .= ":$mergedvodb_rsync_path";
    print "\nPerforming rsync:\n";
    print "$command\n\n";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        &my_die("Unable to perform rsync: $error_code", $mergedvodbcopy_id, $error_code);
    }
    
    print "Rsync complete..\n";

}

{#update the database
    $dtime = 86400.0*(DateTime->now->mjd - $mjd_start);

    my $command = "$mergedvodbtool -updatemergedvodbcopy -mergedvodbcopy_id $mergedvodbcopy_id";
    $command .= " -set_state full -set_dtime $dtime";
    $command .= " -dbname $dbname" if defined $dbname;
    print "$command\n";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = 20;
        &my_die("Unable to update database - rsync completed without error' $error_code", $mergedvodbcopy_id, $error_code);
    }

}
exit 0;



sub my_die
{
    my $msg = shift; # Warning message on die
    my $mergedbvodbcopy_id = shift;
    my $exit_code = shift; # Exit code to add

    print STDERR "$msg $mergedvodbcopy_id\n";

if (defined $mergedvodb_id ) {
    my $command = "mergedvodbtool  -updatemergedvodbcopy -mergedvodbcopy_id $mergedvodbcopy_id";
    $command .= " -set_fault $exit_code";
    $command .= " -set_dtime $dtime" if defined $dtime;
    $command .= " -dbname $dbname" if defined $dbname;
    print $command;
    system ($command);
    }
    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;
    exit $exit_code;
}

__END__
