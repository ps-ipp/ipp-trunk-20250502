# Copyright (c) 2006  Paul Price, Joshua Hoblitt
#
# $Id: Config.pm,v 1.96 2009-02-05 20:29:43 eugene Exp $

package PS::IPP::Config;

use strict;
use warnings FATAL => qw( all );

our $VERSION = '1.01';

use DBI;
use Carp qw( carp );
use File::Basename qw( basename dirname );
# use File::Spec 3.19;
use File::Spec 0.87;
use File::Temp qw( tempfile );

use PS::IPP::Metadata::Config 1.00;
use Getopt::Long 2.33 qw( GetOptions :config gnu_getopt pass_through ); # Set pass_through so we don't kill @ARGV
use IPC::Cmd 0.36 qw( can_run run );
use POSIX qw( strftime );

use base qw( Exporter Class::Accessor::Fast );
__PACKAGE__->mk_accessors( qw( path ) );

our @EXPORT_OK = qw(
                    $PS_EXIT_SUCCESS
                    $PS_EXIT_UNKNOWN_ERROR
                    $PS_EXIT_SYS_ERROR
                    $PS_EXIT_CONFIG_ERROR
                    $PS_EXIT_PROG_ERROR
                    $PS_EXIT_DATA_ERROR
                    $PS_EXIT_TIMEOUT_ERROR
                    $PS_TABLE_ID_CHIP
                    $PS_TABLE_ID_WARP
                    $PS_TABLE_ID_STACK
                    $PS_TABLE_ID_DIFF
                    metadataLookupStr
                    metadataLookupMD
                    metadataLookupBool
                    metadataLookupS32
                    metadataLookupF32
                    caturi
                    file_scheme
                    );
our %EXPORT_TAGS = (standard => [@EXPORT_OK]);

our $PS_EXIT_SUCCESS = 0;
our $PS_EXIT_UNKNOWN_ERROR = 1;
our $PS_EXIT_SYS_ERROR = 2;
our $PS_EXIT_CONFIG_ERROR = 3;
our $PS_EXIT_PROG_ERROR = 4;
our $PS_EXIT_DATA_ERROR = 5;
our $PS_EXIT_TIMEOUT_ERROR = 6;

our $PS_TABLE_ID_CHIP = 1;
our $PS_TABLE_ID_WARP = 2;
our $PS_TABLE_ID_STACK = 3;
our $PS_TABLE_ID_DIFF = 4;
our $PS_TABLE_ID_STATICSKY = 5;

our $parser = PS::IPP::Metadata::Config->new; # Metadata parser
$parser->overwrite(0);                # Turn off overwrite

#$::RD_TRACE = 1;
#$::RD_HINT = 1;
#use Data::Dumper;

# Given a parsed metadata, parse it into an array of hashes
sub new {
    my $class = shift;                # Class name
    my $camera = shift;                # Camera name

    unless (defined $class) {
        carp "Programming error";
        return undef;
    }

    # Get location of ipprc file
    my $name;                        # Name of ipprc file
    GetOptions( 'site=s' => \$name );
    $name = $ENV{IPPRC} if not defined $name;
    if (not defined $name and defined $ENV{HOME}) {
        $name = $ENV{HOME} . '/.ipprc';
    }

    my $file;                        # File handle
    unless (open $file, $name) {
        carp "Unable to open ipprc file $name: $!";
        return undef;
    }
    my @contents = <$file>;        # Contents of the ipprc file
    close $file;

    my $mdc = $parser->parse( join '', @contents); # The parsed metadata config

    my $path = _interpolate_env(metadataLookupStr($mdc, 'PATH')); # The path
    unless (defined $path) {
        carp "PATH is not set in $name\n";
        return undef;
    }

    # Interpolate environment variables in the work directory

    my $self = {
        path => $path,                # Path
        camera => undef,        # Camera configuration
        filerules => undef,        # File rules from the camera configuration
        rejection => undef,        # Rejection data
        reductionClasses => undef, # Reduction class definitions
        _userConfig => $mdc,        # The top-level user configuration
        _siteConfig => undef,        # The site configuration (from _userConfig)
        _systemConfig => undef        # The system configuration (from _userConfig)
        };
    bless $self, $class;

    $self->load_site() or return undef;
    $self->load_system() or return undef;

    ( $self->define_camera($camera) or return undef ) if defined $camera;

    return $self;
}

# Find a configuration file, which might be anywhere in the PATH
sub _find_config
{
    my $self = shift;                # Configuration object
    my $filename = shift;        # File name to find

    my @path = split /:/, $self->{path}; # List of paths to try (I need this: ,/ for emacs fonts...)
    my $found = 0;                        # Found it?
    my $realfile;
    my $tries = 0;
    while (not $found) {
        ## try a second time
        foreach my $path (@path) {
            $realfile = File::Spec->rel2abs( $filename, $path );
            if ($tries) {
                print "re-trying ($tries): filename: $filename, path: $path -> ";
                print "test: $realfile\n";
            }
            if (-f $realfile) {
                $found = 1;
                return $realfile;
            }
        }
        if (not $found) {
            $tries ++;
            if ($tries > 4) {
                unless ($found) {
                    carp "Unable to find camera configuration file $filename\n";
                    return undef;
                }
            }
            ## try again after a moment?
            ## system ("df");
            select(undef, undef, undef, 0.25);
        }
    }

    carp "Unable to find configuration file: $filename";
    return undef;
}

# Load the site config file
sub load_site
{
    my $self = shift;                # Configuration object

    my $i = 0;

    unless (defined $self) {
        carp "Programming error";
        return undef;
    }

    my $filename = metadataLookupStr($self->{_userConfig}, 'SITE'); # Site config file
    unless (defined $filename) {
        carp "Unable to find site configuration file\n";
        return undef;
    }

    my $realfile = $self->_find_config($filename); # Resolved filename, after hunting the PATH
    ( carp "Unable to find site configuration" and return undef ) unless defined $realfile;

    # Read the file
    my $file;                        # File handle
    unless (open $file, $realfile) {
        carp "Unable to open site configuration file $realfile: $!";
        return undef;
    }

    my @contents = <$file>;
    close $file;
    $self->{_siteConfig} = $parser->parse( join '', @contents); # The parsed metadata config

    unless (defined $self->{_siteConfig}) {
        carp "Failure to parse the site configuration file $realfile";
        return undef;
    }

    return $self;
}

