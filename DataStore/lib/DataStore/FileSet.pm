# Copyright (C) 2006-2008  Joshua Hoblitt
#
# $Id: FileSet.pm,v 1.15 2008-05-12 22:04:53 jhoblitt Exp $

package DataStore::FileSet;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.04';

use base qw( DataStore::Record );

use Carp qw( carp );
use DataStore::File::Parser;
use DataStore::Response;
use DataStore::Utils qw( $STD_FIELD $TIME_FIELD %KNOWN_FILESET_TYPES );
use LWP::UserAgent;
use Params::Validate qw( validate SCALAR ARRAYREF );

use vars qw( @BASE_FIELDS );

@BASE_FIELDS = qw( fileset datetime type extra );

__PACKAGE__->mk_accessors(@BASE_FIELDS);

=pod

=head1 NAME

DataStore::FileSet - represents a DataStore FileSet 

=head1 SYNOPSIS

    use DataStore::FileSet;

    my $dsfs = DateStore::FileSet->new(
        uri         => 'http://example.org/',
        fileset     => '12buckelyourshoe',
        datetime    => '2042-01-01T00:00:00Z',
        type        => 'foo',
    );

    my $uri         = $dsfs->uri;
    my $fileset     = $dsfs->fileset;
    my $datatime    = $dsfs->datetime;
    my $type        = $dsfs->type;
    my DataStore::Response $response = $dsfs->request;

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

    my $dsfs = DateStore::FileSet->new(
        uri         => 'http://example.org/',
        fileset     => '12buckelyourshoe',
        datetime    => '2042-01-01T00:00:00Z',
        type        => 'foo',
    );

Accepts a mandatory hash and returns a L<DataStore::FileSet> object.

=over 4

=item * uri

A valid I<HTTP> URI as a string.  I<A trailing slash is required.>

=item * fileset

The FileSet ID as a string.

This key is optional.

=item * datetime

The time and date as a string.

This key is optional.

=item * type

The I<type> of record as a string.

This key is optional.

=back

=cut

sub new
{
    # turn off param validation as it has been causing problems
    local $Params::Validate::NO_VALIDATION = 1;

    my $class = shift;

    # validates uri, doesn't check other params
    my $self = $class->SUPER::new(@_);

    validate(@_,
        {
            uri         => {
                type        => SCALAR,
                callbacks   => {
                    'uri ends with /index.txt'
                        => sub { $_[0] =~ m|/index.txt$| },
                }
            },
            fileset     => {
                type        => SCALAR,
                regex       => $STD_FIELD,
                optional    => 1,
            },
            datetime    => {
                type        => SCALAR,
                regex       => $TIME_FIELD,
                optional    => 1,
            },
            type        => {
                type        => SCALAR,
                callbacks   => {
                    'is valid type' => 
                        sub { exists $KNOWN_FILESET_TYPES{$_[0]} },
                },
                optional    => 1,
            },
            extra       => {
                type        => SCALAR | ARRAYREF,
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

=item * C<fileset()>

Basic accessor.

=item * C<datetime()>

Basic accessor.

=item * C<type()>

Basic accessor.

=item * C<request()>

Retrieves and processes the File listing pointed to by the L<uri> of this
object.

Accepts no parameters and returns a L<DataStore::Response> object.

=cut

sub request 
{
    my $self = shift;

    my %p = validate(@_,
        {
            ua_args     => {
                optional    => 1,
            },
            no_proxy     => {
                type        => SCALAR,
                optional    => 1,
            },
        },
    );

    # make request
    my $ua;
    if (defined $p{ua_args}) {
        $ua = LWP::UserAgent->new(%{$p{ua_args}});
    } else {
        $ua = LWP::UserAgent->new;
    }

    if (!$p{no_proxy}) {
        # load proxy environment variables (if any)
        $ua->env_proxy;
    }

    my $request = HTTP::Request->new(GET => $self->uri);
    my $response = $ua->request($request);

    my $data;

    if ($response->is_success) {
        # parse document
        my $parser = DataStore::File::Parser->new(base_uri => $self->uri);

        eval {
            $data = $parser->parse($response->content);
        };
        if ($@) {
            carp "error parsing DataStore File format: $@";
        }
    }

    # return DS::Response object
    my $dsr = DataStore::Response->new(
        is_success  => $response->is_success,
        code        => $response->code,
        status_line => $response->status_line,
        data        => $data,
        request     => $self,
    );
}

=back

=head1 SEE ALSO

L<DataStore::Response>

=cut

1;

__END__
