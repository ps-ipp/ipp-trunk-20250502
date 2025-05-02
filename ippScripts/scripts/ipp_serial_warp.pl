#!/usr/bin/env perl

use warnings;
use strict;

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use File::Basename;

my $ipprc = PS::IPP::Config->new(); # IPP configuration

my @ARGS = @ARGV;

my ($dbname,			# Database name to use
    $verbose,			# Verbose operations?
    $workdir_global,		# Global working directory
    $no_op,			# No operations?
    $no_update,			# No updating?
    );
GetOptions(
	   'dbname=s' => \$dbname,
	   'verbose' => \$verbose,
	   'workdir' => \$workdir_global,
	   'no-op' => \$no_op,
	   'no-update' => \$no_update,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

pod2usage(
	  -msg => "Required options: --dbname",
	  -exitval => 3,
	  ) unless defined $dbname;

my $mdcParser = PS::IPP::Metadata::Config->new;	# Metadata config parser

# Look for programs we need
my $missing_tools;
my $warptool = can_run('warptool') or
    (warn "Can't find warptool" and $missing_tools = 1);
my $warp_skycell = can_run('warp_skycell.pl') or
    (warn "Can't find warp_skycell.pl" and $missing_tools = 1);
my $warp_overlap = can_run('warp_overlap.pl') or
    (warn "Can't find warp_overlap.pl" and $missing_tools = 1);

if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR); 
}

print "FULL COMMAND: $0 @ARGS\n\n";


# Calculate overlaps
{
    my $list;
    my $command = "$warptool -tooverlap -dbname $dbname"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run( command => $command, verbose => 1 );
    die "Unable to get warps for which to calculate overlaps: $error_code\n" if not $success;
    $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
	die "Unable to parse output from warptool.\n";

    foreach my $item (@$list) {
	my $warp_id = $item->{warp_id};
	my $cam_id = $item->{cam_id};
	my $workdir = $item->{workdir};
	my $camera = $item->{camera};
	my $tess_id = $item->{tess_id};
	
	my $command = "$warp_overlap --warp_id $warp_id --camera $camera --tess_id $tess_id --dbname $dbname";
	$command .= " --verbose" if defined $verbose;
	$command .= " --no-op" if defined $no_op;
	$command .= " --no-update" if defined $no_update;
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run( command => $command, verbose => 1 );
	die "Unable to get warp overlaps on $warp_id: $error_code\n" if not $success;
    }
}


# Warping proper
{
    my $list;
    my $command = "$warptool -towarped -dbname $dbname"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run( command => $command, verbose => 1 );
    die "Unable to get warps for warping: $error_code\n" if not $success;
    $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
	die "Unable to parse output from warptool.\n";

    foreach my $item (@$list) {
	my $warp_id = $item->{warp_id};
	my $skycell_id = $item->{skycell_id};
	my $tess_id = basename($item->{tess_id});
	my $cam_id = $item->{cam_id};
	my $workdir = $item->{workdir};
	my $camera = $item->{camera};
	
	$workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
	die "No working directory specified.\n" unless defined $workdir;
	
	my $outroot = caturi( $workdir, $tess_id, $skycell_id, "$tess_id.$skycell_id.wrp.$warp_id" );
	$ipprc->outroot_prepare( $outroot );

	my $command = "$warp_skycell --warp_id $warp_id --skycell_id $skycell_id --tess_id $tess_id --camera $camera --dbname $dbname --outroot $outroot";
	$command .= " --verbose" if defined $verbose;
	$command .= " --no-op" if defined $no_op;
	$command .= " --no-update" if defined $no_update;
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run( command => $command, verbose => 1 );
	die "Unable to do warp processing on $warp_id,$skycell_id: $error_code\n" if not $success;
    }
}



END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
