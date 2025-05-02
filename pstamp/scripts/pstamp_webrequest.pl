#!/bin/env perl
###
#
# pstampwebrequest.pl: take a postage stamp request command line and process it
#
# The arguments are the command line parameters for the program psmkreq
#
# Note: Despite the name there nothing particularly web specific about this program.
#
###

use warnings;
use strict;

use Getopt::Long qw( GetOptions );
use Sys::Hostname;
use POSIX qw( strftime );

my $host = hostname();
my $verbose = 0;
my $dbname;
my $dbserver;
my $pretend;
my $label = 'WEB';
my $req_name_prefix = 'web';
my $username;

GetOptions(
    'dbname=s'      =>  \$dbname,
    'dbserver=s'    =>  \$dbserver,
    'label=s'       =>  \$label,
    'username=s'    =>  \$username,
    'prefix=s'      =>  \$req_name_prefix,
    'pretend'       =>  \$pretend,
    'verbose'       =>  \$verbose,
);



if ($verbose) {
    print STDERR "\n\n";
    print STDERR "Starting script $0 on $host\n\n";
}


use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Config qw($PS_EXIT_SUCCESS
		       $PS_EXIT_UNKNOWN_ERROR
		       $PS_EXIT_SYS_ERROR
		       $PS_EXIT_CONFIG_ERROR
		       $PS_EXIT_PROG_ERROR
		       $PS_EXIT_DATA_ERROR
		       $PS_EXIT_TIMEOUT_ERROR
		       metadataLookupStr
		       metadataLookupBool
		       caturi
		       );

use Cwd;

my $missing_tools;

my $psmkreq = can_run('psmkreq')  or (warn "Can't find psmkreq"  and $missing_tools = 1);
my $pstamptool = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $pstampparse = can_run('pstampparse.pl')  or (warn "Can't find pstampparse.pl"  and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}

# make a request file in a sub directory of the current directory
my $cur_dir = getcwd();

#print STDERR "cur_dir is $cur_dir\n";

# put file in directory for the current date
my $datestr = strftime "%Y/%m/%d", gmtime;
my $datedir = "$cur_dir/webreq/$datestr";
if (! -e $datedir ) {
    my $rc = system "mkdir -p $datedir";
    if ($rc) {
        my $status = $rc >> 8;
        print STDERR  "failed to create working directory $datedir: $rc $status";
        exit $PS_EXIT_CONFIG_ERROR;
    }
}

my $request_name = $req_name_prefix . '_' . get_webreq_num();
my $request_file = "$datedir/$request_name.fits";
{
    my $command = "$psmkreq --req_name $request_name  --output $request_file @ARGV";

if (0) {
open DEBUG, ">>/tmp/pstamp.debug.log";
print DEBUG "\ncommand is: $command\n";
close DEBUG;
}

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        print STDERR @$stderr_buf;
        print STDERR "\ncommand was: $command\n";
        die("Unable to perform psmkreq: $error_code");
    }
}

# Queue the request
my $req_id = 0;
unless ($pretend) {

    my $command = "$pstamptool -addreq -uri $request_file -ds_id 0";
    $command .= " -label $label";
    $command .= " -username $username" if $username;
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        print STDERR @$stderr_buf;
        die("Unable to perform pstamptool -addreq: $error_code");
    }
    $req_id = ${$stdout_buf}[0];
    chomp $req_id;
}

print "$req_id $request_name";

exit 0;

# Ask the database for the next web request number
sub get_webreq_num
{
    my $command = "$pstamptool -getwebrequestnum";
    $command .= " -dbname $dbname" if $dbname;
    $command .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        print STDERR @$stderr_buf;
        die("Unable to perform pstamptool -getwebrequestnum: $error_code");
    }
    my $webreq_num = ${$stdout_buf}[0];
    chomp $webreq_num;

    if (!$webreq_num) {
        die("pstamptool -getwebreqnum returned no value");
    }

    # print STDERR "webreq_num $webreq_num\n";

    return $webreq_num;
}
