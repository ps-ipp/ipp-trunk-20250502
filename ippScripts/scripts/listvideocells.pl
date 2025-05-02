#!/usr/bin/env perl

use Carp;
use warnings;
use strict;

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $fhead = can_run('fhead') or (warn "Can't find fhead" and $missing_tools = 1);

my ($uri, $verbose);

# Parse the command-line arguments
GetOptions(
           'file=s'         => \$uri,        # uri to examine
           'verbose'        => \$verbose,    # Print stuff?
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --file",
           -exitval => 3) unless
    defined $uri;

my $num_ext = 0;
my $num_video_ext = 0;
for (my $i = 0; ; $i++) {
    my $command = "$fhead -x $i $uri | grep EXTNAME";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        if ($error_code != 1) {
            carp("Unable to perform $command: $error_code");
            exit $error_code;
        }
        last;
    }
    $num_ext++;

    my $output = join "", @$stdout_buf;
    chomp $output;
    last if ! $output;

    my ($exttag, undef, $extname) = split " ", $output;

    last if !$exttag;

    die ("unexpected output from fhead: $output") unless $exttag eq 'EXTNAME';

    if ($extname =~ /video_table/) {
        $num_video_ext++;
        # get rid of the quotes in the ftable output
        $extname =~ s/\'//g;
        my (undef, undef, $cell) = split "_", $extname;
        print "$cell\n";
    }
}

die "no extensions found in $uri" unless $num_ext > 0;
print "no video extensions found in $uri" unless $num_video_ext > 0;
exit 0;
