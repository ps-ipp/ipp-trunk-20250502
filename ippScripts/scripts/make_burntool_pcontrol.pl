#!/usr/bin/env perl 
# script to generate a pcontrol.pro file to run burntool on a set of data.

use DBI;
use warnings;
use DateTime;
use Getopt::Std;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use IPC::Cmd 0.36 qw( can_run run);

use constant DB_SOCKET => '/var/run/mysqld/mysqld.sock';

getopts('hbPQ:d:',\%opt);
if (exists($opt{h})) {
    print STDERR "Usage: make_burntool_pcontrol.pl -b {-Q <SQL_WHERE> | -d <DATE> | DATE_MIN0 DATE_MAX0 DATE_MIN1 DATE_MAX1 [...]}\n";
    print STDERR "         -Q SQL_WHERE      Generate pcontrol from a SQL WHERE query.\n";
    print STDERR "                           (enclose in single quotes, and use double quotes inside to\n";
    print STDERR "                            protect from the shell)\n";
    print STDERR "         -d DATE           Specify a single day to look for.\n";
    print STDERR "         -b                Only print burntool lines.\n";
    print STDERR "         -P                PR images have bad obs_mode values. Work around that.\n";
    print STDERR "\n";
    print STDERR "         Scans the GPC1 database, and identifies \"science\" observations based on the obs_mode.\n";
    print STDERR "         Use a 30 minute safety prelude before the first science data to make sure burntool works.\n";
    
    exit(1);
}

# Load database
## change to use the site.config setting now
$dbname = 'gpc1';
#$dbserver = 'ippdb01';
$dbuser = 'ipp';
$dbpass = 'ipp';
my $ipprc =  PS::IPP::Config->new(); # IPP Configuration
my $siteConfig = $ipprc->{_siteConfig};
my $dbserver = metadataLookupStr($siteConfig, 'DBSERVER');
die "database configuration not set up" unless defined($dbserver);
$db = DBI->connect("DBI:mysql:database=${dbname};host=${dbserver};" .
                   "mysql_socket=" . DB_SOCKET(),
                   ${dbuser},${dbpass}, 
		   { RaiseError => 1, AutoCommit => 1}
		   ) or die "Unable to connect to database $DBI::errstr\n";

my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    die "Cannot find required tools.";
}
# Determine what the value of "BURNTOOL.STATE.GOOD" currently is:
$burntoolStateGood = 999;
open(LAZY,"$ppConfigDump -camera GPC1 -dump-camera - |") || die "Can't run ppConfigDump\n";
while(<LAZY>) {
    chomp;
    if ($_ =~ /BURNTOOL.STATE.GOOD\s/) {
        @line = split /\s+/;
        $burntoolStateGood = $line[2];
    }
}
close(LAZY);

# Read the nightly science config file and use that to determine which data is science
my $verbose = 0;
my $conf_cmd = "$ppConfigDump -dump-recipe NIGHTLY_SCIENCE -";
my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
    run(command => $conf_cmd, verbose => $verbose);
unless ($success) {
    $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
    &my_die("Unable to perform ppConfigDUmp: $error_code", $date, $PS_EXIT_SYS_ERROR);
}

my @target_list;
my %object_list;
my $comment_list;

my $mdcParser = PS::IPP::Metadata::Config->new;
my $metadata = $mdcParser->parse(join "", @$stdout_buf);
foreach my $entry (@{ $metadata }) {
    if (${ $entry }{name} eq 'TARGETS') {
        my @target_data = @{ ${ $entry }{value} };
        my $this_target = '';
	
        foreach my $tentry (@target_data) {
            if (${ $tentry }{name} eq 'NAME') {
                $this_target = ${ $tentry }{value};
                push @target_list, $this_target;
		$obsmode_list{$this_target} = '.*';
		$object_list{$this_target} = '.*';
		$comment_list{$this_target} = '.*';
            }
            elsif (${ $tentry }{name} eq 'OBSMODE') {
                $obsmode_list{$this_target} = ${ $tentry }{value};
		$obsmode_list{$this_target} =~ s/%//;
            }
            elsif (${ $tentry }{name} eq 'OBJECT') {
                $object_list{$this_target} = ${ $tentry }{value};
		$object_list{$this_target} =~ s/%//;
            }
            elsif (${ $tentry }{name} eq 'COMMENT') {
                $comment_list{$this_target} = ${ $tentry }{value};
		$comment_list{$this_target} =~ s/%//;
            }
	}
    }
}
if (exists($opt{P})) {
    push @target_list, 'PR';
    $obsmode_list{'PR'} = 'Unknown';
    $object_list{'PR'} = '.*';
    $comment_list{'PR'} = '.*';
}

