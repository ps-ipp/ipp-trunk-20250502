#! /usr/bin/env perl                                                                                                                                                       
use Carp;
use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


#my $remote_root = '/scratch3/watersc1/';

my ($compmap_file, $remote_root);
GetOptions(
    'compmap=s'   => \$compmap_file,
    'remote_root=s' => \$remote_root,
    ) or pod2usage( 2 );

pod2usage( -msg => "Required options: --compmap", -exitval => 3) unless
    defined($remote_root) and
    defined($compmap_file);

if (($#ARGV + 1) % 2 != 0) { 
    die "Do not have a matched pair of path_base/stack_id values $#ARGV";
}

my %component_map = ();
open(CM,$compmap_file) || die "Cannot find requested compmap file";
while(<CM>) {
    chomp;
    my ($symbolic_name,$disk_name) = split /\s+/;
    $symbolic_name =~ s%//%/%g;
    $disk_name     =~ s%//%/%g;
    $component_map{$symbolic_name} = $disk_name;
}
close(CM);

my $counter = 0;


my $mdc_content .= "INPUT  MULTI\n";
print $mdc_content;


for (my $index = 0; $index <= $#ARGV; $index += 2) {
    my $path_base = $ARGV[$index];
    my $stack_id  = $ARGV[$index+1];
    my $mdc_content = "";
    my $file = "";
    my $error = 0;
    $path_base =~ s%//%/%g;

    #header
    $mdc_content .= "INPUT  METADATA\n";
    $mdc_content .= "    STACK_ID        S64  " . $stack_id . "\n";

    # check each file against its possible locations, incrementing the error counter if not found
    $file = do_checks("${path_base}.unconv.fits",$error);
    $mdc_content .=  "   RAW:IMAGE           STR     $file\n";

    $file = do_checks("${path_base}.unconv.mask.fits",$error);
    $mdc_content .=  "   RAW:MASK            STR     $file\n";

    $file = do_checks("${path_base}.unconv.wt.fits",$error);
    $mdc_content .=  "   RAW:VARIANCE        STR     $file\n";
    
    $file = do_checks("${path_base}.unconv.num.fits",$error);
    $mdc_content .=  "   RAW:EXPNUM             STR     $file\n";
    
    $mdc_content .=  "END\n\n";

    # If we had no errors, print it and move on.
    if ($error == 0) {
	print $mdc_content;
    }

    $counter++;
}

sub do_checks {
    my ($cfile,$error) = @_;
#    print STDERR  ">$cfile<\n";
    if (! ((-e $cfile)&&(-s $cfile))) { # This cfile does not exist or is zero size.
	my $disk = $component_map{$cfile};
	$cfile = "${disk}";
	if (! ((-e $cfile)&&(-s $cfile))) { # The other expected cfile did not exist.
#    $cfile = '';
#	    $cfile = $disk;
	    $error++;
	    die "Missing cfile $cfile ($disk) $mdc_content!";
	}
    }
    return($cfile);
}
    
