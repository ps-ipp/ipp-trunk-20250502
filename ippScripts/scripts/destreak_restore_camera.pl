#!/usr/bin/env perl

use warnings;
use strict;

use Carp;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print STDERR "\n\n";
print STDERR "Starting script $0 on $host at $date\n\n";

use vars qw( $VERSION );
$VERSION = '0.01';

use Digest::MD5::File qw( file_md5_hex );

use IPC::Cmd 0.36 qw( can_run run );
use File::Temp qw( tempfile );
use File::Basename qw( basename dirname );
use File::Copy;
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );

use PS::IPP::Config 1.01 qw( :standard );
use Nebulous::Client;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $magicdstool   = can_run('magicdstool') or (warn "Can't find magicdstool" and $missing_tools = 1);
my $chiptool   = can_run('chiptool') or (warn "Can't find chiptool" and $missing_tools = 1);
my $checkfits   = can_run('checkfits') or (warn "Can't find checkfits" and $missing_tools = 1);
my $ftable   = can_run('ftable') or (warn "Can't find ftable" and $missing_tools = 1);
my $censorObjects   = can_run('censorObjects') or (warn "Can't find censorObjects" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

# Parse the command-line arguments
my ($magic_ds_id, $camera, $streaks, $cam_id, $uri, $path_base);
my ($outroot, $release, $bytes, $md5sum);
my ($dbname, $save_temps, $verbose, $no_update, $no_op, $logfile);

GetOptions(
           'magic_ds_id=s'  => \$magic_ds_id,# Magic destreak run identifier
           'camera=s'       => \$camera,  # path_base of the input
           'cam_id=s'       => \$cam_id, 
           'path_base=s'    => \$path_base,  # path_base of the input
           'dbname=s'       => \$dbname,     # Database name
           'verbose'        => \$verbose,    # Print stuff?
           'no-update'      => \$no_update,  # Don't update the database?
           'no-op'          => \$no_op,      # Don't do any operations?
           'logfile=s'      => \$logfile,
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --magic_ds_id --camera --cam_id --path_base",
           -exitval => 3) unless
    defined $magic_ds_id and
    defined $camera and
    defined $cam_id and
    defined $path_base;

if ($camera ne 'GPC1') {
    my_die("camera must be GPC1 not $camera ",  $magic_ds_id, $PS_EXIT_PROG_ERROR);
}

my $ipprc = PS::IPP::Config->new( $camera ) or my_die( "Unable to set up", $magic_ds_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

$ipprc->redirect_to_logfile($logfile) or my_die( "Unable to redirect output", $magic_ds_id, $PS_EXIT_SYS_ERROR ) if $logfile;

my $nebulousServer = $ENV{NEB_SERVER};
&my_die("cannot find NEB_SERVER in environment", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) if !$nebulousServer;

my $nebulous = eval { Nebulous::Client->new( proxy => $nebulousServer ); };
if ($@ or not defined $nebulous) {
    &my_die ("Unable to create a Nebulous::Client object with proxy $nebulousServer", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);
}

my $mdcParser = PS::IPP::Metadata::Config->new; # Parser for metadata config files

my $dirname  = dirname($path_base);
my $basename = basename($path_base);
my $nebulousInput = inNebulous($dirname);

&my_die ("$dirname is not in nebulous", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) unless $nebulousInput;

my $backup_path_base = $dirname   . "/SR_" . "$basename";
my $recovery_path_base = $dirname . "/REC_" . "$basename";

my $astrom =  $ipprc->filename("PSASTRO.OUTPUT", $path_base);
my $bastrom = $ipprc->filename("PSASTRO.OUTPUT", $backup_path_base);

restore_file($astrom, $bastrom, 1) or
        &my_die("failed to restore smf file", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);

# Now restore the mask files

my @otas = qw(XY01 XY02 XY03 XY04 XY05 XY06 XY10 XY11 XY12 XY13 XY14 XY15 XY16 XY17 XY20 XY21 XY22 XY23 XY24 XY25 XY26 XY27 XY30 XY31 XY32 XY33 XY34 XY35 XY36 XY37 XY40 XY41 XY42 XY43 XY44 XY45 XY46 XY47 XY50 XY51 XY52 XY53 XY54 XY55 XY56 XY57 XY60 XY61 XY62 XY63 XY64 XY65 XY66 XY67 XY71 XY72 XY73 XY74 XY75 XY76);

foreach my $class_id (@otas) {
    my $mask  = "$path_base.$class_id.mk.fits";
    my $bmask = "$backup_path_base.$class_id.mk.fits";
    my $rmask = "$recovery_path_base.$class_id.mk.fits";
    if (!restore_file($mask, $bmask, 0)) {
        # no good mask file. Mark any chips that need this mask file to be useful as bad
        # XXX: what about chipRuns with multiple camRuns can't we preserve those?
        my $command = "$chiptool -dropprocessedimfile -cam_id $cam_id -class_id $class_id -set_quality 45";
        $command   .= " -dbname $dbname" if defined $dbname;
        unless ($no_update) {
            my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                run(command => $command, verbose => $verbose);
            unless ($success) {
                $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
                &my_die("Unable to perform magicdstool -addresult: $error_code", $magic_ds_id, 
                    $error_code);
            }
        } else {
            print "Skipping command: $command\n";
        }
    }
}

{
    # this sets the camRun's magicked value back to zero and deletes the magicDSFile 
    # row from the database
    my $command = "$magicdstool -revertdestreakedfile -i_am_sure";
    $command   .= " -state goto_restored";
    $command   .= " -magic_ds_id $magic_ds_id";
    $command   .= " -component exposure";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform magicdstool -addresult: $error_code", $magic_ds_id, 
                $error_code);
        }
    } else {
        print "Skipping command: $command\n";
    }
}
{
    # this sets the magicDSRun.state to restored
    my $command = "$magicdstool -updaterun";
    $command   .= " -set_state restored";
    $command   .= " -magic_ds_id $magic_ds_id";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform magicdstool -addresult: $error_code", $magic_ds_id, 
                $error_code);
        }
    } else {
        print "Skipping command: $command\n";
    }
}
exit 0;

