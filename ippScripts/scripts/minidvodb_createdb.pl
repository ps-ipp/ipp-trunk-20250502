#!/usr/bin/env perl

use warnings;
use strict;
use Carp;
use DateTime;

use DateTime::Format::Strptime;
use DateTime::Duration;
## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";


my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile );

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $addtool = can_run('addtool') or (warn "Can't find addtool" and $missing_tools = 1);
my $ppConfigDump = can_run('ppConfigDump') or (warn "Can't find ppConfigDump" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}
my (  $outroot, $dbname, $dvodb, $minidvodb,$minidvodb_interval, $minidvodb_nums, $minidvodb_group, $minidvodb_host, $camera,  $verbose, $no_update,
     $no_op, $redirect, $save_temps);
GetOptions(
    'camera|c=s'        => \$camera, # Camera
    'dbname|d=s'        => \$dbname, # Database name
    'minidvodb_group|w=s'       => \$minidvodb_group, # minidvodb_group.
    'outroot|w=s'       => \$outroot, # output file base name
    'dvodb|w=s'         => \$dvodb,  # output DVO database
    'minidvodb|w=s'     => \$minidvodb, # output miniDVODB
    'interval|w=s'      => \$minidvodb_interval, #interval between creation of minidvodbs (default = 1day)
    'num|w=s'      => \$minidvodb_nums, #interval between creation of minidvodbs (default = 500 addRuns)
    'minidvodb_host|w=s'  => \$minidvodb_host, #assign a hostname for addstar
    'verbose'           => \$verbose,   # Print to stdout
    'no-update'         => \$no_update, # Update the database?
    'no-op'             => \$no_op, # Don't do any operations?
    'redirect-output'   => \$redirect,
    'save-temps'        => \$save_temps, # Save temporary files?
    ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage(
          -msg => "Required options: --camera  --dvodb --minidvodb_group  --outroot",
          -exitval => 3,
          ) unless
    defined $minidvodb_group and
    defined $camera and
    defined $outroot and
    defined $dvodb;

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $minidvodb_group   , $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $logDest = $ipprc->filename("LOG.EXP", $outroot) or &my_die("Missing entry from camera config", $minidvodb_group, $PS_EXIT_CONFIG_ERROR);

if ($redirect) {
    $ipprc->redirect_output($logDest) or my_die( "Unable to redirect output", $minidvodb_group, $PS_EXIT_SYS_ERROR );
    print "\n\n";
    print "Starting script $0 on $host\n\n";
    print "COMMAND IS: @ARGV\n\n";
}


# Recipes to use based on reduction class

# XXX This is now not used: do we still need it?

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

# Output products
$ipprc->outroot_prepare($outroot);

# the camera configurations should define the psastro output to be a single file (MEF), regardless of the inputs
 my $create_new = 0;
# convert supplied DVO database name to UNIX filename
my $dvodbReal;
if (defined $dvodb) {
    $dvodbReal = $ipprc->dvo_catdir( $dvodb ); # catdir for DVO
    $dvodbReal = $ipprc->convert_filename_absolute( $dvodbReal );
} else {
    warn("dvodb undefined:\n");
    exit(4);
}
my $minidvodbReal;
if (defined $minidvodb) {
    $minidvodbReal = $ipprc->dvo_catdir( $minidvodb ); # catdir for DVO
    $minidvodbReal = $ipprc->convert_filename_absolute( $minidvodbReal );
} else {
    warn("minidvodb undefined:\n");
    exit(4);
}


if (!defined $minidvodb_interval) {
    $minidvodb_interval = 1;
}
if (!defined $minidvodb_nums) {
    $minidvodb_nums = 500
}


unless ($no_op) {



#see if there is already one in new state
    my $fpaCommand1 = "$addtool -listminidvodbrun";
    $fpaCommand1 .= " -minidvodb_group '$minidvodb_group'";
    $fpaCommand1 .= " -state 'new'";
    $fpaCommand1 .= " -dbname $dbname" if defined $dbname;


unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $fpaCommand1, verbose => $verbose);



   unless ($success) {
       $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
       warn("Unable to list minidvodb database: $error_code\n");
       exit($error_code);
    }

    if (scalar(@{$stdout_buf})) {
        $error_code = 3;
            warn("Unwilling to create minidvodb, one already exists in new state: $error_code\n");
        exit($error_code);
    }

    my $creation_date;
    my $addRun_count;
    my $minidvodb_name;
#find the active one's date, and find out if it has more than 1 addRun in it

    my $fpaCommand2 = "$addtool -listminidvodbrun";
    $fpaCommand2 .= " -minidvodb_group '$minidvodb_group'";
    $fpaCommand2 .= " -state 'active'";
    $fpaCommand2 .= " -limit 1";
     $fpaCommand2 .= " -dbname $dbname" if defined $dbname;

#print $fpaCommand2;


    my ( $success2, $error_code2, $full_buf2, $stdout_buf2, $stderr_buf2 ) =
        run(command => $fpaCommand2, verbose => $verbose);
    &my_die( "Unable to get listminidvodbrun",$minidvodb_group, $PS_EXIT_SYS_ERROR) unless $success2;
  # if it didn't list something in active state (what?) then we definitely need to create a new one
    if ((@$stdout_buf2)) {
  my  $metadata2 = $mdcParser->parse(join "", @$stdout_buf2) or
        &my_die("Unable to parse metadata config", $minidvodb_group, $PS_EXIT_PROG_ERROR);

  my   $components2 = parse_md_list($metadata2) or
        &my_die("Unable to parse metadata list", $minidvodb_group, $PS_EXIT_PROG_ERROR);
  my   $comp2 = $$components2[0];
    $minidvodb_name = $comp2->{minidvodb_name};
    $creation_date  = $comp2->{creation_date};
    if (!defined($minidvodb_name)) {
        &my_die("Unable to parse minidvodb_name", $minidvodb_group, $PS_EXIT_PROG_ERROR);
    }
    if (!defined($creation_date)) {
        &my_die("Unable to parse creation_date", $minidvodb_group, $PS_EXIT_PROG_ERROR);
    }
    } else {
        $create_new = 1; #this is to force it to make a new one
            }

    #find the number of add_ids that have been proccessed
    my $fpaCommand3 = "$addtool -checkminidvodbrunaddrun";
    $fpaCommand3 .= " -minidvodb_group '$minidvodb_group'";
    $fpaCommand3 .= " -state 'active'";
    $fpaCommand3 .= " -minidvodb_name '$minidvodb_name'" if defined $minidvodb_name;
    $fpaCommand3 .= " -limit 1";
    $fpaCommand3 .= " -dbname $dbname" if defined $dbname;


my ( $success3, $error_code3, $full_buf3, $stdout_buf3, $stderr_buf3 ) =
        run(command => $fpaCommand3, verbose => $verbose);
    &my_die( "Unable to get checkminidvodbunaddrun", $minidvodb_group, $PS_EXIT_SYS_ERROR) unless $success3;

    if ((@$stdout_buf3)) {  #checkminidvodb returns nothing IF there have been no addruns added to the db yet
    my  $metadata3 = $mdcParser->parse(join "", @$stdout_buf3) or
        &my_die("Unable to parse metadata config", $minidvodb_group, $PS_EXIT_PROG_ERROR);

    my  $components3 = parse_md_list($metadata3) or
        &my_die("Unable to parse metadata list", $minidvodb_group, $PS_EXIT_PROG_ERROR);
   my  $comp = $$components3[0];
    $addRun_count = $comp->{addRun_count};
        }
    if (!defined($addRun_count)) {
         ## there's nothing to parse if there's nothing
        $addRun_count = 0;
    }


    if ($addRun_count > $minidvodb_nums) {
        #it's too big, create_new
        $create_new = 1;
   }
    if ($create_new == 0) {
        my $parser = DateTime::Format::Strptime->new( pattern => '%Y-%m-%dT%H:%M:%S', time_zone => "HST" );
        my $creation_dt = $parser->parse_datetime( $creation_date )->mjd;
        if ($mjd_start- $creation_dt > $minidvodb_interval && $addRun_count > 0 ) {
            #db is old and has stuff in it, want to create_new
            $create_new = 1;
        }
    }


}
#create the minidvodb entry (well, the command for it)
    my $fpaCommand = "$addtool -addminidvodbrun";
    $fpaCommand .= " -set_minidvodb_group $minidvodb_group";
    $fpaCommand .= " -set_minidvodb_host $minidvodb_host";
    $fpaCommand .= " -set_minidvodb_path  $minidvodbReal" if defined $minidvodbReal;
    $fpaCommand .= " -dbname $dbname" if defined $dbname;

    unless ($no_update or !$create_new) {


        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $fpaCommand, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            warn("Unable to add result to database: $error_code\n");
            exit($error_code);
        }
    } else {
        print "skipping command: $fpaCommand\n";
    }
}

sub my_die
{#complain if it doesn't work
    my $msg = shift; # Warning message on die
    my $minidvodb_group = shift; # Camtool identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    carp($msg);

    exit $exit_code;
}

END {
    my $status = $?;
    system("sync") == 0
        or die "failed to execute sync: $!" ;
    $? = $status;
}

__END__