# Zero the arrays and counters
@dates_min = ();
@dates_max = ();
$N_burntooled_images = 0;
$N_total_images = 0;
$N_ranges = 0;
# Parse SQL query and fill any valid dates to the arrays
if (exists($opt{Q})) {
    $opt{Q} =~ s/\"/\'/g;
    $sth = "SELECT dateobs FROM rawExp WHERE $opt{Q}";
    $data_ref = $db->selectall_arrayref( $sth );

    %day_keys = ();
    foreach $row_ref (@{ $data_ref }) {
	($day,$time) = split /\s+/, ${ $row_ref }[0];

	unless (exists($day_keys{$day})) {
	    push @dates_min, $day;
	    
	    ($year,$mon,$date) = split /-/, $day;
	    $date++;
	    push @dates_max, sprintf("%04d-%02d-%02d",$year,$mon,$date);
	    $day_keys{$day} = 1;
#	    print ">>>D $day $#dates_min\n";
	}
    }
}

# Read a single date, and calculate the end and insert
elsif (exists($opt{d})) {
    push @dates_min, $opt{d};
    ($year,$mon,$date) = split /-/, $opt{d};
    $date++;
    push @dates_max, sprintf("%04d-%02d-%02d",$year,$mon,$date);
}

# Just shift the supplied values into the arrays.
else {
    if ($#ARGV == -1) {
	system("$0 -h");
	exit(-1);
    }
	
    while ($#ARGV > -1) {
	push @dates_min, shift(@ARGV);
	push @dates_max, shift(@ARGV);
    }
}
unless(exists($opt{b})) {
    print_prologue();
}
# Save the query used in the pcontrol to be safe.
if (exists($opt{Q})) {
    print "##query: SELECT dateobs FROM rawExp WHERE $opt{Q}\n";
}