### Pau.

sub restore_file {
    my $original_url = shift;
    my $backup_url = shift;
    my $smf = shift;

    print "\nStarting restore for $original_url\n";

    # It is the backup that we expect to want to use, check it first.
    my $backup = check_file($backup_url, 1, $smf);

    # Is backup a good file whether censored or not?
    my $backup_ok = ($backup->{so_exists} and $backup->{num_instances});
    
    # Check original, skip full check if backup is ok and uncensored since the backup
    # is the one that we want to keep. 
    my $original = check_file($original_url, (!$backup_ok or $backup->{censored}), $smf);

    my $swap_urls = 0;
    my $kill_censored = 0;
    my $replicate = 0;

    if ($backup_ok and !$backup->{censored}) {
        #
        # Normal case. A good uncensored backup file is available
        #
        if ($backup->{inconsistent}) {
            # At least one of the instances was bad. Repair.
            repair_file($backup, $smf) or
                &my_die( "failed to repair $backup_url", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);
        }
            
        $replicate = $backup->{num_instances} < 2;

        if ($original->{so_exists}) {
            #
            # Normal case. 
            #
            $swap_urls = 1;
            $kill_censored = 1;
        } else {
            # no file at original url rename the backup
            # note: rename_file prints a message and dies on failure
            rename_file($backup_url, $original_url);
        }
    } else {
        # Something is wrong with the backup file, either not ok or censored
        # check the original
        my $original_ok = ($original->{so_exists} and $original->{num_instances});
        if ($original_ok) {
            if ($original->{inconsistent}) {
                repair_file($original, $smf) or
                    &my_die( "failed to repair $original_url", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);
            }
            if (!$original->{censored}) {
                # Note this is where we end up if this file has already been restored
                print "Keeping uncensored file at original url: $original_url\n";
            } else {
                print "No uncensored backup. Keeping censored file at original url: $original_url\n";
            }
            $kill_censored = $backup->{so_exists};
            $replicate = $original->{num_instances} < 2;
        }  else {
            # Original is bad. Is the backup at least a good file. If so swap it back in even if it is censored.
            if ($backup_ok) {
                print "No file found at $original_url\n";
                print "Swapping in      $backup_url\n";
                if ($backup->{inconsistent}) {
                    repair_file($backup, $smf) or 
                        &my_die( "failed to repair $backup_url", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);
                }
                $replicate = $backup->{num_instances} < 2;
                if ($original->{so_exists}) {
                    $swap_urls = 1;
                    $kill_censored = 1;
                } else {
                    # note: rename_file prints a message and dies on failure
                    rename_file($backup_url, $original_url);
                }
            } else {
                # we're out of luck
                print "No good file for $original_url found\n";
                return 0;
            }
        }
    }

    # Ok Now we know what we need to do so do it

    if (!$no_op) {
        if ($swap_urls) {
            if (! $nebulous->swap($backup_url, $original_url) ) {
                print "failed to swap $backup_url\n";
                print "            to $original_url\n";
                &my_die( "aborting", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);
            } else {
                print "restored $original_url\n";
            }
        }
        if ($kill_censored) {
            # destroy the backup file
            $ipprc->kill_file($backup_url);
        }

        if ($replicate) {
            $ipprc->replicate_file($original_url) 
                or &my_die( "failed to replicate $original_url", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);
        }
    } else {
        print "Skipping:\n";
        print "  Swap urls $backup_url $original_url\n" if $swap_urls;
        print "  Delete $backup_url\n" if $swap_urls;
        print "  Replicate $original_url\n" if $replicate;
    }

    return 1;
}

