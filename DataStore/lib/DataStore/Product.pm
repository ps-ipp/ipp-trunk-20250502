# Copyright (C) 2006-2008  Joshua Hoblitt
#
# $Id: Product.pm,v 1.11 2008-05-12 22:04:53 jhoblitt Exp $

package DataStore::Product;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.02';

use base qw( DataStore::Record );

use Carp qw( carp );
use DataStore::FileSet::Parser;
use DataStore::Record;
use DataStore::Response;
use DataStore::Utils qw( $STD_FIELD $TIME_FIELD %KNOWN_PRODUCT_TYPES );
use LWP::UserAgent;
use Params::Validate qw( validate SCALAR);

use vars qw( @BASE_FIELDS );

@BASE_FIELDS = qw( product last_fileset last_datetime type desc );

__PACKAGE__->mk_ro_accessors(@BASE_FIELDS);

=pod

=head1 NAME

DataStore::Product - represents a DataStore Product ID

=head1 SYNOPSIS

    use DataStore::Product;

    my $dsp = DataStore::Product->new(
        uri             => 'http://example.com/productid/',
        last_fileset    => 'foobar',
    );

    my $uri     = $dsp->uri;
    my $fileset = $dsp_fileset;
    my DataStore::Response $response = $dsp->request;

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

    my $dsp = DataStore::Product->new(
        uri             => 'http://example.com/productid/',
        last_fileset    => 'foobar',
    );

Accepts a mandatory hash and returns a L<DataStore::Product> object.

=over 4

=item * uri

A valid I<HTTP> URI as a string.  I<A trailing slash is required.>

=item * last_fileset

The last FileSet ID that was requested as a string.

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
            uri             => {
                type        => SCALAR,
                callbacks   => {
                    'uri ends with /index.txt'
                        => sub { $_[0] =~ m|/index.txt$| },
                }
            },
            product         => {
                type        => SCALAR,
                optional    => 1,
            },
            last_fileset    => { 
                type        => SCALAR,
                callbacks   => {
                    'is valid fileset ID' => sub { $_[0] =~ $STD_FIELD },
                },
                optional    => 1,
            },
            last_datetime    => {
                type        => SCALAR,
                regex       => $TIME_FIELD,
                optional    => 1,
            },
            type        => {
                type        => SCALAR,
                callbacks   => {
                    'is valid type' =>
                        sub { exists $KNOWN_PRODUCT_TYPES{$_[0]} },
                },
                optional    => 1,
            },
            desc        => {
                 type       => SCALAR,
                 optional   => 1,
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

=item * C<last_fileset()>

Basic accessor.

=item * C<request()>

Retrieves and processes the FileSet listing pointed to by the L<uri> of this
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

    my $request;
    if ($self->last_fileset) {
        $request = HTTP::Request->new(
            GET => $self->uri . "?" . $self->last_fileset,
        );
    } else {
        $request = HTTP::Request->new(GET => $self->uri);
    }
    my $response = $ua->request($request);

    my $data;

    if ($response->is_success) {
        # parse document
        my $parser = DataStore::FileSet::Parser->new(base_uri => $self->uri);

        $data = $parser->parse($response->content);
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
