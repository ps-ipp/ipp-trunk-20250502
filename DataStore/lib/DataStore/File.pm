# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: File.pm,v 1.18 2008-05-07 03:05:03 jhoblitt Exp $

package DataStore::File;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.03';

use base qw( DataStore::Record );

use Carp qw( carp );
use DataStore::Response;
use DataStore::Utils qw( $STD_FIELD $BYTE_FIELD $MD5_FIELD %KNOWN_FILE_TYPES );
use Digest::MD5::File qw( file_md5_hex );
use File::Basename qw( basename );
use File::Temp ();
use File::stat;
use IPC::Cmd 0.36 qw( can_run run );
use Params::Validate qw( validate SCALAR ARRAYREF UNDEF );

use vars qw( @BASE_FIELDS );

@BASE_FIELDS = qw( fileid bytes md5sum type extra compressed );

__PACKAGE__->mk_accessors(@BASE_FIELDS);

=pod

=head1 NAME

DataStore::File - represents a DataStore File

=head1 SYNOPSIS

    use DataStore::file;

    my $dsf = DateStore::File->new(
        uri         => 'http://example.org/foo',
        fileid      => '12buckelyourshoe',
        bytes       => 12345,
        md5sum      => 'fe6a2b6564c0d4cfb3bbf1db813824ba',
        type        => 'foo',
    );

    my $uri     = $dsf->uri;
    my fileid   = $dsf->fileid;
    my $bytes   = $dsf-bytes;
    my $md5sum  = $dsf->md5sum;
    my $type    = $dsf-type;
    my DataStore::Response $response = $dsf->request( filename => "/foo/bar" );

=head1 DESCRIPTION

This class I<isa> L<DataStore::Record>

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

=head3 Constructors

=over 4

=item * C<new()>

Basic constructor.

    my $dsf = DateStore::File->new(
        uri         => 'http://example.org/foo',
        fileid      => '12buckelyourshoe',
        bytes       => 12345,
        md5sum      => 'fe6a2b6564c0d4cfb3bbf1db813824ba',
        type        => 'foo',
    );

Accepts a mandatory hash and returns a L<DataStore::Product> object.

=over 4

=item * uri

A valid I<HTTP> URI as a string.  I<No trailing slash is allowed.>

=item * fileid

The FIle ID as a string.

This key is optional.

=item * bytes

The size of the file as an integer number of bytes.

This key is optional.

=item * md5sum

The hex encoded md5 checksum of the file.

This key is optional.

=item * type

The type of file as a string.

This key is optional.

=back

=cut

sub new
{
    my $class = shift;

    # validates uri, doesn't check other params
    my $self = $class->SUPER::new(@_);

    validate(@_,
        {
            uri     => {
                type        => SCALAR,
                callbacks   => {
                    'is valid uri filename' => sub { $_[0] !~ m|/$| },
                },
            },
            fileid  => {
                type        => SCALAR,
                regex       => $STD_FIELD,
                optional    => 1,
            },
            bytes   => {
                type        => SCALAR,
                regex       => $BYTE_FIELD,
                optional    => 1,
            },
            md5sum  => {
                type        => SCALAR,
                regex       => $MD5_FIELD,
                optional    => 1,
            },
            type    => {
                type        => SCALAR,
                callbacks   => {
                    'is valid type' =>
                        sub { exists $KNOWN_FILE_TYPES{$_[0]} },
                },
                optional    => 1,
            },
            extra   => {
                type        => SCALAR | ARRAYREF,
                optional    => 1,
            },
            compressed => {
                type        => SCALAR | UNDEF,
                optional    => 1,
            },
        },
    );

    return $self;
}

=back

=head3 Object Methods

=over 4

=item * C<uri()>

Basic accessor.

=item * C<fileid()>

Basic accessor.

=item * C<bytes()>

Basic accessor.

=item * C<md5sum()>

Basic accessor.

=item * C<type()>

Basic accessor.

=item * C<request()>

Retrieves and processes the File listing pointed to by the L<uri> of this
object.

XXX writing the file into memory or to a filehandle will be implemented upon
request.

    my $response = $dsf->request( filename => "/foo/bar" );

Accepts a mandatory hash and returns a L<DataStore::Response> object.

=over 4

=item * filename

The filename/path to save the file to disk as.  It is the user's responsibility
to make sure that this is a valid filename/path.

=back

=cut

sub request 
{
    my $self = shift;

    my %p = validate(@_,
        {
            filename    => {
                type        => SCALAR,
                regex       => qr/\S+/, # string with at least 1 non WS char
            },
            ua_args     => {
                optional    => 1,
            },
            no_proxy     => {
                type        => SCALAR,
                optional    => 1,
            },
        },
    );

    my $verbose = 0;

    # make request
    my $ua;
    if (defined $p{ua_args}) {
        $ua = LWP::UserAgent->new(%{$p{ua_args}});
    } else {
        $ua = LWP::UserAgent->new;
    }

    # load proxy environment variables (if any)
    if (!$p{no_proxy}) {
        $ua->env_proxy;
    }

    my $request = HTTP::Request->new(GET => $self->uri);
    my $filename = $p{filename};
    my $response = $ua->request($request, $filename);

    if ($response->is_success) {
        my $funpack;
        if ($self->compressed) {
            $funpack = can_run('funpack')
                or warn "can't find funpack -- unable to compute checksum";
        }

        my $chk_file = $filename;
        # so that the tmp file stays in scope until we're done processing
        my $tmp;
        if ($funpack) {
            $tmp = File::Temp->new(
                DIR         => '/tmp',
                TEMPLATE    => basename($filename) . '.XXXXXXXX',
                SUFFIX      => '.tmp',
                UNLINK      => 1,
            );

            my $unpacked_path = $tmp->filename;

            my $command = "$funpack -S -C $filename > $unpacked_path";
            my ($success, $status, $full_buf, $stdout_buf, $stderr_buf)
                = run(command => $command, verbose => $verbose);

            unless ($success) {
                die "funpack returned exit status $status\n";
            }

            $chk_file = $unpacked_path;
        }

        # check size
        if (defined $self->bytes) {
            my $size = stat($chk_file)->size;
            if (! ($self->bytes == $size)) {
                unlink $filename;
                carp "uri: ", $self->uri,
                     " - expected size: ", $self->bytes,
                     " got: ", $size;
                # set the filename to undef to indicate an error
                $filename = undef;
            } else {
                carp "uri: ", $self->uri,
                     " - expected size: ", $self->bytes,
                     " got: ", $size
                if $verbose;
            }
        }

        if (defined $filename and defined $self->md5sum) {
            my $md5 = file_md5_hex($chk_file);
            if (! ($self->md5sum eq $md5)) {
                unlink $filename;
                carp "uri: ", $self->uri,
                     " - expected md5: ", $self->md5sum,
                     " got: ", $md5;
                # set the filename to undef to indicate an error
                $filename = undef;
            } else {
                carp "uri: ", $self->uri,
                     " - expected md5: ", $self->md5sum,
                     " got: ", $md5
                if $verbose;
            }
        }
    }

    # return DS::Response object
    my $dsr = DataStore::Response->new(
        is_success  => $response->is_success,
        code        => $response->code,
        status_line => $response->status_line,
        data        => $filename,
        request     => $self,
    );
}

=back

=head1 SEE ALSO

L<DataStore::Response>

=cut

1;

__END__