# Load the system config file
sub load_system
{
    my $self = shift;                # Configuration object

    unless (defined $self) {
        carp "Programming error";
        return undef;
    }

    my $filename = metadataLookupStr($self->{_userConfig}, 'SYSTEM'); # System config file
    unless (defined $filename) {
        carp "Unable to find system configuration file\n";
        return undef;
    }

    my $realfile = $self->_find_config($filename); # Resolved filename, after hunting the PATH
    ( carp "Unable to find system configuration" and return undef ) unless defined $realfile;

    # Read the file
    my $file;                        # File handle
    unless (open $file, $realfile) {
        carp "Unable to open system configuration file $realfile: $!";
        return undef;
    }
    my @contents = <$file>;
    close $file;
    $self->{_systemConfig} = $parser->parse( join '', @contents); # The parsed metadata config

    unless (defined $self->{_systemConfig}) {
        carp "Failure to define system configuration";
        return undef;
    }

    return $self;
}

# Define a camera to use
sub define_camera
{
    my $self = shift;                # Configuration object
    my $name = shift;                # Camera name

    unless (defined $self and defined $name) {
        carp "Programming error";
        return undef;
    }

    my $camera_list = metadataLookupMD($self->{_systemConfig}, 'CAMERAS'); # List of cameras
    my $filename = metadataLookupStr($camera_list, $name); # Filename of camera configuration
    unless (defined $filename) {
        carp "Unable to find configuration file for camera $name\n";
        return undef;
    }

    my $realfile = $self->_find_config($filename); # Resolved filename, after hunting the PATH
    ( carp "Unable to find camera configuration" and return undef ) unless defined $realfile;

    # Read the file
    my $file;                        # File handle
    unless (open $file, $realfile) {
        carp "Unable to open camera configuration file $realfile: $!";
        return undef;
    }
    my @contents = <$file>;
    close $file;
    $self->{camera} = $parser->parse( join '', @contents); # The parsed metadata config
    $self->{cameraName} = $name;

    unless (defined $self->{camera}) {
        carp "Failure to define camera";
        return undef;
    }

    # XXX why isn't just $self being returned here? -JH
    return defined $self->{camera};
}

# Return the scheme used for a filename
sub file_scheme
{
    my $name = shift;                # Filename for which to get the scheme
    my ($scheme) = $name =~ /^(path|neb|file):/; # The scheme, e.g., file://, path://
    # $scheme may be undef if the input doesn't contain one of the above recognised schemes
    return $scheme;
}


