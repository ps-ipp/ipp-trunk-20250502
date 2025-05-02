#!/usr/bin/env perl

use warnings;
use strict;

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Config 1.01 qw( :standard );
use PS::IPP::Metadata::Config;
use Sys::Hostname;

use Digest::MD5::File qw( file_md5_hex );
use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

## report the program and machine
my $host = hostname();
print "\n\n";
#print "Starting script $0 on $host\n\n";
my $date = `date`;
print "Starting script $0 on $host at $date";


# Parse the command-line arguments
my ( $uri, $filename, $compress, $bytes, $md5, $nebulous, $summit_id, $exp_name, $inst, $telescope, $class, $class_id, 
     $dbname, $verbose, $no_update, $no_op, $timeout, $copies );
GetOptions(
       'uri=s'          => \$uri,       # source location of file on data store
       'filename=s'     => \$filename, # target location of file on local system
       'compress'       => \$compress,  # request file in compressed format
       'bytes=s'        => \$bytes,     # reported file size in bytes
       'md5=s'          => \$md5,       # reported md5 checksum
       'nebulous'       => \$nebulous,  # use nebulous for the target file
       'summit_id=s'    => \$summit_id, # summit_id for this exposure
       'exp_name=s'     => \$exp_name,  # Exposure name
       'inst=s'         => \$inst,      # Instrument
       'telescope=s'    => \$telescope, # Telescope
       'class=s'        => \$class,     # Class level
       'class_id=s'     => \$class_id,  # Class identifier
       'dbname=s'       => \$dbname,    # Database name
       'verbose'        => \$verbose,   # Print to stdout
       'no-update'      => \$no_update, # Don't update the database?
       'no-op'          => \$no_op,     # Don't do any operations?
       'timeout=s'      => \$timeout,   # passed through to dsget
       'copies=i'       => \$copies,    # passed through to dsget
) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --uri --filename --summit_id --exp_name --inst --telescope --class --class_id",
       -exitval => 3)
    unless defined $uri
    and defined $filename
    and defined $summit_id
    and defined $exp_name
    and defined $inst
    and defined $telescope
    and defined $class
    and defined $class_id;

# Look for programs we need
my $missing_tools;
my $dsget = can_run('dsget')
    or (warn "Can't find dsget" and $missing_tools = 1);
my $pztool = can_run('pztool')
    or (warn "Can't find pztool" and $missing_tools = 1);
my $neblocate = can_run('neb-locate')
    or (warn "Can't find neb-locate" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $ipprc = PS::IPP::Config->new();

# dsget command
my $command;
$command  = "$dsget --uri $uri --filename $filename";
$command .= " --compress"           if defined $compress;
$command .= " --bytes $bytes"       if defined $bytes;
$command .= " --nebulous"           if defined $nebulous;
$command .= " --md5 $md5"           if defined $md5;
$command .= " --timeout $timeout"   if defined $timeout;
$command .= " --copies $copies"     if defined $copies;

# run command
unless ($no_op) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf )
        = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        my_die("Unable to perform dsget: $error_code",
            $exp_name,
            $inst,
            $telescope,
            $class,
            $class_id,
            $uri,
            $error_code
        );
    }
} else {
    print "skipping command: $command\n";
}

#
# Find all instances of the new file and make sure they have the
# same checksum and size.
# if so pass the results to pztool to the results in pzDownloadImfile.
# uncomment this to turn on
my ($new_bytes, $new_md5) = check_instances($filename, $nebulous, $compress);

$date = `date`;
print "starting pztool instances checksum  $date";

# command to update database
$command  = "$pztool -copydone";
$command .= " -row_lock";
$command .= " -summit_id $summit_id";
$command .= " -exp_name $exp_name";
$command .= " -inst $inst";
$command .= " -telescope $telescope";
$command .= " -class $class";
$command .= " -class_id $class_id";
$command .= " -uri $filename";
$command .= " -hostname $host";
$command .= " -dbname $dbname" if defined $dbname;

