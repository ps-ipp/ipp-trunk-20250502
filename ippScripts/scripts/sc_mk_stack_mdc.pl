#! /usr/bin/env perl                                                                                                                                                       
use Carp;
use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );


#my $remote_root = '/scratch3/watersc1/';

my ($compmap_file,$remote_root);
GetOptions(
    'compmap=s'   => \$compmap_file,
    'remote_root=s' => \$remote_root,
    ) or pod2usage( 2 );

pod2usage( -msg => "Required options: --compmap", -exitval => 3) unless
    defined($remote_root) and
    defined($compmap_file);

my %component_map = ();
open(CM,$compmap_file) || die "Cannot find requested compmap file";
while(<CM>) {
    chomp;
    my ($symbolic_name,$disk_name) = split /\s+/;
    $component_map{$symbolic_name} = $disk_name;
}
close(CM);

my $counter = 0;

foreach my $path_base (@ARGV) {
    my $mdc_content = "";
    my $file = "";
    my $error = 0;

#   print "PB: $path_base\n";

    #header
    $mdc_content .= "INPUT${counter}  METADATA\n";

    # check each file against its possible locations, incrementing the error counter if not found
    $file = do_checks("${path_base}.fits",$error);
    $mdc_content .=  "   IMAGE           STR     $file\n";

    $file = do_checks("${path_base}.mask.fits",$error);
    $mdc_content .=  "   MASK            STR     $file\n";

    $file = do_checks("${path_base}.wt.fits",$error);
    $mdc_content .=  "   VARIANCE        STR     $file\n";
    
    $file = do_checks("${path_base}.psf",$error);
    $mdc_content .=  "   PSF             STR     $file\n";
    
    $file = do_checks("${path_base}.cmf",$error);
    $mdc_content .=  "   SOURCES         STR     $file\n";

    # This is static, and isn't used by ppStack, but I believe needs to exist in the mdc. 2014-08-28 czw
    $mdc_content .=  "   BKGMODEL        STR     ${path_base}.mdl.fits\n";
    $mdc_content .=  "END\n\n";
#    print "MDC: $mdc_content\n";
    # If we had no errors, print it and move on.
    if ($error == 0) {
	print $mdc_content;
    }

    $counter++;
}

sub do_checks {
    my ($cfile,$error) = @_;
#    print  ">$cfile<\n";
    if (! ((-e $cfile)&&(-s $cfile))) { # This cfile does not exist or is zero size.
	my $disk = $component_map{$cfile};
	$cfile = "${disk}";
	if (! ((-e $cfile)&&(-s $cfile))) { # The other expected cfile did not exist.
#	    $cfile = '';
	    $error++;
	    die "Missing cfile $cfile!";
	}
    }
    return($cfile);
}
    
