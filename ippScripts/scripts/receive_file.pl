#!/usr/bin/env perl

use warnings;
use strict;

## report the program and machine
use Sys::Hostname;
my $host = hostname();
my $date = `date`;
print "\n\n";
print "Starting script $0 on $host at $date\n\n";

use DateTime;
my $mjd_start = DateTime->now->mjd;   # MJD of starting script

use vars qw( $VERSION );
$VERSION = '0.01';

use IPC::Cmd 0.36 qw( can_run run );
use PS::IPP::Metadata::Config;
use PS::IPP::Metadata::List qw( parse_md_list );
use PS::IPP::Config 1.01 qw( :standard );
use File::Temp qw( tempfile tempdir );
use File::Basename qw( basename );
use Carp;

use Getopt::Long qw( GetOptions :config auto_help auto_version gnu_getopt );
use Pod::Usage qw( pod2usage );

# Look for programs we need
my $missing_tools;
my $receivetool = can_run('receivetool') or (warn "Can't find receivetool" and $missing_tools = 1);
my $dsproductls = can_run('dsfilesetls') or (warn "Can't find dsfilesetls" and $missing_tools = 1);
my $gunzip = can_run('gunzip') or (warn "Can't find gunzip" and $missing_tools = 1);
if ($missing_tools) {
    warn("Can't find required tools.");
    exit($PS_EXIT_CONFIG_ERROR);
}

my $temproot = "/tmp";

# Parse the command-line arguments
my ( $file_id, $source, $product, $fileset, $fileset_id, $file, $component, $bytes, $md5sum, $workdir, $dirinfo_uri, $no_extract, $dbname, $verbose, $no_update, $save_temps );

GetOptions(
           'file_id=s'         => \$file_id, # File identifier
           'source=s'          => \$source, # Source for data
           'product=s'         => \$product, # Product for data
           'fileset=s'         => \$fileset, # Fileset for data
           'fileset_id=s'      => \$fileset_id, # database id for the fileset
           'file=s'            => \$file, # File to retrieve
           'component=s'       => \$component, # component for this file (class_id, skycell_id or dbinfo)
           'bytes=i'           => \$bytes, # file size in bytes
           'md5sum=s'          => \$md5sum, # md5sum for file from data store
           'workdir=s'         => \$workdir, # Working directory for output
           'no-extract'        => \$no_extract, # Do not extract the tarfiles
           'dirinfo=s'    => \$dirinfo_uri, # file containing the destination directories for this component
           'dbname=s'          => \$dbname,    # Database name
           'verbose'           => \$verbose,   # Print to stdout
           'no-update'         => \$no_update, # Don't update the database?
           'save-temps'        => \$save_temps, # Save temporary files?
           ) or pod2usage( 2 );

pod2usage( -msg => "Unknown option: @ARGV", -exitval => 2 ) if @ARGV;
pod2usage( -msg => "Required options: --file_id --source --product --fileset --file --component --workdir --bytes --md5sum --dirinfo",
           -exitval => $PS_EXIT_CONFIG_ERROR) unless
    defined $file_id and
    defined $source and
    defined $product and
    defined $fileset and
    defined $file and
    defined $component and
    defined $bytes and
    defined $md5sum and
    defined $dirinfo_uri and
    defined $workdir;

my $tempdir = tempdir( "$temproot/receive.$file_id.XXXX", CLEANUP => !$save_temps);

&my_die( "dirinfo is NULL for $component", $file_id, $PS_EXIT_CONFIG_ERROR )
    if (($dirinfo_uri eq "NULL") and ($component ne "dirinfo"));

my $ipprc = PS::IPP::Config->new() or
    &my_die( "Unable to set up", $file_id, $PS_EXIT_CONFIG_ERROR ); # IPP configuration

my $mdcParser = PS::IPP::Metadata::Config->new;

# select a directory for the dirinfo and dbinfo files
# XXX: perhaps this directory should be set by the script and passed in
# rather than computed here.

my ($day, $month, $year) = (localtime)[3,4,5];
my $datestr = sprintf "%04d%02d%02d", $year+1900, $month + 1, $day;
my $dir_for_info_files = caturi($workdir, $datestr, $fileset);


