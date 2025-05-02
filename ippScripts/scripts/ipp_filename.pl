#!/usr/bin/env perl

use warnings;
use strict;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
 
# use these to check the apache environment
#print "$ENV{'PATH'}\n";
#print "$ENV{'PERL5LIB'}\n";
#print "NEB_SERVER: $ENV{'NEB_SERVER'}<br>\n";

# EAM: check for missing NEB_SERVER and exit with an error
unless (defined($ENV{'NEB_SERVER'})) {
    print "NEB_SERVER not defined in ipp_filename.pl\n";
    exit (5);
}
if ($ENV{'NEB_SERVER'} eq "") {
    print "NEB_SERVER not set in ipp_filename.pl\n";
    exit (6);
}

use PS::IPP::Config;
my $ipprc = PS::IPP::Config->new();
  
my ($filerule, $class_id, $basename, $camera, $touch);

GetOptions('filerule=s'    => \$filerule,
	   'class_id=s'    => \$class_id,
	   'basename=s'    => \$basename,
	   'camera|c=s'    => \$camera,
           'touch'         => \$touch,
	   ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
    -msg => "Required options: --filerule --class_id --basename --camera",
    -exitval => 3,
) unless defined $basename
    and defined $filerule 
    and defined $class_id 
    and defined $camera;

$touch = 0 unless (defined $touch);

$ipprc->define_camera($camera);

# print "$filerule\n";
# print "$basename\n";
# print "$camera\n";
# print "$class_id\n";

my $filename = $ipprc->filename($filerule, $basename, $class_id);
# print "$filename\n";
# print "touch: $touch\n";

my $realname = $ipprc->file_resolve( $filename, $touch );
if (not defined $realname) {
    print "nebulous file $filename not found\n";
    exit (1);
}

print "$realname\n";

1;

__END__
