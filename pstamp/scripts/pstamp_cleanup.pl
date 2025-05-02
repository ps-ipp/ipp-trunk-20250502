#!/bin/env perl
###
### pstamp_cleanup.pl
### delete all data associated with a postage stamp request
###  
### This script should probably be called request_cleanup.pl since it handles more than postage stamp requests
###

use warnings;
use strict;

use Sys::Hostname;
use Getopt::Long qw( GetOptions );
use File::Basename qw( basename dirname);
use File::Copy;
use POSIX qw( strftime );
use Carp;
use IPC::Cmd 0.36 qw( can_run run );

use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config qw( :standard );
use PS::IPP::PStamp::Job qw( :standard );

my $req_id;
my $uri;
my $name;
my $outdir;
my $reqType;
my $redirect_output;
my $product;
my $label;
my $verbose;
my $dbname;
my $dbserver;
my $no_update;

GetOptions(
    'req_id=s'          =>  \$req_id,
    'name=s'            =>  \$name,
    'outdir=s'          =>  \$outdir,
    'reqType=s'         =>  \$reqType,
    'product=s'         =>  \$product,
    'uri=s'             =>  \$uri,
    'redirect-output'   =>  \$redirect_output,
    'verbose'           =>  \$verbose,
    'dbname=s'          =>  \$dbname,
    'dbserver=s'        =>  \$dbserver,
    'no-update'         =>  \$no_update,
);

if ($verbose) {
    my $host = hostname();
    print "\n\n";
    print "Starting script $0 on $host\n\n";
}

my $missing_tools;

my $pstamptool  = can_run('pstamptool')  or (warn "Can't find pstamptool"  and $missing_tools = 1);
my $dsreg = can_run('dsreg') or (warn "Can't find dsreg" and $missing_tools = 1);

if ($missing_tools) {
    warn("Can't find required tools.");
    exit ($PS_EXIT_CONFIG_ERROR);
}


die("--req_id --name --product and --outdir are required") if !defined($req_id) or !defined($product) or !defined($name) or !defined $outdir;

my $ipprc = PS::IPP::Config->new(); # IPP Configuration

my $pstamp_workdir = metadataLookupStr($ipprc->{_siteConfig}, 'PSTAMP_WORKDIR');
exit ($PS_EXIT_CONFIG_ERROR) unless defined $pstamp_workdir; # lookup failure outputs a message

if (!$dbserver) {
    $dbserver =  metadataLookupStr($ipprc->{_siteConfig}, 'PS_DBSERVER');
}

if (0) {
my_die("Cleanup not yet supported for reqType: $reqType", $req_id, $PS_EXIT_UNKNOWN_ERROR)
    if ($reqType ne "pstamp") and ($reqType ne "NULL") and ($reqType ne "dquery");
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

if ($product ne 'NULL' and $name ne 'NULL') {
    my $command = "$dsreg --product $product --del $name --rm --force";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        my_die("Unable to perform $command error code: $error_code", $req_id, $PS_EXIT_UNKNOWN_ERROR);
    }
}

{
    my $command = "$pstamptool -listfile -req_id $req_id";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        die("Unable to perform $command error code: $error_code");
    }
    my $output = join "", @$stdout_buf;
    if ($output) {
        my $files = parse_md_fast($mdcParser, $output);
        foreach my $file (@$files) {
            $ipprc->file_delete($file->{path}); 
        }
    }
}

# now go find the workdir for this request 
# XXX: we finally *have* to store this in the database
$outdir = undef if $outdir eq "NULL";

if ($outdir) {
    delete_workdir($outdir);
} else {
    if ($uri ne 'NULL') {
        # use the URI to find the workdir
        $outdir = dirname($uri);
        if (index($outdir, $pstamp_workdir) == 0) {
            print "pstamp workdir for req_id $req_id is $outdir\n";
            delete_workdir($outdir);
        } else {
            print "no outdir found for $req_id\n";
        }
    }
    if (!$outdir) {
        # gotta go look for it in the subdirectories of $pstamp_workdir
        my $command = "find $pstamp_workdir -maxdepth 2 -type d -name $req_id";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            my $rc = $error_code >> 8;
            my_die("Unable to perform $command error code: $error_code", $req_id, $rc);
        }
        if ($stdout_buf) {
            # for historical reasons (a bug) there may be more than one outdir
            my @outdirs = split "\n", join "", $stdout_buf;
            foreach $outdir (@outdirs) {
                chomp $outdir;
                delete_workdir($outdir);
            }
        } else {
            print "no outdir found for $req_id\n";
        }
    }
}


{
    my $command = "$pstamptool -updatereq -req_id $req_id -set_state cleaned";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            die("Unable to perform $command error code: $error_code");
        }
    } else {
        print "skipping $command\n";
    }
}

exit 0;


sub delete_workdir {
    my $dir = shift;

    if (!-e $dir) {
        print "outdir $dir does not exist\n";
        return 0;
    }
    if (!-d $dir) {
        print "outdir $dir is not a directory\n";
        return 0;
    }

    # do an ls of the directory before starting
#    print "directory listing for $dir\n";
#    system "ls $dir";

    my $command = "rm -rf $dir";
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            if (-e $dir) {
                my $rc = $error_code >> 8;
                my_die("Unable to perform $command return status: $rc", $req_id, $rc);
            } else {
                print STDERR "rm claimed to fail but directory no longer exists\n";
            }
        }
    } else {
        print "skipping $command\n";
    }
    return 0;
}

sub my_die {
    my $msg = shift;
    my $req_id = shift;
    my $fault = shift;

    carp($msg);

    my $command = "$pstamptool -updatereq -req_id $req_id  -set_fault $fault";
    $command   .= " -dbname $dbname" if $dbname;
    $command   .= " -dbserver $dbserver" if $dbserver;
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            die("Unable to perform $command error code: $error_code");
        }
    } else {
        print "skipping $command\n";
    }
    exit $fault;
}
