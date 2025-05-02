#!/usr/bin/env perl

# make a exposure list the exposures in a relGroup (a given night or LAP run)

use strict;
use warnings;

use Carp;
use File::Basename;
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );
use DBI;
use Time::Local;
use PS::IPP::Config 1.01 qw( :standard );
use IPC::Cmd 0.36 qw( can_run run );


my ($group_id, $group_type, $lap_id, $group_name, $release_name, $rebuild, $dbname, $no_update, $verbose);

GetOptions(
    'group_id=s'    =>      \$group_id,
    'group_type=s'  =>      \$group_type,
    'lap_id=s'      =>      \$lap_id,
    'group_name=s'  =>      \$group_name,
    'release_name=s' =>     \$release_name,
    'rebuild'       =>      \$rebuild,      # rebuild directory if it exists
    'no-update'     =>      \$no_update,
    'verbose|v'     =>      \$verbose,
) or pod2usage (2);

pod2usage( -msg => "Required options: --group_id --group_type --release_name",
           -exitval => 3) 
        unless $group_id and $release_name and $group_type ;

if ($group_type eq 'lap') {
    pod2usage( -msg => "--lap_id is required with --group_type lap",
           -exitval => 3) 
        unless $lap_id;
    $group_name = "lap_$lap_id";
} else {
    pod2usage( -msg => "--group_name is required with --group_type $group_type",
           -exitval => 3) 
        unless $group_name;
}

