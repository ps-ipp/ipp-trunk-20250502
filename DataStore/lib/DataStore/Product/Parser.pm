# Adapted from FileSet/Parser.pm
#
# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: Parser.pm,v 1.2 2007-09-26 00:13:29 jhoblitt Exp $

package DataStore::Product::Parser;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.01';

use base qw( Class::Accessor::Fast );

use Carp qw( carp );
use Data::Validate::URI qw( is_uri );
use DataStore::Product;
use DataStore::Utils qw( $STD_FIELD $TIME_FIELD %KNOWN_PRODUCT_TYPES );
use Params::Validate qw( validate validate_pos SCALAR);

__PACKAGE__->mk_ro_accessors(qw(base_uri));

=pod

=head1 NAME

DataStore::Product::Parser - parses the DataStore 'Product' list format

=head1 SYNOPSIS

    use DataStore::Product::Parser;

    my $parser = DataStore::Product::Parser->new(
        base_uri => 'http://example.org/index.txt',
    );

    my @data = $parser->parse($str);
        or
    my $data = $parser->parse($str);

=head1 DESCRIPTION

This class parses a root DataStore listing into an array of L<DataStore::Product> objects.

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

=head3 Constructors

=over 4

=item * C<new()>

Basic constructor.

    my $parser = DataStore::Product::Parser->new(
        base_uri => 'http://example.org/index.txt',
    );

Accepts an optional hash and returns a L<DataStore::Product::Parser> object.

=over 4

=item * base_uri

The base of the URI to set in the created L<DataStore::Product> objects.

This key is optional and defaults to C<http://example.org/>.

=back

=cut

sub new
{
    my $class = shift;

    my %p = validate(@_,
        {
            base_uri => {
                type        => SCALAR,
                callbacks   => {
                    'is valid http uri' =>
                        sub { is_uri($_[0]) and $_[0] =~ /^http:/ },
                    'uri ends with /index.txt' =>
                        sub { $_[0] =~ m|/index.txt$| },

                },
                default =>  'http://example.org/index.txt',
            },
        },
    );

    my $self = bless {}, ref $class || $class;

    $p{base_uri} =~ qr|(.*?/)index.txt|;
    $self->{base_uri} = $1;

    return $self;
}

=back

=head3 Object Methods

=over 4

=item * C<base_uri()>

Basic accessor.

=item * C<parse()>

Accepts a string and returns a list in list context or an arrayref is scalar
context.  An empty list or undef is returned if the string contained no rows.

=cut

sub parse
{
    my $self = shift;

    my ($doc) = validate_pos(@_,
        {
            type    => SCALAR,
        }
    );

    my @data;
    my $lineno = 1;
LINE: foreach my $line (split /\n/, $doc) {
        # blank lines
        next LINE if $line =~ /^\s*$/;

        # comment lines
        next LINE if $line =~ /^\s*\#/;

        my @fields = split /\|/, $line;

        # at least fileset, datatime, and type fields are required
        if (scalar @fields < 5) {
            carp "line $lineno: not enough fields: $line";
            next LINE;
        }

        foreach my $field (@fields) {
            # fields are not allowed to contain #
            if ($field =~ /\#/) {
                carp "line $lineno: field $field: contains #: $line";
                next LINE;
            }

            # strip leading and trailing whitespace
            $field =~ s/^\s+//;
            $field =~ s/\s+$//;
        }

        my ($product, $last_fileset, $last_datetime, $type, $desc) = @fields;

        # validate format of fileset
        unless ($last_fileset =~ $STD_FIELD) {
            carp "line $lineno: field $last_fileset:"
               . " does not conform to $STD_FIELD: $line";
            next LINE;
        }

        # validate format of datetime
        unless ($last_datetime =~ $TIME_FIELD) {
            carp "line $lineno: field $last_datetime:"
               . " does not conform to $TIME_FIELD";
            next LINE;
        }

        unless (exists $KNOWN_PRODUCT_TYPES{$type}) {
            carp "line $lineno: type $type unknown: $line";
            next LINE;
        }

        # fifo
        push @data, DataStore::Product->new({
            product       => $product,
            last_fileset  => $last_fileset,
            last_datetime => $last_datetime,
            type          => $type,
            desc          => $desc,
            uri           => $self->base_uri . $product . '/index.txt',
        });
    } continue {
        $lineno++;
    }

    return unless @data;
    return wantarray ? @data : \@data;
}

=back

=head1 SEE ALSO

L<DataStore::Product>, L<DataStore::FileSet::Parser>

=cut

1;

__END__