while ($#dates_min > -1) {
    $date_min = shift(@dates_min);
    $date_max = shift(@dates_max);
    
    # Find _ALL_ observations within the date range we care about
    $sth = "SELECT exp_id,dateobs,pon_time,exp_type,obs_mode,comment,object FROM rawExp WHERE " .
	"dateobs >= '${date_min}' AND dateobs <= '${date_max}' order by dateobs";

    $data_ref = $db->selectall_arrayref( $sth );

    $seq = 0;
    
    # Identify the science observations, and make a window earlier for burntool.
    @start_s  = (0);
    @end_s  = (0);
    @skips = (0);

    foreach $row_ref (@{ $data_ref }) {

	($exp_id,$dateobs,$pontime,$exp_type,$obs_mode,$comment,$object) = @{ $row_ref };
	# Fix nulls in the database
	if (!defined($obs_mode)) {
	    $obs_mode = ' ';
	}
	if (!defined($object)) {
	    $object = ' ';
	}
	if (!defined($comment)) {
	    $comment = ' ';
	}
	
	($date,$time) = split / /, $dateobs;
	($year,$month,$day) = split /-/, $date;
	($hour,$minute,$second) = split /:/, $time; # / # This stops emacs run-on syntax highlights.
	
	$dt = DateTime->new( year   => $year, month  => $month, day    => $day,
			     hour   => $hour, minute => $minute, second => $second,
			     nanosecond => 0, time_zone => 'Pacific/Honolulu');
	
	$st = DateTime->new( year   => $year, month  => $month, day    => $day,
			     hour   => $hour, minute => $minute, second => $second,
			     nanosecond => 0, time_zone => 'Pacific/Honolulu')->subtract(minutes => 30);
	$et = $dt->epoch();

	if (is_science($exp_type,$obs_mode,$object,$comment)) {
	    if ($start_s[-1] == 0) {
		$start_s[-1] = $et - 1800;
		$end_s[-1] = $et + 1;
	    }
	    else {
		if ($et > $end_s[-1] + 1800) {
		    push @start_s, $et - 1800;
		    push @end_s, $et + 1;
		    push @skips, 0;
		}
		else {
		    $end_s[-1] = $et + 1;
		}
	    }
	}
	
	$seq++;

    }

    # If this night has no science exposures, skip to the next one.
    if (($#start_s == 0) && ($start_s[0] == 0)) {
	next;
    }

    # Scan again and count how many exposures are between windows
    foreach $row_ref (@{ $data_ref }) {
	($exp_id,$dateobs,$pontime,$exp_type,$obs_mode,$comment,$object) = @{ $row_ref };
	if (!defined($obs_mode)) {
	    $obs_mode = ' ';
	}
	if (!defined($comment)) {
	    $comment = ' ';
	}

	($date,$time) = split / /, $dateobs;
	($year,$month,$day) = split /-/, $date;
	($hour,$minute,$second) = split /:/, $time; # / # another emacs syntax highlight bug.
	
	$dt = DateTime->new( year   => $year, month  => $month, day    => $day,
			     hour   => $hour, minute => $minute, second => $second,
			     nanosecond => 0, time_zone => 'Pacific/Honolulu');
	
	$st = DateTime->new( year   => $year, month  => $month, day    => $day,
			     hour   => $hour, minute => $minute, second => $second,
			     nanosecond => 0, time_zone => 'Pacific/Honolulu')->subtract(minutes => 30);
	$et = $dt->epoch();

	for ($i = 0; $i <= $#start_s; $i++) {
	    if (($et < $start_s[$i])&&(is_science($exp_type,$obs_mode,$object,$comment) == 0)) {
		if ($i == 0) {
		    $skips[$i]++;
		}
		else {
		    if ($et > $end_s[$i-1]) {
			$skips[$i]++;
		    }
		}
	    }
	}
    }

    # Clear out windows that overlap with previous ones so we have the minimum set
    for ($i = $#start_s; $i > 0; $i--) {
	if ($skips[$i] == 0) {
	    $trash = pop(@start_s);
	    $end = pop(@end_s);
	    $trash = pop(@skips);
	    $end_s[-1] = $end;
	}
    }
    # Print out the end points in the format burntool wants.

    for ($i = 0; $i <= $#start_s; $i++) {
	@start_t = localtime($start_s[$i]);
	@end_t = localtime($end_s[$i]);
	
	my $date_min = sprintf("%04d-%02d-%02dT%02d:%02d:%02d",
			       $start_t[5] + 1900,$start_t[4] + 1,$start_t[3],
			       $start_t[2],$start_t[1],$start_t[0]);
	my $date_max = sprintf("%04d-%02d-%02dT%02d:%02d:%02d",
			       $end_t[5] + 1900,$end_t[4] + 1,$end_t[3],
			       $end_t[2],$end_t[1],$end_t[0]);

	my $total_images_sth = "SELECT count(exp_id) from rawImfile where dateobs >= '$date_min' AND dateobs <= '$date_max'";
	my $im_ref = $db->selectall_arrayref( $total_images_sth );
	$N_total_images += ${ $im_ref }[0][0];

	my $burntooled_images_sth = "SELECT count(exp_id) from rawImfile where dateobs >= '$date_min' AND dateobs <= '$date_max' AND abs(burntool_state) = $burntoolStateGood";
	$im_ref = $db->selectall_arrayref( $burntooled_images_sth );
	$N_burntooled_images += ${ $im_ref }[0][0];
	

	printf(" burntool %s %s\n",$date_min,$date_max);
    }
    unless(exists($opt{d})) {
        print "\n";
    }
    $N_ranges += ($#start_s + 1);
}

if ($N_ranges == 0) {
    print_short_epilogue();
}

unless(exists($opt{b}) || ($N_ranges == 0)) {
    print_epilogue();
}


# Subroutine to use the configuration file data and the exposure data to determine whether or not something is "science"
sub is_science {
#    if (is_science($exp_type,$obs_mode,$object,$comment)) {
    my ($exp_type,$obs_mode,$object,$comment) = @_;
    my $return_value = 0;
    unless($object) {
	$object = '';
    }
    if ($exp_type eq 'OBJECT') {
	foreach my $target (@target_list) {
#	    print STDERR "$target $obsmode_list{$target} $exp_type $obs_mode $object $comment\n";
	    if (($obs_mode =~ /$obsmode_list{$target}/i)&&
		($object   =~ /$object_list{$target}/i)&&
		($comment  =~ /$comment_list{$target}/i)) {
		$return_value = 1;
	    }
#	    print ">$return_value $exp_type $obs_mode $object $comment <> $target $obsmode_list{$target} $object_list{$target} $comment_list{$target}\n";
	    if ($return_value) {
		return($return_value);
	    }
	}
    }
    return(0);
}

##
## These functions print out the remaining text of the pcontrol.pro script.  I've removed things
##  that seemed superfluous.
##

sub print_prologue {

    print << 'END_PROLOGUE';
# this script sets up a series of burntool runs targetted to the appropriate machine

# use the following sql to get the host table match:
# select exp_id, exp_name, class_id, dateobs, user_1, obs_mode, uri from rawImfile where exp_id = 33750 limit 100;

macro burntool
  if ($0 != 3)
    echo "USAGE: burntool (dateobs_begin) (dateobs_end)"
    break
  end

  for i 0 $hostmatch:n
    list word -split $hostmatch:$i
    $class_id = $word:0
    $logfile = "burntool_logs/$class_id.$1.log"
    job -host $word:1 ipp_apply_burntool.pl --class_id $class_id --dateobs_begin $1 --dateobs_end $2 --dbname gpc1 --logfile $logfile
  end
end

macro go
END_PROLOGUE

return(0);
}
sub print_short_epilogue {
    if (exists($opt{b})) {
	print STDERR "There were no science exposures to process. Sorry, try again tomorrow.\n";
	return();
    }
    print << 'END_SHORT_EPILOGUE';

    echo "There were no science exposures to process. Sorry, try again tomorrow."

end
END_SHORT_EPILOGUE
}

sub print_epilogue {
    print " echo 'There were $N_total_images total images, of which $N_burntooled_images were already burntooled.'\n";
    print "### BTSTAT $N_total_images $N_burntooled_images\n";
    print << 'END_EPILOGUE';
    
end

macro setnames
 $burntool_range:20081001 = 2008-10-01T07:50:00 2008-10-01T15:05:00
end

# for a re-run add --skip_burned:
# job -host $word:1 ipp_apply_burntool.pl --class_id $word:0 --dateobs_begin $1 --dateobs_end $2 --dbname gpc1 --skip_burned

macro loadhosts
  for i 0 $allhosts:n
    host add $allhosts:$i
  end
end

# this macro may be used to complete a burntool run that exited due to error
macro burntool_skip_burned
  if ($0 != 3)
    echo "USAGE: burntool (dateobs_begin) (dateobs_end)"
    break
  end

  for i 0 $hostmatch:n
    list word -split $hostmatch:$i
    $class_id = $word:0
    $logfile = "burntool_logs/$class_id.$1.log"
    job -host $word:1 ipp_apply_burntool.pl --class_id $class_id --dateobs_begin $1 --dateobs_end $2 --dbname gpc1 --skip_burned --logfile $logfile
  end
end

list hostmatch
  XY01    ipp014
  XY02    ipp014
  XY03    ipp038
  XY04    ipp038
  XY05    ipp023
  XY06    ipp023
  XY10    ipp039
  XY11    ipp039
  XY12    ipp024
  XY13    ipp024
  XY14    ipp040
  XY15    ipp040
  XY16    ipp026
  XY17    ipp026
  XY20    ipp041
  XY21    ipp041
  XY22    ipp042
  XY23    ipp042
  XY24    ipp043 
  XY25    ipp043
  XY26    ipp028
  XY27    ipp028
  XY30    ipp044
  XY31    ipp044
  XY32    ipp029
  XY33    ipp029
  XY34    ipp045
  XY35    ipp045
  XY36    ipp030
  XY37    ipp030
  XY40    ipp046
  XY41    ipp046
  XY42    ipp031
  XY43    ipp031
  XY44    ipp047
  XY45    ipp047
  XY46    ipp032
  XY47    ipp032
  XY50    ipp048
  XY51    ipp048
  XY52    ipp033
  XY53    ipp033
  XY54    ipp049
  XY55    ipp049
  XY56    ipp034
  XY57    ipp034
  XY60    ipp050
  XY61    ipp050
  XY62    ipp035
  XY63    ipp035
  XY64    ipp051
  XY65    ipp051
  XY66    ipp036
  XY67    ipp036
  XY71    ipp052
  XY72    ipp052
  XY73    ipp015
  XY74    ipp015
  XY75    ipp053
  XY76    ipp053
end
list allhosts
  ipp043
  ipp014
  ipp015
  ipp023
  ipp024
  ipp026
  ipp028
  ipp029
  ipp030
  ipp031
  ipp032
  ipp033
  ipp034
  ipp035
  ipp036
  ipp038
  ipp039
  ipp040
  ipp041
  ipp042
  ipp044
  ipp045
  ipp046
  ipp047
  ipp048
  ipp049
  ipp050
  ipp051
  ipp052
  ipp053
end

END_EPILOGUE
return(0);
}
