#!/usr/bin/env perl

use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config qw( caturi );
use Data::Dumper;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ($dbname,			# Database name to use
    $workdir_default,		# Default working directory
    $verbose,			# Verbose operations?
    $no_op,			# No operations?
    $no_update,			# No updating?
    );
GetOptions(
	   'dbname=s' => \$dbname,
	   'workdir=s' => \$workdir_default,
	   'verbose' => \$verbose,
	   'no-op' => \$no_op,
	   'no-update' => \$no_update,
) or pod2usage( 2 );

pod2usage(
	  -msg => "Required options: --dbname",
	  -exitval => 3,
	  ) unless defined $dbname;

$workdir_default = `pwd` unless defined $workdir_default;

my $mdcParser = PS::IPP::Metadata::Config->new;	# Metadata config parser
my $ipprc = PS::IPP::Config->new; # IPP Configuration

# Look for programs we need
my $missing_tools;
my $camtool = can_run('camtool') or (warn "Can't find camtool" and $missing_tools = 1);
my $camera_exp = can_run('camera_exp.pl') or (warn "Can't find camera_exp.pl" and $missing_tools = 1);
die "Can't find required tools.\n" if $missing_tools;

# Camera exposure processing
my $list;
{
    my $command = "$camtool -pendingexp -dbname $dbname"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run( command => $command, verbose => 1 );
    die "Unable to get camera exposure list: $error_code\n" if not $success;
    $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
	die "Unable to parse output from camtool.\n";
}

foreach my $item (@$list) {
    my $cam_id = $item->{cam_id};
    my $exp_tag = $item->{exp_tag};
    my $camera = $item->{camera};
    my $workdir = $item->{workdir};
    
    $workdir = $workdir_default unless (defined $workdir or $workdir ne "NULL");
    my $outroot = caturi( $workdir, $exp_tag, "$exp_tag.cm.$cam_id" );
    $ipprc->outroot_prepare( $outroot );

    my $command = "$camera_exp --cam_id $cam_id --exp_tag $exp_tag --camera $camera --dbname $dbname --outroot $outroot";
    $command .= " --verbose" if defined $verbose;
    $command .= " --no-op" if defined $no_op;
    $command .= " --no-update" if defined $no_update;
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run( command => $command, verbose => 1 );
    die "Unable to do camera processing on $cam_id: $error_code\n" if not $success;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}


__END__


