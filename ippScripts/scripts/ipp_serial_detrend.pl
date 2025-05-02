#!/usr/bin/env perl

use warnings;
use strict;

use Getopt::Long;
use Pod::Usage qw( pod2usage );
use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config qw( caturi );
use Data::Dumper;

my ($dbname,                    # Database name to use
    $workdir_global,            # Global working directory
    $verbose,                   # Produce verbose output?
    );
GetOptions(
           'dbname=s'  => \$dbname,
           'workdir=s' => \$workdir_global,
           'verbose'   => \$verbose,
) or pod2usage( 2 );

pod2usage(
          -msg => "Required options: --dbname",
          -exitval => 3,
          ) unless defined $dbname;

my $mdcParser = PS::IPP::Metadata::Config->new; # Metadata config parser
my $ipprc = PS::IPP::Config->new(); # IPP configuration

# Look for programs we need
my $missing_tools;
my $dettool = can_run('dettool') or (warn "Can't find dettool" and $missing_tools = 1);
my $detrend_process_imfile = can_run('detrend_process_imfile.pl')  or (warn "Can't find detrend_process_imfile.pl" and $missing_tools = 1);
my $detrend_process_exp = can_run('detrend_process_exp.pl') or (warn "Can't find detrend_process_exp.pl" and $missing_tools = 1);
my $detrend_stack = can_run('detrend_stack.pl') or (warn "Can't find detrend_stack.pl" and $missing_tools = 1);
my $detrend_norm_calc = can_run('detrend_norm_calc.pl') or (warn "Can't find detrend_norm_calc.pl" and $missing_tools = 1);
my $detrend_norm_apply = can_run('detrend_norm_apply.pl') or (warn "Can't find detrend_norm_apply.pl" and $missing_tools = 1);
my $detrend_norm_exp = can_run('detrend_norm_exp.pl') or (warn "Can't find detrend_norm_exp.pl" and $missing_tools = 1);
my $detrend_resid_imfile = can_run('detrend_resid_imfile.pl') or (warn "Can't find detrend_resid_imfile.pl" and $missing_tools = 1);
my $detrend_resid_exp = can_run('detrend_resid_exp.pl') or (warn "Can't find detrend_resid_exp.pl" and $missing_tools = 1);
my $detrend_reject_exp = can_run('detrend_reject_exp.pl') or (warn "Can't find detrend_reject_exp.pl" and $missing_tools = 1);
die "Can't find required tools.\n" if $missing_tools;

# Process raw imfiles
{
    my $command = "$dettool -toprocessedimfile -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get detrend raw list: $error_code\n" if not $success;

    my @whole = split /\n/, join( '', @$stdout_buf );
    my @single = ();

    while ( scalar @whole > 0 ) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
            push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) ) or
                die "Unable to parse output from dettool.\n";

            foreach my $item (@$list) {
                my $det_id = $item->{det_id};
                my $det_type = $item->{det_type};
                my $exp_tag = $item->{exp_tag};
                my $exp_id = $item->{exp_id};
                my $class_id = $item->{class_id};
                my $uri = $item->{uri};
                my $camera = $item->{camera};
                my $workdir = $item->{workdir};
                my $reduction = $item->{reduction};

                $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
                die "No working directory specified.\n" unless defined $workdir;

                my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", $exp_tag, "$exp_tag.detproc.$det_id" );
                $ipprc->outroot_prepare( $outroot );

                my $command = "$detrend_process_imfile --det_id $det_id --exp_tag $exp_tag --exp_id $exp_id --class_id $class_id --det_type $det_type --input_uri $uri --camera $camera --dbname $dbname --outroot $outroot";
                $command .= " --reduction $reduction" if defined $reduction;
                $command .= " --verbose" if defined $verbose;
                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run( command => $command, verbose => 1 );
                die "Unable to do raw imfile processing on $exp_tag $class_id: $error_code\n" if not $success;
            }

            @single = ();

        }
    }
}

