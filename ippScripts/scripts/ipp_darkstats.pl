#!/usr/bin/env perl

# use warnings;
# use strict;
use Carp;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use IPC::Cmd 0.36 qw( can_run run );
use IO::Handle;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );

my $ipprc = PS::IPP::Config->new(); # IPP configuration

my ($dbname, $det_id, $camera);

GetOptions('dbname=s'    => \$dbname,
	   'det_id=s'    => \$det_id,
	   'camera|c=s'  => \$camera,
	   ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;

pod2usage(
	  -msg => "USAGE: ipp_darkstats.pl --dbname (name) --det_id (id) --camera (name)",
	  -exitval => 3,
	  ) unless defined $dbname and defined $det_id and defined $camera;

$ipprc->define_camera($camera);

###  Get list of dark imfile results

# define the dettool command
my $command = "dettool -processedimfile -select_state stop"; # Command to run
$command .= " -det_id $det_id";
$command .= " -dbname $dbname" if defined $dbname;

# run the dettool command and catch the output
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $command, verbose => 0);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform dettool: $error_code", $error_code);
}

# parse the output into a list
my $mdcParser = PS::IPP::Metadata::Config->new;	# Parser for metadata config files
my $metadata = $mdcParser->parse(join "", @$stdout_buf) or
    &my_die("Unable to parse metadata config doc", $PS_EXIT_PROG_ERROR);
my $list = parse_md_list($metadata) or
    &my_die("Unable to parse metadata list", $PS_EXIT_PROG_ERROR);

my @bg_data;
my @bg_stdev_data;
my @bg_name;
my @bg_exptime;
my %components;

print STDERR "extracted the data from the database\n";

# we now have a list of imfiles; we need to extract the background for each cell
# from the stats files for each imfile
foreach my $item (@$list) {
    my $path_base = $item->{path_base};
    my $class_id = $item->{class_id};
    my $exp_time = $item->{exp_time};

    my $rootName  = $ipprc->file_resolve ($path_base);
    my $statsName = "$rootName.$class_id.stats";

    # print STDERR "rootName: $rootName : $exp_time\n";
    print STDERR "statsName: $statsName : $exp_time\n";

    my $statsFile;
    open $statsFile, $statsName;
    my @contents = <$statsFile>;
    close ($statsFile);

    # print STDERR "contents: @contents\n";

    my $parser = PS::IPP::Metadata::Config->new;	# Parser for metadata config files
    my $statsList = $parser->parse(join "", @contents) or &my_die("Unable to parse metadata for imfile stats", $PS_EXIT_SYS_ERROR);

    &parse_stats_table ($exp_time, $class_id, $statsList);
}

print STDERR "parsed the stats from the data files\n";

for (my $i = 0; $i < @bg_data; $i++) {
    $nameX = "$bg_name[$i].exp";
    $nameY = "$bg_name[$i].bg";
    push @{$nameX}, $bg_exptime[$i];
    push @{$nameY}, $bg_data[$i];
}

if (-e "output.dat") { unlink "output.dat"; }

print STDERR "dumping stats\n";
open (MANA, "|mana --norc");
MANA->autoflush;

foreach my $component (@components) {
    $nameX = "$component.exp";
    $nameY = "$component.bg";

    print MANA "delete X Y\n";

    open (DATA, ">$component.dat");
    for (my $i = 0; $i < @{$nameX}; $i++) {
	print DATA "${$nameX}[$i] ${$nameY}[$i]\n";
    }
    close (DATA);

    print MANA "data $component.dat\n";
    print MANA "read X 1 Y 2\n";
    print MANA "fit X Y 2 -clip 3 3\n";
    print MANA "output output.dat\n";
    print MANA "echo $component METADATA\n";
    print MANA "echo \"   NORDER_X  S32 2   \"\n";
    print MANA "echo \"   VAL_X00   F64 \$C0\"\n";
    print MANA "echo \"   VAL_X01   F64 \$C1\"\n";
    print MANA "echo \"   VAL_X02   F64 \$C2\"\n";
    print MANA "echo \"   NELEMENTS S32 3    \"\n";
    print MANA "echo END\n";
    print MANA "echo\n";
    print MANA "output stdout\n";

    print MANA "applyfit X Yf\n";
    print MANA "lim X Y\n";
    print MANA "clear\n";
    print MANA "box\n";
    print MANA "plot -x 2 -pt 2 -sz 1.0 -c black X Y\n";
    print MANA "plot -x 2 -pt 7 -sz 1.0 -c red X Yf\n";

    print STDERR "hit return to continue\n";
    $answer = <STDIN>;
}

close (MANA);

exit 0;

sub parse_stats_table
{
    my ($exp_time, $tag, $md) = @_;

    # descend through the fpa        
    foreach my $entry (@$md) {
	# print STDERR "name: $entry->{name}, class: $entry->{class}\n";
        # recurse on nested metadata
        if ($entry->{class} eq 'metadata') {
	    my $newtag = $tag . "_" . $entry->{name};
            &parse_stats_table ($exp_time, $newtag, $entry->{value});
        }

        if ($entry->{name} =~ /^(SAMPLE|ROBUST|FITTED|CLIPPED)/) {
            # It's a statistic of some sort
            if ($entry->{name} =~ /_STDEV$/) {
                push @bg_stdev_data, $entry->{value};
            } else {
		push @bg_name,    $tag;
                push @bg_data,    $entry->{value};
		push @bg_exptime, $exp_time;
		# print STDERR "$tag $exp_time $entry->{value}\n";
            }
	    if (!$componentsHash{$tag}) {
		push @components, $tag;
		$componentsHash{$tag} = 1;
	    }
	    next;
	} 
    }
    return 1;
}

sub my_die
{
    my $msg = shift; # Warning message on die
    my $exit_code = shift; # Exit code to add

    carp($msg);
    exit $exit_code;
}

# - get the exp_time as well from dettool
# - build an array of bg & exptime for each cell
# - fit the trend (in mana? pslib functions?)
# - write the polynomial for each cell
