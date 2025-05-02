#! /usr/bin/env perl                                                                                                                                                       
use Carp;
use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


#my $remote_root = '/scratch3/watersc1/';

my ($diff_id,$skycell_id,$dbname,$out_path_base, $remote_root);
GetOptions(
    'diff_id=s'         => \$diff_id,
    'skycell_id=s'      => \$skycell_id,
    'dbname=s'          => \$dbname,
    'out_path_base=s'   => \$out_path_base,
    'remote_root=s'     => \$remote_root,
    ) or pod2usage( 2 );

pod2usage( -msg => "Required options: --out_path_base --remote_root", -exitval => 3) unless
    defined($out_path_base) &&
    defined($diff_id) &&
    defined($skycell_id) &&
    defined($remote_root) &&
    defined($dbname);

my $counter = 0;

foreach my $file (@ARGV) {
    my $error = 0;

    #header
    
    # check each file against its possible locations, incrementing the error counter if not found
    $error = do_checks($file);

    if ($error != 0) {
	open(DB,">${out_path_base}.dbinfo");
	print DB "difftool -adddiffskyfile -diff_id ${diff_id} -skycell_id ${skycell_id} -dbname ${dbname} -quality 4242\n";
	close(DB);
	print "Failed to find required file $file for $diff_id $skycell_id\n";
	exit(1);
    }

    $counter++;
}

sub do_checks {
    my ($cfile) = @_;
    if (! ((-e $cfile)&&(-s $cfile))) { # This cfile does not exist or is zero size.
	return(1);
    }
    return(0);
}