# Process raw exposures
{
    my $command = "$dettool -toprocessedexp -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get detrend raw list: $error_code\n" if not $success;
    my $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
        die "Unable to parse output from dettool.\n";

    foreach my $item (@$list) {
        my $exp_tag = $item->{exp_tag};
        my $exp_id = $item->{exp_id};
        my $camera = $item->{camera};
        my $det_id = $item->{det_id};
        my $det_type = $item->{det_type};
        my $workdir = $item->{workdir};

        $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
        die "No working directory specified.\n" unless defined $workdir;

        my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", $exp_tag, "$exp_tag.detproc.$det_id" );
        $ipprc->outroot_prepare( $outroot );

        my $command = "$detrend_process_exp --det_id $det_id --det_type $det_type --exp_tag $exp_tag --exp_id $exp_id --camera $camera --dbname $dbname --outroot $outroot";
        $command .= " --verbose" if defined $verbose;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to do raw exposure processing on $det_id $exp_tag: $error_code\n" if not $success;
    }
}

# Stack
{
    my $command = "$dettool -tostacked -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get stack list: $error_code\n" if not $success;
    my $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
        die "Unable to parse output from dettool.\n";

    foreach my $item (@$list) {
        my $det_id = $item->{det_id};
        my $iteration = $item->{iteration};
        my $class_id = $item->{class_id};
        my $det_type = $item->{det_type};
        my $camera = $item->{camera};
        my $workdir = $item->{workdir};
        my $reduction = $item->{reduction};

        $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
        die "No working directory specified.\n" unless defined $workdir;

        my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", "$camera.$det_type.$det_id.$iteration" );
        $ipprc->outroot_prepare( $outroot );


        my $command = "$detrend_stack --det_id $det_id --iteration $iteration --class_id $class_id --det_type $det_type --camera $camera --dbname $dbname --outroot $outroot";
        $command .= " --reduction $reduction" if defined $reduction;
        $command .= " --verbose" if defined $verbose;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to stack detrend $det_id $iteration $class_id: $error_code\n" if not $success;
    }
}

# Calculate normalisation
{
    my $command = "$dettool -tonormalizedstat -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get normalise calculation list: $error_code\n" if not $success;
    my $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
        die "Unable to parse output from dettool.\n";

    foreach my $item (@$list) {
        my $det_id = $item->{det_id};
        my $iteration = $item->{iteration};
        my $det_type = $item->{det_type};
        my $camera = $item->{camera};
        my $workdir = $item->{workdir};

        $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
        die "No working directory specified.\n" unless defined $workdir;

        my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", "$camera.$det_type.normstat.$det_id.$iteration" );
        $ipprc->outroot_prepare( $outroot );

        my $command = "$detrend_norm_calc --det_id $det_id --iteration $iteration --det_type $det_type --dbname $dbname --outroot $outroot";
        $command .= " --verbose" if defined $verbose;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to calculate normalisation for $det_id $iteration: $error_code\n" if not $success;
    }
}

# Apply normalisation
{
    my $command = "$dettool -tonormalize -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get normalisation list: $error_code\n" if not $success;
    my $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
        die "Unable to parse output from dettool.\n";

    foreach my $item (@$list) {
        my $det_id = $item->{det_id};
        my $iteration = $item->{iteration};
        my $det_type = $item->{det_type};
        my $class_id = $item->{class_id};
        my $value = $item->{norm};
        my $uri = $item->{uri};
        my $camera = $item->{camera};
        my $workdir = $item->{workdir};

        $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
        die "No working directory specified.\n" unless defined $workdir;

        my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", "$camera.$det_type.norm.$det_id.$iteration" );
        $ipprc->outroot_prepare( $outroot );

        my $command = "$detrend_norm_apply --det_id $det_id --iteration $iteration --class_id $class_id --value $value --input_uri $uri --camera $camera --det_type $det_type --dbname $dbname --outroot $outroot";
        $command .= " --verbose" if defined $verbose;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to apply normalisation for $det_id $iteration $class_id: $error_code\n" if not $success;
    }
}

# Examine normalised exposure
{
    my $command = "$dettool -tonormalizedexp -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get normalised exposures list: $error_code\n" if not $success;
    my $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
        die "Unable to parse output from dettool.\n";

    foreach my $item (@$list) {
        my $det_id = $item->{det_id};
        my $iteration = $item->{iteration};
        my $det_type = $item->{det_type};
        my $class_id = $item->{class_id};
        my $value = $item->{norm};
        my $uri = $item->{uri};
        my $camera = $item->{camera};
        my $workdir = $item->{workdir};

        $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
        die "No working directory specified.\n" unless defined $workdir;

        my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", "$camera.$det_type.normexp.$det_id.$iteration" );
        $ipprc->outroot_prepare( $outroot );

        my $command = "$detrend_norm_exp --det_id $det_id --iteration $iteration --camera $camera --det_type $det_type --dbname $dbname --outroot $outroot";
        $command .= " --verbose" if defined $verbose;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to examine normalised exposure for $det_id $iteration: $error_code\n" if not $success;
    }
}

