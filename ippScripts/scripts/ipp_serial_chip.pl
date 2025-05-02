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

my ($dbname,                    # Database name to use
    $workdir_default,           # Default working directory
    $verbose,                   # Verbose operations?
    $no_op,                     # No operations?
    $no_update,                 # No updating?
    );
GetOptions(
           'dbname=s' => \$dbname,
           'workdir=s' => \$workdir_default,
           'verbose' => \$verbose,
           'no-op' => \$no_op,
           'no-update' => \$no_update,
) or pod2usage( 2 );

pod2usage( -msg => "Required options: --dbname --workdir",
           -exitval => 3,
           ) unless
    defined $dbname;

$workdir_default = `pwd` unless defined $workdir_default;

my $mdcParser = PS::IPP::Metadata::Config->new; # Metadata config parser
my $ipprc = PS::IPP::Config->new; # IPP Configuration

# Look for programs we need
my $missing_tools;
my $chiptool = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $chip = can_run('chip_imfile.pl') or (warn "Can't find chip_imfile.pl" and $missing_tools = 1);
die "Can't find required tools.\n" if $missing_tools;

# Imfile processing
my @whole;                      # The whole list for processing
{
    my $command = "$chiptool -pendingimfile -dbname $dbname"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get phase 2 imfile list: $error_code\n" if not $success;
    @whole = split /\n/, join( '', @$stdout_buf );
}

my @single = ();

while ( scalar @whole > 0 ) {
    my $value = shift @whole;
    push @single, $value;
    if ($value =~ /^\s*END\s*$/) {
        push @single, "\n";

        my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) ) or
            die "Unable to parse output from chiptool.\n";

        foreach my $item (@$list) {
            my $chip_id = $item->{chip_id};
            my $exp_id = $item->{exp_id};
            my $exp_tag = $item->{exp_tag};
            my $chip_imfile_id = $item->{chip_imfile_id};
            my $camera = $item->{camera};
            my $class_id = $item->{class_id};
            my $uri = $item->{uri};
            my $reduction = $item->{reduction};
            my $state = $item->{state};
            my $workdir = $item->{workdir};
            $workdir = $workdir_default unless (defined $workdir or $workdir ne "NULL");

            my $outroot = caturi( $workdir, $exp_tag, "$exp_tag.ch.$chip_id" );
            $ipprc->outroot_prepare( $outroot );

            my $command = "$chip --chip_id $chip_id --chip_imfile_id $chip_imfile_id --exp_id $exp_id --class_id $class_id --uri $uri --dbname $dbname --camera $camera --outroot $outroot --run-state $state";
            $command .= " --reduction $reduction" if defined $reduction;
            $command .= " --verbose" if defined $verbose;
            $command .= " --no-op" if defined $no_op;
            $command .= " --no-update" if defined $no_update;
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run( command => $command, verbose => 1 );
            die "Unable to do phase 2 processing on $chip_id $class_id: $error_code\n" if not $success;
        }

        @single = ();

    }
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}


__END__