my $missing_tools;
my $releasetool = can_run('releasetool') or (warn "Can't find releasetool" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new() or &my_die("Unable to set up", $group_id, $PS_EXIT_CONFIG_ERROR);

# XXX: get this from site.config
my $exposureListRoot = "/data/ippc17.0/www/ipp-misc/ps1-data/single-frame";

my $outputBase = "$exposureListRoot/$release_name";

if (!-e $outputBase) {
    mkdir $outputBase or &my_die("Failed to create $outputBase", $group_id, $PS_EXIT_SYS_ERROR);
}

my $outdir = "$outputBase/$group_name";
if (-e $outdir) {
    if ($rebuild) {
        my $rc = system "rm -rf $outdir";
        if ($rc) {
            my $status = $rc >> 8;
            my_die("failed to remove existing directory $outdir: $rc $status", $group_id, $PS_EXIT_UNKNOWN_ERROR);
        }
    } else {
        my_die("directory $outdir already exists and --rebuild not supplied", $group_id, $PS_EXIT_UNKNOWN_ERROR);
    }
}

mkdir $outdir or my_die ("failed to create $outdir", $group_id, $PS_EXIT_UNKNOWN_ERROR);

my $oh;
my $outfile = "$outdir/index.html";
open $oh, ">$outfile" or my_die("failed to open $outfile", $group_id, $PS_EXIT_UNKNOWN_ERROR);

my $oh2;
my $outfile2 = "$outdir/index.txt";
open $oh2, ">$outfile2" or my_die("failed to open $outfile2", $group_id, $PS_EXIT_UNKNOWN_ERROR);

my $title = "PS1 Exposure List for";
if ($group_type eq 'lap') {
    $title .= " $release_name LAP run $lap_id";
} else {
    $title .= " $group_name";
}

my $head = "
<HTML>
<head>
<title>$title</title>
</head>
<body>
<h1>$title</title>
</h1>
<br>
<a href=..>Up</a>
<br>
<br>
<table border=1>
";

print $oh "$head\n";


# find all exposures in this relGroup
my $query = "
SELECT 
    exp_name, 
    exp_id,
    obs_mode,
    filter,
    DEGREES(ra) as RA,
    DEGREES(decl) AS 'DEC',
    dateobs AS 'Date Time (UTC)',
    comment
FROM 
    relGroup JOIN relExp USING(group_id) join rawExp USING(exp_id) 
    JOIN camRun USING(cam_id) JOIN camProcessedExp using(cam_id)
WHERE 
    group_id = $group_id
    and camProcessedExp.quality = 0 AND camProcessedExp.fault = 0;
";

my $dbh = open_db();
my $stmt = $dbh->prepare($query);

$stmt->execute() or my_die("failed to execute query: $stmt->errstr\n", $group_id, $PS_EXIT_UNKNOWN_ERROR);

printf STDERR "%4d exposures in group for $group_name\n", $stmt->rows;

# $stmt->dump_results(1000);

# start the table with the header fields
print $oh "<tr>\n";
print $oh2 "# ";

my $headings = $stmt->{NAME};
my $numcols = $stmt->{NUM_OF_FIELDS};
for (my $col = 0; $col < scalar @$headings; $col++) {
    print $oh "<th>$headings->[$col]</th>\n";
    print $oh2 "$headings->[$col]|";
    if ($col == 0) {
        # add a column
        print $oh "<th>Retrieve</th>\n";
        print $oh2 "geturl|";
    }
}
print $oh "</tr>\n";
print $oh2 "\n";

# my $hostname = hostname();
my $get_link_fmt = "/getsmf.php?exp_name=%s&release=$release_name";
my $link_fmt = "<a href=%s>Get smf</a>";
while (my $row = $stmt->fetchrow_arrayref()) {
    my $exp_name = $row->[0];
    print $oh "<tr align=center>";
    for (my $i=0; $i < $numcols; $i++) {
        my $val = $row->[$i];
        $val = 'null' if !defined $val;
        my $column = $headings->[$i];
        if ($column eq 'filter') {
            $val = ' ' . (substr $val, 0, 1) . ' ';
        } elsif ($column eq 'RA' or $column eq 'DEC') {
            $val = 0 if $val eq 'null';
            $val = sprintf "%9.6f", $val;
        }
        
        my $align;
        my $extra;
        if ($column eq 'comment') {
            $align = 'left';
        } else {
            $align = 'center';
        }

        print $oh "<td align=$align>$val</td> ";
        print $oh2 "$val|";
        if ($i == 0) {
            # add the link column
            my $getsmf = sprintf $get_link_fmt, $exp_name;
            print $oh2 " $getsmf ";

            my $html_link = sprintf $link_fmt, $getsmf;
            print $oh "<td>$html_link</td> ";
        }
    }
    print $oh "</tr>\n";
    print $oh2 "\n";
}
print $oh "\n</table>\n";
print $oh "\n</body>\n";

close $oh;
close $oh2;

print STDERR "build exposure lists for group_id $group_id in $outdir\n";
{
    my $command = "$releasetool -updaterelgroup";
    $command .= " -group_id $group_id";
    $command .= " -set_exp_list_path $outdir";
    $command .= ' -set_state full';
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = $error_code >> 8;
            my_die("failed to run $command $error_code\n", $group_id, $error_code);
        }
    } else {
        print STDERR "Skipping $command\n";
    }
}


exit (0);

# All done.

sub open_db {
    # XXX Use site.config
    my $dbserver = "ippdb01";
    my $dbname = "gpc1";
    my $dbuser = "ipp";
    my $dbpassword = "ipp";

    my $dsn = "DBI:mysql:host=$dbserver;database=$dbname";

    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) 
        or my_die("Cannot connect to database.\n", $group_id, $PS_EXIT_UNKNOWN_ERROR);

    return $dbh;
}

sub my_die
{
    my $msg = shift;
    my $group_id = shift;
    my $exit_code = shift;

    $exit_code = $PS_EXIT_PROG_ERROR unless $exit_code;

    carp($msg);

    my $command = "$releasetool -updaterelgroup";
    $command .= " -group_id $group_id";
    $command .= " -set_fault $exit_code";
    $command .= " -dbname $dbname" if defined $dbname;
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = $error_code >> 8;
            carp("failed to run $command $error_code\n", $error_code);
            exit $error_code;
        }
    } else {
        print STDERR "Skipping $command\n";
    }
    exit $exit_code;
}