# Concatenate elements of a URI
sub caturi
{
    my $base = shift;                # Base name (might be "foo /foo path://SOMETHING" or "neb://SOMETHING")
    my @segments = @_;                # Path segments

    carp "base is not inited" if not defined $base;
    my $scheme = file_scheme($base); # The scheme, e.g., file://, path://
    $base =~ s|^$scheme:/*|| if defined $scheme;

    my $root = '';
    if ($base =~ m|^/|) { $root = '/'; }

    unshift @segments, $base if $base =~ /\S+/;

    # Remove leading and trailing slashes
    foreach my $segment ( @segments ) {
        $segment =~ s|^/*||;
        $segment =~ s|/*$||;
    }

    # Join everything together
    my $joined = join('/', @segments);
    if (defined $scheme) {
        $joined = lc($scheme) . "://" . $joined;
    } else {
        $joined = $root . $joined;
    }

    return $joined;
}

# select a datapath from config.site
sub datapath
{
    my $self = shift;                # Configuration object
    my $name = shift;                # datapath name

    unless (defined $self and defined $name) {
        carp "Programming error";
        return undef;
    }

    my $path_list = metadataLookupMD($self->{_siteConfig}, 'DATAPATH'); # List of paths
    my $pathname = metadataLookupStr($path_list, $name); # Path of interest
    unless (defined $pathname) {
        carp "Unable to find datapath $name\n" ;
        return undef;
    }

    return $pathname;
}

# convert the database name and the table ID to a image source id
sub source_id
{
    my $self = shift;
    my $dbname = shift;
    my $table = shift;

    my $dbserver = metadataLookupStr($self->{_siteConfig}, "DBSERVER");
    my $dbuser = metadataLookupStr($self->{_siteConfig}, "DBUSER");
    my $dbpassword = metadataLookupStr($self->{_siteConfig}, "DBPASSWORD");
    my $dbport = metadataLookupS32($self->{_siteConfig}, "DBPORT");
    my $admindb = "ippadmin";

    ( carp "dbserver not defined in configuration"  and return undef ) unless defined($dbserver);
    ( carp "dbuser not defined in configuration"  and return undef ) unless defined($dbuser);
    ( carp "dbpassword not defined in configuration"  and return undef ) unless defined($dbpassword);
    ( carp "dbname not defined in configuration"  and return undef ) unless defined($dbname);

    my $dsn = "DBI:mysql:host=$dbserver;database=$admindb";
    $dsn .= ";port=$dbport" if $dbport;     # if $dbport eq "0" we use the default
    my $dbh = DBI->connect($dsn, $dbuser, $dbpassword) or die "Cannot connect to database.\n";

    my $query = "SELECT proj_id FROM projects WHERE projname = \'$dbname\'";
    my $stmt = $dbh->prepare($query);
    $stmt->execute();
    my $ref = $stmt->fetchrow_hashref();
    ( carp "ippdb $dbname not found" and return undef ) unless ($ref);

    my $proj_id = $ref->{proj_id};
    $stmt->finish();
    ( carp "proj_id for $dbname not found in $admindb" and return undef ) unless $proj_id;


    my $source_id = ($proj_id << 3) | $table;
    return $source_id;
}

# Start up Nebulous
sub _neb_start
{
    my $self = shift;                # Configuration object

    eval {
        require Nebulous::Client;
    };
    if ($@) {
        carp "Can't find Nebulous::Client.";
        return undef;
    }

    return $self->{nebulous} if defined $self->{nebulous}; # Already started

    # For consistency with the rest of the IPP, don't use site.config
    # to set NEB_SERVER use environment variable.
    # my $server = metadataLookupStr( $self->{_siteConfig}, 'NEB_SERVER' ); # Nebulous server
    my $server = $ENV{NEB_SERVER};
    unless (defined $server) {
        carp "Unable to find NEB_SERVER in camera configuration file.";
        return undef;
    }

    my $neb = eval { Nebulous::Client->new( proxy => $server ); };
    if ($@ or not defined $neb) {
        carp "Unable to create a Nebulous::Client object with proxy => $server";
        return undef;
    }

    $self->{nebulous} = $neb;

    return $neb;
}

sub nebulous
{
    my $self = shift;
    carp "no parameters are allowed" if @_;

    # check to see if we already have a live nebulous object
    return $self->{nebulous} if defined $self->{nebulous};

    # if not, call _neb_start() and return the new object
    (return $self->_neb_start()) or ( carp "Can't start Nebulous" and return undef );
}

# Resolve a URI to a file name
sub file_resolve
{
    my $self = shift;                # Configuration object
    my $name = shift;                # File name to check
    my $create_if_doesnt_exist = shift;

    my $scheme = file_scheme($name); # The scheme, e.g., file://, path://

    if ($scheme) {
        $scheme = lc($scheme);
        # print "scheme: $scheme\n";

        if ($scheme eq 'neb') {
            my $neb = $self->nebulous; # Nebulous handle
            if ($create_if_doesnt_exist) {
                my $status = eval { $neb->stat( $name ); };
                ( carp "Unable to stat Nebulous handle $name" and return undef ) if $@;
                unless ($status) {
                    # print "entry $name not found, creating...\n";
                    my $uri = eval { $neb->create( $name ); };
                    if ($@ or not defined $uri) {
                        carp "unable to instantiate $name.";
                        return undef;
                    }
                    my $path = URI->new( $uri )->path;
                    # print "created path: $path\n";
                    return $path;
                }
            }
            my $path = eval { $neb->find( $name ); };
            if ($@ or not defined $path) {
                carp "neb entry $name not found, not created\n";
                return undef;
            }
            # print "found path: $path\n";
            return $path;
        }

        if ($scheme eq 'path' or $scheme eq 'file') {
            # guaranteed to have a scheme (path:// or file://)
            $name = $self->convert_filename_absolute( $name ) or return undef;
            # print "resolved path to $name\n";
        }
    }

    if ($create_if_doesnt_exist && ! -e $name) {
        # make sure the file's parent directory exists
        my $dir = dirname($name);
        if (! -e $dir) {
            my $rc = system "mkdir -p $dir";
            ( carp "failed to create directory for $name" and return undef ) unless (!$rc);
        } elsif (! -d $dir ) {
            carp "parent for $name exists and is not a directory";
            return undef;
        }

        open F, ">$name" or ( carp "failed to create $name" and return undef );
        close F;
        # print "created target $name\n";
    }

    return $name;
}

# Create and open file
sub file_create_open
{
    my $self = shift;                # Configuration object
    my $name = shift;                # File name to check

    $self->file_prepare( $name ) or return undef;

    my $scheme = file_scheme($name); # The scheme, e.g., file://, path://
    if (defined $scheme) {
        $scheme = lc($scheme);
        if ($scheme eq 'neb') {
            my $fh = eval { $self->nebulous->open_create( $name ); };
            if ($@ or not defined $fh) {
                carp "Unable to open/create Nebulous handle $name";
                return undef;
            }
            return $fh;
        }
        if ($scheme eq 'path' or $scheme eq 'file') {
            # guaranteed to have a scheme (path:// or file://)
            $name = $self->convert_filename_absolute( $name ) or return undef;
        }
    }

    if (-f $name) {
        carp "Unable to create file $name --- file exists.";
        return undef;
    }

    my $fh;
    unless (open $fh, '>', $name) {
        carp "Unable to create file $name --- $!";
        return undef;
    }
    return $fh;
}

# Create and open file for appending
sub file_create_append
{
    my $self = shift;                # Configuration object
    my $name = shift;                # File name to check

    $self->file_prepare( $name ) or return undef;

    my $scheme = file_scheme($name); # The scheme, e.g., file://, path://
    if (defined $scheme) {
        $scheme = lc($scheme);
        if ($scheme eq 'neb') {
            my $fh = eval { $self->nebulous->open_create( $name ) };
            if ($@ or not defined $fh) {
                carp "Unable to open/create Nebulous handle $name";
                return undef;
            }
            return $fh;
        }
        if ($scheme eq 'path' or $scheme eq 'file') {
            # guaranteed to have a scheme (path:// or file://)
            $name = $self->convert_filename_absolute( $name ) or return undef;
        }
    }

    my $fh;
    unless (open $fh, '>>', $name) {
        carp "Unable to create file $name --- $!";
        return undef;
    }
    return $fh;
}

# Create a file (intended principally to abstract Nebulous files)
sub file_create
{
    my $self = shift;                # Configuration object
    my $name = shift;                # File name to check

    $self->file_prepare( $name ) or return undef;

    my $scheme = file_scheme($name); # The scheme, e.g., file://, path://

    if (defined $scheme and lc($scheme) eq 'neb') {
        my $save_name = $name;
        $name = eval { $self->nebulous->create( $name ) };

        if ($@ or not defined $name) {
            carp "Unable to create Nebulous handle $save_name";
            return undef;
        }
    }

    return $self->file_resolve($name);
}

# Check if a file exists
sub file_exists
{
    my $self = shift;                # Configuration object
    my $name = shift;                # File name to check

    my $scheme = file_scheme($name); # The scheme, e.g., file://, path://
    if (defined $scheme and lc($scheme) eq 'neb') {
        my $found = eval { $self->nebulous->find( $name ); };
        ( carp "Unable to find instance of Nebulous handle $name" and return undef ) if $@;
        return 0 unless defined $found;
        return (-f $found and -s $found);
    }

    return (-f $self->file_resolve($name) and -s $self->file_resolve($name));
}

# Copy a file
sub file_copy
{
    my $self = shift;                # Configuration object
    my $source = shift;                # Name of source file
    my $target = shift;                # Name of target file

    $self->file_prepare( $target ) or return undef;

    my $scheme = file_scheme($target); # The scheme, e.g., file://, path://
    if (defined $scheme and lc($scheme) eq 'neb') {
        $target = eval { $self->nebulous->create( $target ); };
        if ($@ or not defined $target) {
            carp "Unable to create Nebulous handle";
            return undef;
        }
    }
    $target = $self->file_resolve( $target );
    $source = $self->file_resolve( $source );

    system("cp $source $target") == 0 or ( carp "Can't copy file $source to $target." and return undef );
    return 1;
}

# create a (possibly) temporary file with name (basename $_[0])
# if $preserve create a file named basename$_[0] in current directory

sub create_temp_file
{
    my $self     = shift;
    my $pathname = shift;
    my $preserve = shift;

    ( carp "pathname must be defined" and return undef ) unless ($pathname);

    my $fileRef;
    my $fileName;

    # we only use in the last part of the path
    my $base = basename($pathname);

    if ($preserve) {
        # we want to keep the file just create it in the current directory
        $fileName = "./$base";
        open $fileRef, ">$fileName" or ( carp "can't open $fileName for output" and return undef );
    } else {
        #  we really want a tempfile, so put it in /tmp
        ($fileRef, $fileName) = tempfile("/tmp/$base.XXXX", UNLINK => 1);
    }

    return ($fileRef, $fileName);
}
# Delete a file
sub file_delete
{
    my $self = shift;                # Configuration object
    my $name = shift;                # File name to check
    my $force = shift;               # pass force flag to the delete method
    my $status;

    my $scheme = file_scheme($name); # The scheme, e.g., file://, path://
    if (defined $scheme and lc($scheme) eq 'neb') {
        $status = eval { $self->nebulous->delete( $name, $force ); };
        ( carp "Unable to delete Nebulous handle $name" and return undef ) if $@;
    } else {
        my $resolved = $self->file_resolve($name) or return undef;
        if (defined $resolved and -e $resolved) {
            $status = unlink($resolved);
        }
    }
    return $status;
}

# delete existing destreak backup file if it exists
sub delete_destreak_backup_file
{
    my $self = shift;
    my $name = shift;
    my $force = shift;

    my $dir = dirname($name);
    my $base = basename($name);
    my $backup =  "$dir/SR_$base";

    if ($self->file_exists($backup)) {
        return $self->file_delete($backup, $force);
    }
    return 1;
}



# redirect stderr and stdout streams to a file
sub redirect_output
{
    my $self = shift;
    my $name = shift;

    ( carp "need redirection target" and return undef ) unless $name;

    my $filename = $self->file_resolve($name, 1);

    ( carp "cannot resolve $name" and return undef ) unless $filename;

    if (! open(STDOUT, ">>$filename") ) {
        # try to work around the 'nfs glitch' by waiting and retrying the open a couple of times
        print STDERR "failed to redirect stdout to $filename re-trying\n";
        my $max_tries = 10;
        my $try = 2;
        while (! open STDOUT, ">>$filename" ) {
            if ($try == $max_tries) {
                carp "failed to redirect stdout to $filename after trying $max_tries times";
                return undef;
            }
            sleep 5;
            $try++;
        }
        print STDERR "   redirect stdout to $filename succeded on try $try\n";
    }
    open STDERR, ">>$filename" or ( carp "failed to redirect stderr to $filename" and return undef );

    return 1;
}

sub file_rename {
    my $self = shift;
    my $old_name = shift;
    my $name = shift;

    my $scheme = file_scheme($old_name);
    my $newscheme = file_scheme($name);
    if ($scheme and $scheme eq 'neb') {
        if (!$newscheme or $newscheme ne 'neb') {
            carp "cannot rename nebulous file to non nebulous path";
            return undef;
        }
        my $neb = $self->nebulous;
        if ($neb->storage_object_exists($old_name)) {
            eval {
                $neb->move($old_name, $name);
            };
            if ($@) {
                carp "nebulous move failed for $old_name";
                return undef;
            }
        } else {
            carp "$old_name not found";
            return undef;
        }
    } else {
        if ($newscheme and $newscheme eq 'neb') {
            carp "cannot rename non-nebulous file to nebulous path";
            return undef;
        }
        if ($self->file_exists($old_name)) {
            my $resolved = $self->file_resolve($old_name);
            my $resolved_new = $self->file_resolve($name);
            if (!rename($resolved, $resolved_new)) {
                carp "Failed to rename $resolved $resolved_new";
                return undef;
            }
        } else {
            carp "$old_name not found";
            return undef;
        }
    }
    return $name;
}

# redirect stderr and stdout streams to log file. If file already exists rename it and record
# the name in the new log file.
sub redirect_to_logfile {
    my $self = shift;
    my $name = shift;
    
    my $tstring = strftime "%Y%m%dT%H%M%S", gmtime;

    my $old_name;
    my $scheme = file_scheme($name);
    if ($scheme and $scheme eq 'neb') {
        my $neb = $self->nebulous;
        if ($neb->storage_object_exists($name)) {
            eval {
                $old_name = "$name.$tstring";
                $neb->move($name, $old_name);
            };
            if ($@) {
                carp "nebulous move failed for $name";
                return undef;
            }
        }
    } else {
        if ($self->file_exists($name)) {
            my $resolved = $self->file_resolve($name);
            $old_name = "$resolved.$tstring";
            if (!rename($resolved, $old_name)) {
                carp "Failed to rename $resolved $old_name";
                return undef;
            }
        }
    }
    my $result = $self->redirect_output($name);
    if ($result and $old_name) {
        print "\nPrevious log file moved to $old_name\n";
    }
    return $result;
}

# Strip the filename (the last element of the URI), returning the scheme and directory part
sub _strip_filename
{
    my $name = shift;                # File name of interest
    $name =~ s|/[\w.-]*?$||;
    return $name;
}

# Prepare to receive a new file --- create the directory, if appropriate.  Return the appropriate filename
# Does not register anything with Nebulous
sub file_prepare
{
    my $self = shift;                # Configuration object
    my $name = shift;                # File name for which to prepare
    my $workdir = shift;        # Working directory
    my $template = shift;        # Template filename from which to get working directory if

    if (defined $workdir) {
        $name = caturi( $workdir, $name );
    } elsif (defined $template) {
        # Take directory from template and apply to name
        my $dir = _strip_filename( $template );
        $name = caturi( $dir, $name );
    }

    my $scheme = file_scheme($name); # The scheme, e.g., file://, path://
    return $name if defined $scheme and lc($scheme) eq 'neb'; # Nothing to be done: Nebulous handles it all

    # Might need to create a directory
    # not guaranteed to have a scheme (path:// or file://) - might be /PATH/foobar
    # a relative path (PATH/foobar) is invalid here
    my $resolved = $self->convert_filename_absolute( $name ) or return undef;
    my ( $vol, $dirs, $file ) = File::Spec->splitpath( $resolved );
    unless (-d $dirs) {
        system("mkdir -p $dirs") == 0 or ( carp "Can't create directory $dirs" and return undef );
    }

    return $name;
}

# Prepare to receive a new file --- create the directory, if appropriate.  Return the appropriate filename
# Does not register anything with Nebulous
sub outroot_prepare
{
    my $self = shift;                # Configuration object
    my $outroot = shift;        # Working directory

    # outroot is a directory + fragement of a filename

    my $scheme = file_scheme($outroot); # The scheme, e.g., file://, path://
    return 1 if defined $scheme and lc($scheme) eq 'neb'; # Nothing to be done: Nebulous handles it all

    # Might need to create a directory
    # not guaranteed to have a scheme (path:// or file://) - might be /PATH/foobar
    # a relative path (PATH/foobar) is invalid here
    my $resolved = $self->convert_filename_absolute( $outroot ) or return undef;
    my ( $vol, $dirs, $file ) = File::Spec->splitpath( $resolved );
    unless (-d $dirs) {
        system("mkdir -p $dirs") == 0 or ( carp "Can't create directory $dirs" and return undef );
    }

    return 1;
}

# Convert a relative filename (e.g., "path://PATH/file") to an absolute (e.g., "/path/to/file")
# note that file://PATH/file resolves to /PATH/file, regardless of the number of leading slashes
sub convert_filename_absolute
{
    my $self = shift;                # Configuration object
    my $name = shift;                # raw name

    unless (defined $self and defined $name) {
        carp "Programming error";
        return undef;
    }

    $name =~ s|/$||; # drop tailing slashes (foobar/ to foobar)
    my $scheme = file_scheme($name); # The scheme, e.g., file, path

    ## if this is already an absolute path (/PATH/file), just return the path
    unless (defined $scheme) {
        if ($name =~ m|^/|) { return $name; }
        # without a leading slash, this is an error
        carp "Relative file name provided: relative paths are not permitted.";
        return undef;
    }

    $name =~ s|^$scheme:/*||;

    if (lc($scheme) eq 'file') {
        # the above strips of the leading slash; replace it for file://
        $name = '/' . $name;
        return $name;
    }

    if (lc($scheme) eq 'path') {
        my @dirs = split('/', $name);
        my $pathName = shift @dirs;
        my $path = $self->datapath( $pathName );
        $path =~ s|/*$||;
        return File::Spec->catfile( $path, @dirs );
    }

    # It's already absolute
    return $name;
}

# Convert a relative filename (e.g., "/path/to/file") to an absolute (e.g., "path://PATH/file")
sub convert_filename_relative
{
    my $self = shift;                # Configuration object
    my $name = shift;                # raw name

    unless (defined $self and defined $name) {
        carp "Programming error";
        return undef;
    }

    # First, check to see if it's already in a relative form
    my $scheme = file_scheme($name); # The scheme, e.g., file, path
    if (defined $scheme) {
        $scheme = lc($scheme);
        if ($scheme eq 'path' or $scheme eq 'file') {
            # We may as well search for a 'better' path
            # guaranteed to have a scheme (path:// or file://)
            $name = $self->convert_filename_absolute( $name ) or return undef;
        } elsif ($scheme eq 'neb') {
            # No chance of changing anything --- move along
            return $name;
        }
    }

    $name = File::Spec->canonpath( $name); # Clean up
    my @dirs = File::Spec->splitdir( $name );

    my $path_list = metadataLookupMD($self->{_siteConfig}, 'DATAPATH'); # List of paths
    my $best_path;
    my $best_name;
    my $best_score;
  CFR_PATH_CHECK: foreach my $path_item (@$path_list) {
      my $path_name = $path_item->{name}; # Name of the path
      my $path = File::Spec->canonpath( $path_item->{value} ); # The path
      $path =~ s|/*$||;
      my @path_dirs = File::Spec->splitdir( $path );

      # Check if the path is suitable
      next if scalar @path_dirs > scalar @dirs;
      for (my $i = 0; $i < scalar @path_dirs; $i++) {
          next CFR_PATH_CHECK if $path_dirs[$i] ne $dirs[$i];
      }
      # It is suitable; see if it is 'best' (minimizes the number of directories)
      my $score = scalar @path_dirs;
      if (not defined $best_score or $score > $best_score) {
          $best_path = $path;
          $best_score = $score;
          $best_name = $path_name;
      }
  }

    $name =~ s|^/||;
    $name =~ s|/$||;
    if (defined $best_score) {
        $best_path =~ s|^/||;
        $best_path =~ s|/$||;
        $name =~ s|^$best_path|$best_name|;
        return 'path://' . $name;
    }
    return 'file://' . $name;
}

# Return a rejection limit from the camera configuration
sub rejection
{
    my $self = shift;                # Configuration object
    my $name = shift;                # Name of limit
    my $type = lc(shift);        # Type of frame
    my $filter = shift;                # Filter name, for extra qualification; optional

    unless (defined $self and defined $name and defined $type) {
        carp "Programming error";
        return undef;
    }

    unless (defined $self->{rejection}) {
        my $camera = $self->{cameraName}; # Camera name
        unless (defined $camera) {
            carp "Camera has not yet been defined.\n";
            return undef;
        }

        # rejections are saved as a recipe: REJECTIONS
        my @rejContents = `ppConfigDump -dump-recipe REJECTIONS -camera $camera -`;

        # load from resulting psMetadataConfig
        $self->{rejection} = $parser->parse( join '', @rejContents); # The rejection metadata
        unless (defined $self->{rejection}) {
            carp "Unable to parse REJECTION recipe for $camera.";
            return undef;
        }
    }

    my $rejection = $self->{rejection};

    # Find the appropriate entry
  TYPES: foreach my $item (@$rejection) {
      if (lc($item->{name}) eq $type) {
          unless ($item->{class} eq "metadata") {
              carp "$name within REJECTIONS is not of type METADATA";
              return undef;
          }
          my $limits = $item->{value}; # List of rejection limits

          # Check the FILTER first
          foreach my $limit (@$limits) {
              if ($limit->{name} eq 'FILTER') {
                  if ($limit->{value} eq '*' or (defined $filter and $limit->{value} eq $filter)) {
                      last;
                  }
                  next TYPES; # Doesn't match --- look at the next one
              }
          }

          foreach my $limit (@$limits) {
              return $limit->{value} if $limit->{name} eq $name;
          }
          carp "Unable to find limit $name in REJECTIONS list for $type.\n";
          return undef;
      }
  }

    if (not defined $filter) {
        carp "Unable to find type $type with no FILTER in REJECTIONS list.\n";
    } else {
        carp "Unable to find type $type with FILTER $filter in REJECTIONS list.\n";
    }
    return undef;
}

# Return a filename from the camera configuration FILERULES
sub filename
{
    my $self = shift;                # Configuration object
    my $name = shift;                # Name of the file
    my $output = shift;                # For replacing {OUTPUT}
    my $component = shift;        # For replacing {CHIP.NAME} and {CELL.NAME}

    unless (defined $self and defined $name and defined $output) {
        carp "Programming error: required inputs left undefined";
        return undef;
    }

    my $filerules;                # File rules
    if (defined $self->{filerules}) {
        $filerules = $self->{filerules};
    } else {
        # Get the file rules from the camera configuration

        my $camera = $self->{camera}; # Camera configuration
        unless (defined $camera) {
            carp "Camera has not yet been defined.\n";
            return undef;
        }

        $filerules = metadataLookup($camera, 'FILERULES');        # File rules
        unless (defined $filerules) {
            carp "Can't find FILERULES list in camera configuration.\n";
            return undef;
        }

        if ($filerules->{class} eq "scalar" and $filerules->{type} eq "STR") {
            # Allow indirection to a file
            my $filename = $self->_find_config($filerules->{value}); # Resolved filename
            ( carp "Unable to find file rules file" and return undef ) unless defined $filename;

            # Read the file
            my $file;                # File handle
            unless (open $file, $filename) {
                carp "Unable to open filerules file $filename: $!";
                return undef;
            }
            my @contents = <$file>;
            close $file;
            $filerules = $parser->parse( join '', @contents);
        } elsif ($filerules->{class} eq "metadata") {
            $filerules = $filerules->{value};
        } else {
            carp "Can't interpret FILERULES in camera configuration.\n";
            return undef;
        }
        $self->{filerules} = $filerules;
    }

    my $file = _mdLookupMDwithRedirect($filerules, $name); # The file of interest
    unless (defined $file) {
        carp "Can't find $name within FILERULES in camera configuration.\n";
        return undef;
    }

    my $filename = metadataLookupStr($file, "FILENAME.RULE"); # File name
    unless (defined $filename) {
        carp "Can't find FILENAME.RULE for $name within FILERULES in camera configuration.\n";
        return undef;
    }

    $filename =~ s/\{OUTPUT\}/$output/;
    if ($filename =~ /\{CHIP\.NAME\}/) {
        my $chip = $component; # Chip name (only one component is allowed, will be used for all elements)
        $chip = "fpa" unless defined $chip;
        $filename =~ s/\{CHIP\.NAME\}/$chip/;
    }
    if ($filename =~ /\{CELL\.NAME\}/) {
        my $cell = $component; # Cell name 
        $cell = "chip" unless defined $cell;
        $filename =~ s/\{CELL\.NAME\}/$cell/;
    }
    if ($filename =~ /\{FILE\.ID\}/) {
        my $fileID = $component; # FILE.ID
        $fileID = 0 unless defined $fileID;
	my $fileN = sprintf ("%03d", $fileID);
        $filename =~ s/\{FILE\.ID\}/$fileN/;
    }

    return $filename;
}

sub magic_filename
{
    my $file = shift;
    my $prefix = shift;
    my $dirname = dirname($file);
    my $base = basename($file);

    return "$dirname/$prefix$base";
}

sub destreaked_filename
{
    my $self  = shift;
    my $file  = shift;

    return magic_filename($file, "SR_");
}
sub recovery_filename
{
    my $self  = shift;
    my $file  = shift;

    return magic_filename($file, "REC_");
}

# Return an EXTNAME From the EXTNAME.RULE table in the camera configuration
sub extname_rule
{
    my $self = shift;                # Configuration object
    my $name = shift;                # Name of the EXTNAME to build
    my $component = shift;        # For replacing {CHIP.NAME}

    unless (defined $self and defined $name) {
        carp "Programming error";
        return undef;
    }

    my $camera = $self->{camera}; # Camera configuration
    unless (defined $camera) {
        carp "Camera has not yet been defined.\n";
        return undef;
    }

    my $extname_rules = metadataLookupMD($camera, 'EXTNAME.RULES');        # File rules
    unless (defined $extname_rules) {
        carp "Can't find EXTNAME.RULES in camera configuration.\n";
        return undef;
    }

    my $extname = metadataLookupStr($extname_rules, $name); # File name
    unless (defined $extname) {
        carp "Can't find $name within EXTNAME.RULES in camera configuration.\n";
        return undef;
    }

    if ($extname =~ /\{CHIP\.NAME\}/) {
        my $chip = $component;  # Chip name
        $chip = "fpa" unless defined $chip;
        $extname =~ s/\{CHIP\.NAME\}/$chip/;
    }

    return $extname;
}

sub prepare_output
{
    my $self = shift;             # Configuration object
    my $name = shift;             # Name of the filerule
    my $outroot = shift;          # For replacing {OUTPUT}
    my $component = shift;        # For replacing {CHIP.NAME} and {CELL.NAME}
    my $delete_existing = shift;  # if file exists delete it
    my $r_error = shift;          # reference to error code

    die "prepare_output: invalid number of arguments" if !defined $r_error;

    $$r_error = 0;

    my $output = $self->filename($name, $outroot, $component);
    if (!$output) {
        # error message emitted by filename should be sufficient
        $$r_error = $PS_EXIT_CONFIG_ERROR;
        return undef;
    }

    my $scheme = file_scheme($output);
    if (!$scheme or ($scheme ne 'neb')) {
        # non-nebulous file we're done
        if ($delete_existing and $self->file_exists($output)) {
            if (!$self->file_delete($output)) {
                carp "failed to delete $output";
                $$r_error = $PS_EXIT_SYS_ERROR;
                $output = undef;
            }
        }
        return $output;
    }

    my $neb = $self->nebulous;
    if ($neb->storage_object_exists($output)) {
        if ($delete_existing) {
            $$r_error = $self->_kill_nebulous_file($output);
            if ($$r_error) {
                $output = undef;
            }
        } else {
            # Make sure that there is only 1 instance.
            eval {
                $neb->there_can_be_only_one($output);
            };
            if ($@) {
                carp "nebulous there_can_be_only_one() failed for $output";
                $output = undef;
                $$r_error = $PS_EXIT_SYS_ERROR;
            }
        }
    }
    return $output;
}

# _kill_nebulous_file: reliably get a nebulous file out of way.
# First move it to the trash then attempt to delete it. No failure if delete fails.
# The file is in the trash.
# Assumes that file is a nebulous file with a storage object that exists
# Users should call kill_file()
# Returns 0 on success otherwise and a PS_EXIT error code on failure
sub _kill_nebulous_file {
    my $self = shift;
    my $filename = shift;
    my $neb = $self->nebulous;

    # avoid dead instances by moving the file before deleting it.
    # append current time to form new name
    my $todelete;
    eval {
        # parse the key so that we can compute the new name
        require Nebulous::Key;
    };
    if ($@) {
        carp "Can't find Nebulous::Key";
        return $PS_EXIT_CONFIG_ERROR;
    }
    eval {
        # parse the key so that we can compute the new name
        my $neb_key = Nebulous::Key::parse_neb_key($filename);
        die "parse_neb_key failed" if !$neb_key;
        my $path = $neb_key->path;
        die "neb_key has no path" if !$path;
        my $ticks = time();
        $todelete = "ipp_trash/$path.$ticks";
        $neb->move($filename, $todelete);
    };
    if ($@) {
        carp "nebulous move failed for $filename";
        return $PS_EXIT_SYS_ERROR;
    }
    if ($todelete) {
        eval {
            $neb->delete($todelete);
        };
        if ($@) {
            carp "nebulous delete for $todelete failed. Ignoring.\n";
        }
    }
    return 0;
}

# user level interface to kill_file reliably get a file out of way.
#
# If a nebulous file, move it to the trash then attempt to delete it. 
# if non nebulous file just delete it.
# Returns 0 on success otherwise and a PS_EXIT error code on failure

sub kill_file {
    my $self = shift;
    my $filename = shift;
    my $neb = $self->nebulous;

    my $scheme = file_scheme($filename);
    if (!$scheme or ($scheme ne 'neb')) {
        if ($self->file_exists($filename)) {
            if (!$self->file_delete($filename)) {
                carp "failed to delete $filename";
                return $PS_EXIT_SYS_ERROR;
            }
        }
    } elsif ($neb->storage_object_exists($filename)) {
        $self->_kill_nebulous_file($filename);
    }
    return 0;
}

# Cause a nebulous file to be replicated setting the user.copies value
sub replicate_file
{
    my $self = shift;
    my $file = shift;
    my $copies = shift;

    my $scheme = file_scheme($file);
    if (!$scheme or ($scheme ne 'neb')) {
        carp "cannot replicate non-neulous file: $file";
        return 0;
    }

    if (!$copies) {
        $copies = 2;
    }

    my $neb = $self->nebulous;
    eval {
        $neb->setxattr($file, 'user.copies', $copies, 'replace');
    };
    if ($@) {
        carp "failed to set user.copies for $file";
        return 0;
    }

    # XXX: if copies > 2 should we make more replicants here ?
    eval {
        $neb->replicate($file);
    };
    if ($@) {
        carp "failed to replicate $file";
        return 0;
    }
    return 1;
}


# Return catdir for tessellation, from TESSELLATIONS within the site configuration
sub tessellation_catdir
{
    my $self = shift;                # Configuration object
    my $tess_id = shift;        # Tessellation identifier

    unless (defined $self and defined $self->{_siteConfig} and defined $tess_id) {
        carp "Programming error";
        return undef;
    }

    my $tessellations = metadataLookupMD($self->{_siteConfig}, 'TESSELLATIONS'); # Tessellations
    unless (defined $tessellations) {
        carp "Can't find TESSELLATIONS in site configuration.\n";
        return undef;
    }

    my $catdir = metadataLookupStr($tessellations, $tess_id);
    unless (defined $catdir) {
        print "Can't find tessellation identifier $tess_id in TESSELLATIONS in site configuration, assuming it is a filename.\n";
        return $tess_id;
    }

    ### Because DVO doesn't use psModules, it doesn't understand Nebulous --- check
    my $scheme = file_scheme($catdir); # The scheme, e.g., file, path, neb
    if (defined $scheme and lc($scheme) eq 'neb') {
        carp "Tessellation $tess_id refers to a Nebulous path: $catdir\n";
        return undef;
    }

    return $catdir
}


# Return catdir for the dvo db, from DVO.CATDIRS within the site configuration
sub dvo_catdir
{
    my $self = shift;        # Configuration object
    my $dvodb = shift;        # DVO db identifier

    unless (defined $self and defined $self->{_siteConfig} and defined $dvodb) {
        carp "Programming error";
        return undef;
    }

    my $catdirs = metadataLookupMD($self->{_siteConfig}, 'DVO.CATDIRS'); # Tessellations
    unless (defined $catdirs) {
        carp "Can't find DVO.CATDIRS in site configuration.\n";
        return undef;
    }

    my $catdir = metadataLookupStr($catdirs, $dvodb);
    unless (defined $catdir) {
        print "Can't find dvodb identifier $dvodb in DVO.CATDIR in site configuration, assuming filename\n";
        return $dvodb;
    }

    ### Because DVO doesn't use psModules, it doesn't understand Nebulous --- check
    my $scheme = file_scheme($catdir); # The scheme, e.g., file, path, neb
    if (defined $scheme and lc($scheme) eq 'neb') {
        carp "DVO catdir $dvodb refers to a Nebulous path: $catdir\n";
        return undef;
    }

    return $catdir
}

# Return catdir for the psastro reference, from PSASTRO.CATDIRS within the site configuration
sub psastro_catdir
{
    my $self = shift;        # Configuration object
    my $dvodb = shift;        # DVO db identifier

    unless (defined $self and defined $self->{_siteConfig} and defined $dvodb) {
        carp "Programming error";
        return undef;
    }

    my $catdirs = metadataLookupMD($self->{_siteConfig}, 'PSASTRO.CATDIRS'); # Tessellations
    unless (defined $catdirs) {
        carp "Can't find PSASTRO.CATDIRS in site configuration.\n";
        return undef;
    }

    my $catdir = metadataLookupStr($catdirs, $dvodb);
    unless (defined $catdir) {
        print "Can't find dvodb identifier $dvodb in PSASTRO.CATDIR in site configuration, assuming filename.\n";
        return $dvodb;
    }

    ### Because DVO doesn't use psModules, it doesn't understand Nebulous --- check
    my $scheme = file_scheme($catdir); # The scheme, e.g., file, path, neb
    if (defined $scheme and lc($scheme) eq 'neb') {
        carp "PSASTRO catdir $dvodb refers to a Nebulous path: $catdir\n";
        return undef;
    }

    return $catdir
}

# Return the DVO.CAMERADIR in the camera configuration
sub dvo_cameradir
{
    my $self = shift;                # Configuration object

    unless (defined $self) {
        carp "Programming error";
        return undef;
    }

    my $camera = $self->{camera}; # Camera configuration
    unless (defined $camera) {
        carp "Camera has not yet been defined.\n";
        return undef;
    }

    my $dir = metadataLookupStr($camera, "DVO.CAMERADIR"); # Directory of interest
    unless (defined $dir) {
        carp "Can't find DVO.CAMERADIR in camera configuration.\n";
        return undef;
    }

    return $dir;
}

# Given a reduction class name (and a previously defined camera) and a
# symbolic name for the recipe, return the actual name of the recipe.
sub reduction
{
    my $self = shift;                # Configuration object
    my $reduction = shift;        # Reduction class
    my $name = shift;                # Symbolic name of recipe

    unless (defined $self and defined $reduction and defined $name) {
        carp "Programming error --- inputs undefined";
        return undef;
    }

    my $reductionClasses;
    if (defined $self->{reductionClasses}) {
        $reductionClasses = $self->{reductionClasses};
    } else  {
        # load and save the reductionClasses table

        # need to have a valid camear
        my $camera = $self->{camera}; # Camera configuration
        unless (defined $camera) {
            carp "Camera has not yet been defined.\n";
            return undef;
        }

        $reductionClasses = metadataLookup($camera, 'REDUCTION');        # File rules

        unless (defined $reductionClasses) {
            carp "Can't find REDUCTION list in camera configuration.\n";
            return undef;
        }

        if ($reductionClasses->{class} eq "scalar" and $reductionClasses->{type} eq "STR") {
            # Allow indirection to a file
            my $filename = $self->_find_config($reductionClasses->{value}); # Resolved filename
            ( carp "Unable to find reduction classes file" and return undef ) unless defined $filename;
            # Read the file
            my $file;                # File handle
            unless (open $file, $filename) {
                carp "Unable to open reductionClasses file $filename: $!";
                return undef;
            }
            my @contents = <$file>;
            close $file;
            $reductionClasses = $parser->parse( join '', @contents);
        } elsif ($reductionClasses->{class} eq "metadata") {
            $reductionClasses = $reductionClasses->{value};
        } else {
            carp "Can't interpret REDUCTIONS in camera configuration.\n";
            return undef;
        }
        $self->{reductionClasses} = $reductionClasses;
    }

    my $class = metadataLookupMD($reductionClasses, $reduction) or # Class of interest
        ( carp "Can't find $reduction in REDUCTION in camera configuration.\n" and return undef );

    my $actual = metadataLookupStr($class, $name) or # The actual recipe name of interest
        (carp "Can't find $name in $class in REDUCTION in camera configuration.\n" and return undef );

    return $actual;
}

sub skycell_file
{
    my $self = shift;                # Configuration object
    my $tess_id = shift;        # Tessellation identifier
    my $skycell_id = shift;        # Skycell identifier
    my $outname = shift;        # Output name
    my $verbose = shift;        # Verbose?

    unless (defined $self and defined $tess_id and defined $skycell_id and defined $outname) {
        carp "Input parameters not defined";
        return 0;
    }

    my $dvoImageExtract = can_run('dvoImageExtract') or ( carp "Can't find dvoImageExtract" and return undef );

    my $tess_dir = $self->tessellation_catdir( $tess_id ); # Tessellation catdir for DVO
    unless (defined $tess_dir) {
        carp "Can't get list of tessellations.";
        return undef;
    }
    $tess_dir = $self->convert_filename_absolute( $tess_dir ) or return undef;

    my $outnameResolved = $self->file_resolve( $outname, 0 );

    # check if file actually exists
    if (defined $outnameResolved and -f $outnameResolved) {
        if (-s $outnameResolved) {
            # Exists and has non-zero size
            return 1;
        }
        if (!$self->file_delete($outname)) {
            carp "Can't delete zero-sized skycell";
            return 0;
        }
        $outnameResolved = undef;
    }

    unless (defined $outnameResolved) {
        $outnameResolved = $self->file_create( $outname ) or return undef; # Resolved filename, for Nebulous
    }

    my $command = "$dvoImageExtract -D CATDIR $tess_dir $skycell_id -o $outnameResolved";
    my ( $success, $error_code, $full_buf, $stdout_buf, $stderr_buf ) =
        run(command => $command, verbose => $verbose);
    ( carp "Unable to perform dvoImageExtract for $tess_id $skycell_id\n" and return undef ) unless ($success and $self->file_exists( $outname ));

    return 1;
}

# Interpolate environment variables in a directory (or colon-delimited list of directories)
sub _interpolate_env
{
    my $dir = shift;                # Directory of interest

    return undef unless defined $dir;

    while ($dir =~ /\$\{?(\w*)[\}\/:]?/) {
        my $name = $1;                # Environment variable name
        my $value = $ENV{$name}; # Environment variable value
        unless (defined $value) {
            carp "Unable to find environment variable $name";
            return undef;
        }
        $dir =~ s/\$\{?$name\}?/$value\//;
    }
    return $dir;
}

sub metadataLookup
{
    my $mdc = shift;                # Metadata config to look up
    my $name = shift;                # Name of item to look up

    unless (defined $mdc and defined $name) {
        carp "Programming error";
        return undef;
    }

    # Iterate through the array of hashes
    foreach my $item (@$mdc) {
        if ($item->{name} eq $name) {
            return $item;
        }
    }

    carp "Unable to find $name within metadata.\n";
    return undef;
}


# Lookup the metadata, checking the type is STR
sub metadataLookupStr
{
    my $item = metadataLookup(@_);
    return undef if not defined $item;
    my $name = shift;                # Name of item
    carp "$name within metadata is type $item->{type} not STR.\n" unless $item->{type} eq "STR";
    return $item->{value};
}

# Lookup the metadata, checking the type is S32
sub metadataLookupS32
{
    my $item = metadataLookup(@_);
    return undef if not defined $item;
    my $name = shift;                # Name of item
    carp "$name within metadata is type $item->{type} not S32.\n" unless $item->{type} eq "S32";
    return $item->{value};
}

# Lookup the metadata, checking the type is F32
sub metadataLookupF32
{
    my $item = metadataLookup(@_);
    return undef if not defined $item;
    my $name = shift;                # Name of item
    carp "$name within metadata is type $item->{type} not F32.\n" unless $item->{type} eq "F32";
    return $item->{value};
}

# Lookup the metadata, checking the type is MD
sub metadataLookupMD
{
    my $item = metadataLookup(@_);
    return undef if not defined $item;
    my $name = shift;                # Name of item
    carp "$name within metadata is not of type METADATA.\n" unless $item->{class} eq "metadata";
    return $item->{value};
}

# Lookup the metadata, checking the type is BOOL
sub metadataLookupBool
{
    my $item = metadataLookup(@_);
    return undef if not defined $item;
    my $name = shift;                # Name of item
    carp "$name within metadata is type $item->{type} not BOOL.\n" unless $item->{type} eq "BOOL";
    return $item->{value};
}

# Lookup the metadata, checking the type is MD; may be redirection
sub _mdLookupMDwithRedirect
{
    my $mdc = shift;                # Metadata config to look up
    my $name = shift;                # Name of item to look ip

    unless (defined $mdc and defined $name) {
        carp "Programming error";
        return undef;
    }

    # Iterate through the array of hashes
    foreach my $item (@$mdc) {
        if ($item->{name} eq $name) {
            if ($item->{class} eq "metadata") {
                return $item->{value};
            }
            if ($item->{class} eq "scalar" and $item->{type} eq "STR") {
                # Redirect
                return _mdLookupMDwithRedirect($mdc, $item->{value});
            }
            carp "$name within metadata is not of type METADATA or STR.\n";
            return undef;
        }
    }

    carp "Unable to find $name within metadata.\n";
    return undef;
}


1;

__END__;

