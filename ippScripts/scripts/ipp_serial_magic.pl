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
    $identifier,                # Identifier to process
    $workdir_default,           # Default working directory
    $verbose,                   # Verbose operations?
    $no_op,                     # No operations?
    $no_update,                 # No updating?
    );
GetOptions( 'dbname=s' => \$dbname,
            'magic_id=s' => \$identifier,
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
my $magictool = can_run('magictool') or (warn "Can't find magictool" and $missing_tools = 1);
my $magic_tree = can_run('magic_tree.pl') or (warn "Can't find magic_tree.pl" and $missing_tools = 1);
my $magic_process = can_run('magic_process.pl') or (warn "Can't find magic_process.pl" and $missing_tools = 1);
die "Can't find required tools.\n" if $missing_tools;

# Tree
{
    my @whole;                      # The whole list for processing
    {
        my $command = "$magictool -totree -dbname $dbname"; # Command to run
        $command .= " -magic_id $identifier" if defined $identifier;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => $verbose );
        die "Unable to get magic tree list: $error_code\n" if not $success;
        @whole = split /\n/, join( '', @$stdout_buf );
    }

    my @single = ();

    while ( scalar @whole > 0 ) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
            push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) ) or
                die "Unable to parse output from magictool.\n";

            foreach my $item (@$list) {
                my $magic_id = $item->{magic_id};
                my $exp_id = $item->{exp_id};
                my $camera = $item->{camera};
                my $tess_id = $item->{tess_id};
                my $ra = $item->{ra};
                my $dec = $item->{decl};
                my $workdir = $item->{workdir};
                $workdir = $workdir_default unless (defined $workdir or $workdir ne "NULL");

                my $outroot = caturi( $workdir, $exp_id, "$exp_id.mgc.$magic_id" );
                $ipprc->outroot_prepare( $outroot );

                my $command = "$magic_tree --magic_id $magic_id --camera $camera --tess_id $tess_id --outroot $outroot --ra $ra --dec $dec --dbname $dbname";
                $command .= " --verbose" if defined $verbose;
                $command .= " --no-op" if defined $no_op;
                $command .= " --no-update" if defined $no_update;
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run( command => $command, verbose => $verbose );
                die "Unable to do magic tree on $magic_id: $error_code\n" if not $success;
            }

            @single = ();
        }
    }
}

# Process leaf
{
    my @whole;                      # The whole list for processing
    {
        my $command = "$magictool -toprocess -dbname $dbname"; # Command to run
        $command .= " -magic_id $identifier" if defined $identifier;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => $verbose );
        die "Unable to get magic tree list: $error_code\n" if not $success;
        @whole = split /\n/, join( '', @$stdout_buf );
    }

    my @single = ();

    while ( scalar @whole > 0 ) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
            push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) ) or
                die "Unable to parse output from magictool.\n";

            foreach my $item (@$list) {
                my $magic_id = $item->{magic_id};
                my $exp_id = $item->{exp_id};
                my $camera = $item->{camera};
                my $node = $item->{node};
                my $workdir = $item->{workdir};
                $workdir = $workdir_default unless (defined $workdir or $workdir ne "NULL");

                my $outroot = caturi( $workdir, $exp_id, "$exp_id.mgc.$magic_id.$node" );
                $ipprc->outroot_prepare( $outroot );

                my $command = "$magic_process --magic_id $magic_id --camera $camera --node $node --outroot $outroot --dbname $dbname";
                $command .= " --verbose" if defined $verbose;
                $command .= " --no-op" if defined $no_op;
                $command .= " --no-update" if defined $no_update;
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run( command => $command, verbose => $verbose );
                die "Unable to do magic process on $magic_id $node: $error_code\n" if not $success;
            }

            @single = ();
        }
    }
}



END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}


__END__


