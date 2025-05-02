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

my ($dbname,			# Database name to use
    $verbose,			# Verbose operations?
    $workdir_global,		# Global working directory
    $no_op,			# No operations?
    $no_update,			# No updating?
    $save_temps,		# Save temporary files?
    );
GetOptions(
	   'dbname=s' => \$dbname,
	   'verbose' => \$verbose,
	   'workdir' => \$workdir_global,
	   'no-op' => \$no_op,
	   'no-update' => \$no_update,
	   'save-temps' => \$save_temps,
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

pod2usage(
	  -msg => "Required options: --dbname",
	  -exitval => 3,
	  ) unless defined $dbname;

my $mdcParser = PS::IPP::Metadata::Config->new;	# Metadata config parser

# Look for programs we need
my $missing_tools;
my $stacktool = can_run('stacktool') or
    (warn "Can't find difftool" and $missing_tools = 1);
my $stack_skycell = can_run('stack_skycell.pl') or
    (warn "Can't find stack_skycell.pl" and $missing_tools = 1);

if ($missing_tools) {
    warn ("Can't find required tools");
    exit($PS_EXIT_CONFIG_ERROR); 
}


# Image stacking
{
    my $list;
    my $command = "$stacktool -tosum -dbname $dbname"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run( command => $command, verbose => 1 );
    die "Unable to get list of stacks: $error_code\n" if not $success;
    $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
	die "Unable to parse output from stacktool.\n";

    foreach my $item (@$list) {
	my $stack_id = $item->{stack_id};
	my $workdir = $item->{workdir};
	my $tess_id = basename($item->{tess_id});
	my $skycell_id = $item->{skycell_id};
	
	$workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
	die "No working directory specified.\n" unless defined $workdir;
	
	my $outroot = caturi( $workdir, $tess_id, $skycell_id, "$tess_id.$skycell_id.stk.$stack_id" );
	$ipprc->outroot_prepare( $outroot );

	my $command = "$stack_skycell --stack_id $stack_id --dbname $dbname --outroot $outroot";
	$command .= " --verbose" if defined $verbose;
	$command .= " --no-op" if defined $no_op;
	$command .= " --no-update" if defined $no_update;
	$command .= " --save-temps" if defined $save_temps;
	my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	    run( command => $command, verbose => 1 );
###	die "Unable to do stack for $stack_id: $error_code\n" if not $success;
    }
}


END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