sub check_file {
    my $url = shift;
    my $full_check = shift;
    my $smf = shift;

    my %file;
    $file{url} = $url;
    $file{so_exists} = $nebulous->storage_object_exists($url);
    $file{total_instances} = 0;
    $file{num_instances} = 0;
    $file{censored} = 0;
    $file{inconsistent} = 0;
    $file{instances} = [];
    $file{bad_instances} = [];
    $file{size} = 0;
    $file{md5} = 0;

    if ($file{so_exists}) {
        my $stat = $nebulous->stat($url);

        &my_die("nebulous stat failed for $url", $magic_ds_id, $PS_EXIT_CONFIG_ERROR) unless $stat;

        $file{total_instances} = $stat->[7];

        if ($file{total_instances}) {
            my $instances;
            eval {
                $instances = $nebulous->find_instances($url, 'any');
            };
            if (!$instances or !scalar @$instances) {
                print "No instances found for $url\n";
                return \%file;
            }

            my $good_inst;
            my $good_size = 0;
            my $good_md5 = "";
            my $good_censored ;
            foreach my $inst (@$instances) {
                $inst =~ s/file\:\/\///g;

                if ( ! -e $inst) {
                    print "instance file $inst does not exist\n";
                    push @{$file{bad_instances}}, $inst;
                    next;
                }
                my $this_size = -s $inst;
                if (!$this_size) {
                    print "instance file $inst has zero size\n";
                    push @{$file{bad_instances}}, $inst;
                    next;
                }

                my $this_censored = ($inst =~ /SR_/);

                my $this_md5 = "";
                if ($full_check) {
                    $this_md5 = file_md5_hex($inst);
                    if ($smf) {
                        my $command = "$ftable -list  $inst | grep psf | wc";
                        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                                run(command => $command, verbose => $verbose);

                        my $good = 0;
                        if ($success) {
                            my ($nchips) = split " ", join "", @$stdout_buf;
                            $nchips += 0;
                            if ($nchips and $nchips == 60) {
                                $good = 1;
                            } else {
                                print "$inst has less than 60 psf extensions: $nchips checking with censorObjects\n";
                                my $command = "$censorObjects -checkinputonly -file $inst";
                                my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                                    run(command => $command, verbose => $verbose);

                                if ($success) {
                                    $good = 1;
                                } else {
                                    print "$command failed with error_code: $error_code\n", 
                                }
                            }
                        } else {
                            print "$command failed with error_code: $error_code\n", 
                        } 
                        if (!$good) {
                            push @{$file{bad_instances}}, $inst;
                            next;
                        }
                    } else {
                        my $command = "$checkfits $inst";
                        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
                                run(command => $command, verbose => $verbose);
                        unless ($success) {
                            print "$command failed with error_code: $error_code\n", 
                            push @{$file{bad_instances}}, $inst;
                            next;
                        }
                    }
                }
                if (!defined $good_inst) {
                    $good_inst = $inst;
                    $good_size = $this_size;
                    $good_md5 = $this_md5;
                    $good_censored = $this_censored;

                    push @{$file{instances}}, $inst;

                    # unless we're asked for a full all really care about is the censored status
                    last if !$full_check;
                } elsif ($this_size != $good_size) {
                    print "size of $inst $this_size is not equal to size of $good_inst: $good_size\n";
                    $file{inconsistent} = 1;
                    push @{$file{bad_instances}}, $inst;
                } elsif ($this_md5 ne $good_md5) {
                    print "md5 of $inst $this_md5 is not equal to size of $good_inst: $good_md5\n";
                    $file{inconsistent} = 1;
                    push @{$file{bad_instances}}, $inst;
                } else {
                    # this matches the good instance
                    push @{$file{instances}}, $inst;
                }
            }
            $file{num_instances} = scalar @{$file{instances}};
            $file{size} = $good_size;
            $file{md5} = $good_md5;
            if ($good_inst) {
                $file{censored} = $good_censored;
            } else {
                # no good instances see if there was a bad one and check whether its url
                # looks like a censored one
                if (scalar @{$file{bad_instances}}) {
                    my $bad = ${$file{bad_instances}}[0];
                    $file{censored} = ($bad =~ /SR_/);
                    print "no good instances found setting censored based on $bad\n";
                }
            }
        }
    } else {
        print "No storage object found for $url \n";
    }

    return \%file;
}