# Get residuals
{
    my $command = "$dettool -toresidimfile -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get residual processing list: $error_code\n" if not $success;

    my @whole = split /\n/, join( '', @$stdout_buf );
    my @single = ();

    while ( scalar @whole > 0 ) {
        my $value = shift @whole;
        push @single, $value;
        if ($value =~ /^\s*END\s*$/) {
            push @single, "\n";

            my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) ) or
                die "Unable to parse output from dettool.\n";

            foreach my $item (@$list) {
                my $exp_tag = $item->{exp_tag};
                my $exp_id = $item->{exp_id};
                my $camera = $item->{camera};
                my $det_id = $item->{det_id};
                my $iteration = $item->{iteration};
                my $class_id = $item->{class_id};
                my $det_type = $item->{det_type};
                my $detrend = $item->{det_uri};
                my $uri = $item->{uri};
                my $mode = $item->{mode};
                my $workdir = $item->{workdir};
                my $reduction = $item->{reduction};

                $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
                die "No working directory specified.\n" unless defined $workdir;

                my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", $exp_tag, "$exp_tag.detresid.$det_id.$iteration" );
                $ipprc->outroot_prepare( $outroot );

                my $command = "$detrend_resid_imfile --det_id $det_id --iteration $iteration --exp_tag $exp_tag --exp_id $exp_id --class_id $class_id --det_type $det_type --camera $camera --input_uri $uri --mode $mode --dbname $dbname --outroot $outroot";
                $command .= " --reduction $reduction" if defined $reduction;
                $command .= " --detrend $detrend" if defined $detrend;
                $command .= " --verbose" if defined $verbose;

                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                    run( command => $command, verbose => 1 );
                die "Unable to do residual processing on $exp_tag $class_id: $error_code\n" if not $success;
            }

            @single = ();

        }
    }
}

# Reject based on imfiles
{
    my $command = "$dettool -toresidexp -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get residual imfile list: $error_code\n" if not $success;
    my $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
        die "Unable to parse output from dettool.\n";

    foreach my $item (@$list) {
        my $exp_tag = $item->{exp_tag};
        my $exp_id = $item->{exp_id};
        my $camera = $item->{camera};
        my $det_id = $item->{det_id};
        my $iteration = $item->{iteration};
        my $det_type = $item->{det_type};
        my $workdir = $item->{workdir};

        $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
        die "No working directory specified.\n" unless defined $workdir;

        my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", $exp_tag, "$exp_tag.detresid.$det_id.$iteration" );
        $ipprc->outroot_prepare( $outroot );

        my $command = "$detrend_resid_exp --det_id $det_id --iteration $iteration --exp_tag $exp_tag --exp_id $exp_id --det_type $det_type --camera $camera --dbname $dbname --outroot $outroot";
        $command .= " --verbose" if defined $verbose;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to do imfile rejection on $exp_tag $det_id $iteration: $error_code\n" if not $success;
    }
}

# Reject based on exposures
{
    my $command = "$dettool -todetrunsummary -dbname $dbname";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run( command => $command, verbose => 1 );
    die "Unable to get residual exposure list: $error_code\n" if not $success;
    my $list = parse_md_list( $mdcParser->parse( join( '', @$stdout_buf ) ) ) or
        die "Unable to parse output from dettool.\n";

    foreach my $item (@$list) {
        my $camera = $item->{camera};
        my $det_id = $item->{det_id};
        my $iteration = $item->{iteration};
        my $det_type = $item->{det_type};
        my $workdir = $item->{workdir};

        $workdir = $workdir_global unless defined $workdir and $workdir ne "NULL";
        die "No working directory specified.\n" unless defined $workdir;

        my $outroot = caturi( $workdir, "$camera.$det_type.$det_id", "$camera.$det_type.$det_id.$iteration.detreject" );
        $ipprc->outroot_prepare( $outroot );

        my $command = "$detrend_reject_exp --det_id $det_id --iteration $iteration --det_type $det_type --camera $camera --dbname $dbname --outroot $outroot";
        $command .= " --verbose" if defined $verbose;
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run( command => $command, verbose => 1 );
        die "Unable to do exposure rejection on $det_id $iteration: $error_code\n" if not $success;
    }
}


__END__


