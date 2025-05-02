#!/usr/bin/env perl
#
# parse a MOPS_DETCTABILITY_QUERY table and create a results file
#
# Note: this file is currently only a placeholder which creates a fake response file
#

use strict;
use warnings;

use Getopt::Long qw( GetOptions );
use Pod::Usage qw( pod2usage );
use DBI;
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::Stats;
use PS::IPP::Metadata::List qw( parse_md_list );
use File::Copy;
use File::Basename;

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
my ($input, $output, $verbose, $save_temps, $workdir);
#
# parse args
#


GetOptions(
        'input=s'          =>     \$input,
        'output=s'        =>      \$output,
        'workdir=s'       =>      \$workdir,
        'verbose'         =>      \$verbose,
        'save-temps'      =>      \$save_temps,
) or pod2usage(2);

my $err = "";

if (!$input) {
    $err .= "--input is required\n";
}
if (!$output) {
    $err .="--output is required\n";
}

die $err if ($err);

# spit the contents of the file in text format 
# XXX Do we lose precision on the floats when we do this?

my $query_text = `detect_query_read --nolabel --input $input`;

my @lines = split "^", $query_text;

# first line is the header keywords
my $header = shift @lines;
chomp $header;
die "failed to parse $input" unless $header;

my ($query_id, $fpa_id, $dateobs, $filter, $obscode) = split " ", $header;
die "failed to parse $input" unless $query_id && $fpa_id && $dateobs && $filter && $obscode;

if (!$workdir) {
    $workdir="/tmp";
}
my $txt_file = "$workdir/response$$.txt";
open OUT, ">$txt_file";

print OUT "$query_id $fpa_id $dateobs $filter $obscode\n";

while (my $line = shift @lines) {
    chomp $line;
    my ($rownum, $detect_n, $detect_f) = fake_dquery_response($line);

    printf OUT "%-12s %-8d %-8.4f\n", $rownum, $detect_n, $detect_f;
}
close OUT;

my $result = system "detect_response_create --input $txt_file --output $output";

unlink $txt_file unless $save_temps;

exit $result;


sub fake_dquery_response {
    my $line = shift;
    my ($rownum, $ra1, $dec1, $ra2, $dec2, $mag) = split " ", $line;

    # todo perhaps think more about these values
    my $n = rand(16);
    my $f = rand(1);

    return ($rownum, $n, $f);
}