my $is_tarfile = 0;
if ($file =~ m|.*\.tgz$|) {        # XXX: perhaps get this off of file type ?
    $is_tarfile = 1;
}

my $filename;
if ($is_tarfile and $no_extract) {
    $filename = "$dir_for_info_files/$file"; 
} else {
    $filename = "$tempdir/$file";
}

# Retrieve file
{
    my $uri = "$source/$product/$fileset/$file"; # URI for datastore file
    my $command = "dsget --uri $uri --filename $filename"; # Command to execute
    $command .= " --timeout 590";
    $command .= " --bytes $bytes --md5 $md5sum";

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die( "Unable to retrieve file from $uri\n", $file_id, $PS_EXIT_DATA_ERROR) unless $success;
}
my $mjd_copy = DateTime->now->mjd;   # MJD of finishing copy

# figure out which dirinfo file to read
my $dirinfo_file_to_read = $component eq "dirinfo" ? $filename : $dirinfo_uri;

# process it
my ($destdir, $components, $dirinfo_lines) = read_dirinfo_file($dirinfo_file_to_read, $file_id);


# Deal with file
if ($component eq 'dirinfo') {
    # save the dirinfo file contents into the $workdir

    $dirinfo_uri = caturi($dir_for_info_files, basename($filename));
    print "dirinfo_uri: $dirinfo_uri\n" if $verbose;

    my $resolved = $ipprc->file_resolve($dirinfo_uri, 'create');
    &my_die( "failed to resolve $dirinfo_uri\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$resolved;

    print "dirinfo resolved is: $resolved\n" if $verbose;

    open OUT, ">$resolved"
        or &my_die( "failed to open $resolved\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);
    print OUT @$dirinfo_lines
        or &my_die( "failed to write $resolved\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);
    close OUT
        or &my_die( "failed to close $resolved\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    # update the fileset to allow processing of other files
    my $command = "$receivetool -updatefileset -fileset_id $fileset_id";
    $command .= " -set_state new -dirinfo $dirinfo_uri";
    $command .= " -dbname $dbname" if defined $dbname;

    unless ($no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die( "Unable to update fileset $fileset_id to\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) unless $success;
    } else {
        print STDERR "skipping $command\n";
    }

} elsif ($component eq "dbinfo") {

    open INFILE, $filename or &my_die( "Can't open $filename\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    my @lines = (<INFILE>);

    my $dbinfo = join "", @lines;

    close INFILE;

    my $dbinfo_uri = caturi($dir_for_info_files, basename($filename));
    print "dbinfo_uri: $dbinfo_uri\n" if $verbose;

    my $resolved = $ipprc->file_resolve($dbinfo_uri, 'create');
    &my_die( "failed to resolve $dbinfo_uri\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$resolved;

    open OUT, ">$resolved"
        or &my_die( "failed to open $resolved\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    # We process the dbinfo file (the exported run from the distribution) line by line
    # Rather than read it as an mdc and interptet it, we do our substitutions directly
    # This is much faster. Parsing a mdc file for a chip run takes several seconds.
    # we are very strict about the format of the file
    #
    # First comes the dbversion metadata
    # Next comes the data for the Run
    # Next is the data for each component and other associated tables
    # The component_id (class_id, skycell_id) must come before any of the paths that we edit

    my $runType = findRunType(\@lines);

    my $stage;
    my $comp_name;
    my $current_component;
    if ($runType eq 'rawExp') {
        $stage = 'raw';
        $comp_name = 'class_id';
    } elsif ($runType eq 'chipBackgroundRun') {
        $stage = 'chip_bg';
        $comp_name = 'class_id';
    } elsif ($runType eq 'chipRun') {
        $stage = 'chip';
        $comp_name = 'class_id';
    } elsif ($runType eq 'camRun') {
        $stage = 'camera';
        $comp_name = 'exposure';
        $current_component = $comp_name;
    } elsif ($runType eq 'fakeRun') {
        $stage = 'fake';
        $comp_name = 'class_id';
    } elsif ($runType eq 'warpRun') {
        $stage = 'warp';
        $comp_name = 'skycell_id';
    } elsif ($runType eq 'warpBackgroundRun') {
        $stage = 'warp_bg';
        $comp_name = 'skycell_id';
    } elsif ($runType eq 'diffRun') {
        $stage = 'diff';
        $comp_name = 'skycell_id';
    } elsif ($runType eq 'stackRun') {
        $stage = 'stack';
        $comp_name = 'skycell_id';
    } elsif ($runType eq 'staticskyRun') {
        $stage = 'sky';
        $comp_name = 'skycell_id';
    } elsif ($runType eq 'skycalRun') {
        $stage = 'skycal';
        $comp_name = 'skycell_id';
    } else {
        &my_die( "unexpected run type line found in $filename: $runType\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);
    }

    my $new_workdir_value;
    if ($destdir eq 'none') {
        # this only appiles to rawExp
        $new_workdir_value = "$workdir";
    } else {
        $new_workdir_value = "$workdir/$destdir";
    }

    if ($stage eq 'sky' or $stage eq 'skycal') {
        # the dbinfo file for staticskyRun and skycalRun only have one component and they don't contain
        # skycell_id which is the way components are listed in the dirinfo file.
        my @ids = keys %$components;
        my $nComponents = scalar @ids;
        &my_die( "unexpected number of components $nComponents found in $stage dirinfo file\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) if $nComponents  != 1;
        $current_component = $ids[0];
    }

    my $component_dir;
    if ($current_component) {
        $component_dir = $components->{$current_component};
    }
    foreach my $line (@lines) {
        my $out_line = $line;

        my ($name, $type, $value) = split " ", $line;
        # only complete lines have things that we need to examine
        if ($name and $type and $value) {
            my $new_value;
            # we have a new component id, save it and look up the corresponding
            # component_dir
            if ($name eq $comp_name) {
                $current_component = $value;
                $component_dir = $components->{$current_component};
                &my_die( "$component_dir is null for $value in $filename: $runType\n",
                        $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$component_dir;
            } elsif ($name eq 'workdir') {
                $new_value = $new_workdir_value;
            } elsif ($name eq 'tess_id') {
                # for tess_id strip off any directories just keep the basename.
                # The site configuration will need to map this to a proper location
                # XXX: Document this
                $new_value = basename($value);
            } elsif ((($name eq 'uri') or ($name eq 'path_base')) and ($value ne 'NULL')) {
                &my_die( "component_dir is null and we need it for $name",
                        $file_id, $PS_EXIT_PROG_ERROR) if !$component_dir;

                $new_value = caturi($new_workdir_value, $component_dir, basename($value));
            }

            # if the value changed re-write the line, otherwise just print what we read
            if ($new_value) {
                $out_line = "   " . $name . "\t\t" . $type . "\t" . $new_value . "\n";
            }
        }

        print OUT $out_line or &my_die( "failed to write to $resolved\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);
    }

    close OUT
        or &my_die( "failed to close $resolved\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    # update the fileset to allow processing of other files
    my $command = "$receivetool -updatefileset -fileset_id $fileset_id";
    $command .= " -dbinfo $dbinfo_uri";
    $command .= " -dbname $dbname" if defined $dbname;

    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    &my_die( "Unable to update fileset $fileset_id to\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) unless $success;


} elsif ($is_tarfile and !$no_extract) {
    # Get contents of tarball
    my @files = ();
    {
        my $command = "tar -C $tempdir -tzf $filename"; # Command to execute
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die( "Unable to get listing of tar file $filename\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) unless $success;

        my @lines = split(/\n/, join "", @$stdout_buf); # Lines from output
        foreach my $line ( @lines ) {
            $line =~ s/\#.*$//;
            next unless $line =~ /\S+/;
            my @fields = split(/\s+/, $line); # Fields in line
            my $file = $fields[0];      # Name of file
            $file =~ s|^./||;
            push @files, $file if $file =~ /\S+/;
        }
    }

    # Extract files from tarball
    {
        my $command = "tar -C $tempdir -xzf $filename"; # Command to execute
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        &my_die( "Unable to extract tar file $filename\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) unless $success;
    }

    my $component_dir = $components->{$component};
    &my_die( "Unable to find component_dir for $component $filename\n", $file_id, $PS_EXIT_UNKNOWN_ERROR) unless $component_dir;

    my $target_dir;
    if ($destdir eq 'none') {
        $target_dir = "$workdir";
    } else {
        $target_dir = "$workdir/$destdir";
    }
    $target_dir .= "/$component_dir";

    # Move files into filesystem of choice
    foreach my $file ( @files ) {
        my $from = "$tempdir/$file"; # Source for file
        my $target = "$target_dir/$file"; # Target destination for file


        $ipprc->file_delete ($target);

        my $to = $ipprc->file_create( $target ); # Target for move

        if (!$to) {
            &my_die( "failed to create: $target\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);
        }

        if ( $file =~ /.+\.mdc/ ) {
            # this file is a config dump file edit the paths
            edit_mdc_file($file_id, $from, $to, $workdir);
        } else {
            system("mv $from $to") == 0 or &my_die( "Unable to move $file into workdir $workdir: $!\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);
        }
    }
} elsif ($is_tarfile and $no_extract) {
    print "skipping extraction of tarfile $filename\n";
} else {
    &my_die( "Unrecognised file: $file\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);
}

my $mjd_extract = DateTime->now->mjd;   # MJD of finishing extract

# All done
{
    my $command = "$receivetool -addresult";
    $command .= " -file_id $file_id";
    $command .= (" -dtime_copy " . (($mjd_copy - $mjd_start) * 86400));
    $command .= (" -dtime_extract " . (($mjd_extract - $mjd_copy) * 86400));
    $command .= " -dbname $dbname" if defined $dbname;


    print "$command\n";

    unless (defined $no_update) {
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        die "Unable to add result for $file\n" unless $success;
    }
}

# Pau.

sub read_dirinfo_file
{
    my $filename = shift;
    my $file_id = shift;

    my $resolved = $ipprc->file_resolve($filename);
    &my_die("failed to resolve dirinfo file: $filename ", $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$resolved;

    open INFILE, $resolved or &my_die( "Can't open $resolved\n", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    my @lines = (<INFILE>);

    my $dirinfo = join "", @lines;

    close INFILE;

    my $metadata = $mdcParser->parse($dirinfo) or
        &my_die("Unable to parse metadata config doc", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    my $array = parse_md_list($metadata) or
        &my_die("Unable to parse metadata list", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    my $dest_hash = $array->[0];

    my $destdir = $dest_hash->{destdir};
    &my_die("destdir not found in $filename", $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$destdir;

    my $components = $array->[1];

    return ($destdir, $components, \@lines);
}

# edit a config dump file replacing the "volume" value with the new local value: $workdir
sub edit_mdc_file
{
    my $file_id = shift;
    my $src = shift;
    my $dest = shift;
    my $workdir = shift;

    my $filecmd_output = `file $src`;
    &my_die("failed to determinte file type of $src", $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$filecmd_output;
    chomp $filecmd_output;

    my $tmpName;
    if ($filecmd_output =~ /gzip/) {
        my $tmpfile;
        ($tmpfile, $tmpName) = tempfile( "/tmp/receive.XXXX", UNLINK => !$save_temps );
        close($tmpfile) or &my_die("failed to close $tmpName", $file_id, $PS_EXIT_UNKNOWN_ERROR);

        my $command = "$gunzip -c $src > $tmpName";
        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
        unless ($success) {
            $error_code = (($error_code >> 8) or $PS_EXIT_PROG_ERROR);
            &my_die("Unable to perform $command: $error_code", $component, $error_code);
        }
        $src = $tmpName;
    }

    open my $IN,  "<$src" or &my_die("failed to open $src for input", $file_id, $PS_EXIT_UNKNOWN_ERROR);
    open my $OUT, ">$dest" or &my_die("failed to open $dest for output", $file_id, $PS_EXIT_UNKNOWN_ERROR);

    # Assumed file structure
    # stuff
    # FILES.INPUT metadata
    # FILES.OUTPUT metadata
    # more stuff
    # only the paths in the FILES.* metadata are monkeyed with
    my $done_editing = 0;
    my $numFilesMD = 0;
    foreach my $line (<$IN>) {
        my $out_line = $line;
        if (!$done_editing) {
            my (@words) = split " ", $line;
            if (scalar @words) {
                # get rid of any leading blank words
                while ((scalar @words) and !defined $words[0]) {
                    shift @words;
                }

                if ($words[1] and $words[1] eq "METADATA") {
                    if ( $words[0] =~ /^FILES\..+/ ) {
                        $numFilesMD++;
                    }
                } elsif ($words[0] eq "END") {
                    # when we get to the end of the second FILES metadata we're done editing
                    if ($numFilesMD == 2) {
                        $done_editing = 1;
                    }
                } elsif ($numFilesMD and ($words[1] eq "STR"))  {
                    # we're processing one of the files metadata edit the path
                    my $key = shift @words;
                    my $type = shift @words;
                    my $path = shift @words;
                    my $extra = join " ", @words;

                    $path = edit_path($file_id, $workdir, $path);

                    $out_line = "\t" . $key ."\t" . "STR" . "\t" . $path;
                    $out_line .= "\t" . $extra if $extra;
                    $out_line .= "\n";
                }
            }
        }
        print $OUT $out_line;
    }

    close $IN;
    close $OUT or &my_die("failed to close $dest", $file_id, $PS_EXIT_UNKNOWN_ERROR);
}


# XXX: this should go into a module
# Replace 'volume portion of path with $workdir/
# Volume is defined here by
#   neb://volume/
#   /xxx/xxxxx/         i.e. /data/ippxxx.y/
#   file://xxx/xxxxx/   i.e. file://data/ippxxx.y/
#   path://somepath/
sub edit_path
{
    my $file_id = shift;
    my $workdir = shift;
    my $path = shift;

    my $scheme = file_scheme($path);
    my $tail;
    if ($scheme) {
            # strip off scheme://
        $tail = substr($path, length($scheme) + 3);
    } elsif (substr($path, 0, 1) eq '/') {
        $tail = substr($path, 1);
        $scheme = "";
    }
    # remove any leading / that are left
    while ((substr($tail, 0, 1) eq '/')) {
        $tail = substr($tail, 1);
    }

    my @segments;
    if (($scheme eq 'neb') or ($scheme eq 'path')) {
        my $volume;
        ($volume, @segments) = split '/', $tail;

    } elsif (!$scheme or ($scheme eq 'file')) {

        # XXX Here we're assuming the /data/ipp??? structure. This won't be true when data is forwarded
        # by remote sites. We need a way to configure this
        my $volume;

        # data/ippxxx/dirs
        (undef, $volume, @segments) = split '/', $tail;
    } else {
        &my_die( "unexpected workdir value: $path\n", $file_id, $PS_EXIT_PROG_ERROR);
    }

    my $new_path = caturi($workdir, @segments);

    return $new_path;
}
sub findRunType {
    my $lines = shift;
    my $runType;
    my ($firstWord, $multi) = split " ", $lines->[0];
    &my_die( "unexpected first line found in $filename: $lines->[0]\n",
                $file_id, $PS_EXIT_UNKNOWN_ERROR) if ($firstWord ne 'dbversion') or ($multi ne 'MULTI');

    my $dbversionDone = 0;
    foreach my $line (@$lines) {
        # skip blank lines
        next if !$line or $line eq "\n";

        ($firstWord, $multi) = split " ", $line;
        if ($dbversionDone) {
            # The first non blank line after the dbversion tells us the run type.
            # From this we get the stage
            &my_die( "unexpected line found in $filename: $line\n",
                $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$firstWord or !$multi or ($multi ne 'MULTI');

            $runType = $firstWord;
            last;
        } else {
            if ($firstWord and $firstWord eq 'END') {
                # we're past the dbversion file now
                $dbversionDone = 1;
            }
        }
    }
    &my_die( "failed to determine run type from $filename",
                $file_id, $PS_EXIT_UNKNOWN_ERROR) if !$runType;

    return $runType;
}

sub my_die
{
    my $msg = shift;            # Exit message
    my $file_id = shift;        # File identifier
    my $fault = shift;          # Fault code

    $fault = $PS_EXIT_PROG_ERROR unless defined $fault;

    carp($msg);
    if (defined $file_id and not $no_update) {
        my $command = "$receivetool -addresult";
        $command .= " -file_id $file_id";
        $command .= " -fault $fault";
        $command .= " -dbname $dbname" if defined $dbname;

        my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
            run(command => $command, verbose => $verbose);
    }
    exit $fault;
}


__END__