sub rename_file {
    if ($no_op) {
        return 1;
    }

    my $backup_url = shift;
    my $original_url = shift;

    # rename the backup as the orig
    print "No storage object for original url: $original_url\n";
    print "Replacing with file at            : $backup_url\n";

    my $moved;
    eval {
        $moved = $nebulous->move($backup_url, $original_url);
    };
    if ($@ or not $moved) {
        print "Failed to move $backup_url to $original_url\n";
        &my_die( "aborting", $magic_ds_id, $PS_EXIT_CONFIG_ERROR);
    }
}

sub repair_file {
    if ($no_op) {
        return 1;
    }

    my $file = shift;
    my $smf = shift;

    if ($file->{num_instances} < 1) {
        print "cannot repair file with only no good instances\n";
        return 0;
    }

    # file wouldn't get into instances array unless we decided that it was good in check_file
    my $good_inst = $file->{instances}->[0];

    my $fixed = 0;
    foreach my $bad_inst (@{$file->{bad_instances}}) {
        print "Attempting to repair $bad_inst\n";

        my $parent_directory = dirname($bad_inst);
        if (!-d $parent_directory) {
            if (! mkdir $parent_directory) {
                print "failed to mkdir parent directory $parent_directory\n";
                $fixed = 0;
                last;
            }
            if (!copy $good_inst, $bad_inst) {
                print "failed to copy $good_inst to $bad_inst\n";
            }
            $fixed = 1;
            push @{$file->{instances}}, $bad_inst;
        }
    }
        
    return $fixed;
}



sub delete_recovery_files
{
    foreach my $file (@_) {
        next if !$file;
	# don't care if this fails or not (it will fail if storage
	# object doesn't exist
        $ipprc->kill_file($file);
    }
}

sub inNebulous
{
    my $path = shift;

    my $scheme = file_scheme($path);

    return $scheme and ($scheme eq "neb");
}


sub my_die
{
    my $msg = shift;            # Warning message on die
    my $magic_ds_id = shift;    # Magic DS identifier
    my $exit_code = shift;      # Exit code to add

    carp($msg);

    $exit_code = $PS_EXIT_PROG_ERROR unless defined $exit_code;

    # fault the whole run if one of the components fails to revert
    my $command = "$magicdstool -updaterun";
    $command   .= " -magic_ds_id $magic_ds_id";
    $command   .= " -set_state failed_restore";
    $command   .= " -dbname $dbname" if defined $dbname;

    # Add the processed file to the database
    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            carp("failed to update database for $magic_ds_id");
        }
    } else {
        print "Skipping command: $command\n";
    }

    exit $exit_code;
}

__END__