# XXX: TODO: see above. Don't do this until pztool and the DB have
# been updated
$command .= " -md5sum $new_md5 -bytes $new_bytes";

# update the database
unless ($no_update) {
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf )
        = run(command => $command, verbose => $verbose);
    unless ($success) {
        $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
        warn("Unable to perform $command: $error_code\n");
        exit($error_code);
    }
} else {
    print "skipping command: $command\n";
}

$date = `date`;
print "finished  $date\n";


sub get_file_params
{
    my $filename = shift;

    my $size = -s $filename;
    my $md5 = file_md5_hex($filename);

    return ($size, $md5);
}

sub check_instances {
    my $filename = shift;
    my $nebulous = shift;
    my $compress = shift;

    my @instances;
    if ($nebulous) {
        my $command = "$neblocate --path --all $filename";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf )
            = run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            my_die("Unable to perform neb-locate: $error_code",
                $exp_name,
                $inst,
                $telescope,
                $class,
                $class_id,
                $uri,
                $error_code
            );
        }
        @instances = split "\n", join "", @$stdout_buf;
    } else {
        if (!defined $ipprc) {
            # we don't usually need this so defer instantaiating it
            $ipprc = PS::IPP::Config->new();
        }
        my $resolved = $ipprc->file_resolve($filename);
        if ($resolved) {
            $instances[0] = $resolved;
        }
    }
    if (! scalar @instances) {
        my_die("no instances",
                    $exp_name,
                    $inst,
                    $telescope,
                    $class,
                    $class_id,
                    $uri,
                    $PS_EXIT_UNKNOWN_ERROR
            );
    }

    my ($new_bytes, $new_md5) = get_file_params($instances[0]);
        
    for (my $i = 1; $i < scalar @instances; $i++) {
        my ($b, $m) = get_file_params($instances[$i]);
        my $error = "";
        if ($b ne $new_bytes) {
            $error = "size of $instances[$i] does not match $instances[0]";
        } elsif ($m ne $new_md5) {
            $error = "md5sum of $instances[$i] does not match $instances[0]";
        }
        if ($error) {
            my_die($error,
                    $exp_name,
                    $inst,
                    $telescope,
                    $class,
                    $class_id,
                    $uri,
                    $PS_EXIT_DATA_ERROR
            );
        }
    }

    return ($new_bytes, $new_md5);
    
}

sub my_die
{
    my $msg       = shift; # Warning message on die
    my $exp_name  = shift; # Chiptool identifier
    my $inst      = shift; # Chiptool identifier
    my $telescope = shift; # Class identifier
    my $class     = shift; # Class identifier
    my $class_id  = shift; # Class identifier
    my $uri       = shift; # Class identifier
    my $exit_code = shift; # Exit code to add

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    warn $msg;
    unless ($no_update) {
        # command to update database
        my $command;
        $command  = "$pztool -copydone";
	$command .= " -summit_id $summit_id";
        $command .= " -exp_name $exp_name";
        $command .= " -inst $inst";
        $command .= " -telescope $telescope";
        $command .= " -class $class";
        $command .= " -class_id $class_id";
        $command .= " -uri $uri";
        $command .= " -fault $exit_code";
        $command .= " -dbname $dbname" if defined $dbname;

        system ($command);

	if ($exit_code == 110) {
	    $command = "$pztool -updatepzexp";
	    $command .= " -summit_id $summit_id";
	    $command .= " -exp_name $exp_name";
	    $command .= " -inst $inst";
	    $command .= " -telescope $telescope";
	    $command .= " -set_state drop";
	    $command .= " -dbname $dbname" if defined $dbname;

	    system ($command);
	}

    }

    exit $exit_code;
}

# XXX: I don't think that we need this - JH
#END {
#    my $status = $?;
#    system("sync") == 0
#        or die "failed to execute sync: $!" ;
#    $? = $status;
#}

__END__
