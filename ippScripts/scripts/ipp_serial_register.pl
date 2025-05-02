#!/usr/bin/env perl

use warnings;
use strict;

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use Data::Dumper;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

my ($dbname,			# Database name to use
    );
GetOptions(
	   'dbname|d=s'  => \$dbname,
) or pod2usage( 2 );

pod2usage(
	  -msg => "Required options: --dbname",
	  -exitval => 3,
	  ) unless defined $dbname;

my $mdcParser = PS::IPP::Metadata::Config->new;	# Metadata config parser

# Look for programs we need
my $missing_tools;
my $regtool = can_run('regtool') or (warn "Can't find regtool" and $missing_tools = 1);
my $register_imfile = can_run('register_imfile.pl') or (warn "Can't find register_imfile.pl" and $missing_tools = 1);
my $register_exp = can_run('register_exp.pl') or (warn "Can't find register_exp.pl" and $missing_tools = 1);
die "Can't find required tools.\n" if $missing_tools;

# Phase 0 imfile processing
{
    my $command = "$regtool -pendingimfile -dbname $dbname"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run( command => $command, verbose => 1 );
    die "Unable to get phase 0 imfile list: $error_code\n" if not $success;

    my @whole = split /\n/, join( '', @$stdout_buf );
    my @single = ();

    while ( scalar @whole > 0 ) {
	my $value = shift @whole;
	push @single, $value;
	if ($value =~ /^\s*END\s*$/) {
	    push @single, "\n";

	    my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) ) or
		die "Unable to parse output from regtool.\n";

	    foreach my $item (@$list) {
		my $exp_id = $item->{exp_id};
		my $exp_name = $item->{tmp_exp_name};
		my $class_id = $item->{tmp_class_id};
		my $uri = $item->{uri};
		
		my $command = "$register_imfile --exp_id $exp_id --tmp_class_id $class_id --tmp_exp_name $exp_name --uri $uri --dbname $dbname";
		my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		    run( command => $command, verbose => 1 );
		die "Unable to do phase 0 imfile processing on $exp_name $class_id: $error_code\n" if not $success;
	    }


	    @single = ();

	}	
    }
}

# Phase 0 exposure processing
{
    my $command = "$regtool -pendingexp -dbname $dbname"; # Command to run
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
	run( command => $command, verbose => 1 );
    die "Unable to get phase 0 exposure list: $error_code\n" if not $success;

    my @whole = split /\n/, join( '', @$stdout_buf );
    my @single = ();

    while ( scalar @whole > 0 ) {
	my $value = shift @whole;
	push @single, $value;
	if ($value =~ /^\s*END\s*$/) {
	    push @single, "\n";

	    my $list = parse_md_list( $mdcParser->parse( join( "\n", @single ) ) ) or
		die "Unable to parse output from regtool.\n";

	    foreach my $item (@$list) {
		my $exp_tag = $item->{tmp_exp_name} . '.' . $item->{exp_id};
		my $exp_id = $item->{exp_id};
		
		my $command = "$register_exp --exp_tag $exp_tag --exp_id $exp_id --dbname $dbname";
		my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
		    run( command => $command, verbose => 1 );
		die "Unable to do phase 0 exposure processing on $exp_tag: $error_code\n" if not $success;
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


