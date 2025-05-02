# Copyright (C) 2006  Joshua Hoblitt
#
# $Id: Parser.pm,v 1.17 2007-09-26 00:09:52 jhoblitt Exp $

package DataStore::FileSet::Parser;

use strict;
use warnings;

use vars qw($VERSION);
$VERSION = '0.02';

use base qw( Class::Accessor::Fast );

use Carp qw( carp );
use Data::Validate::URI qw( is_uri );
use DataStore::FileSet;
use DataStore::Utils qw( $STD_FIELD $TIME_FIELD %KNOWN_FILESET_TYPES );
use Params::Validate qw( validate validate_pos SCALAR);

__PACKAGE__->mk_ro_accessors(qw(base_uri));

=pod

=head1 NAME

DataStore::FileSet::Parser - parses the DataStore 'FileSet' list format

=head1 SYNOPSIS

    use DataStore::FileSet::Parser;

    my $parser = DataStore::FileSet::Parser->new(
        base_uri => 'http://example.org/index.txt',
    );

    my @data = $parser->parse($str);
        or
    my $data = $parser->parse($str);

=head1 DESCRIPTION

This class parses a DataStore listing of I<FileSet>s into an array of L<DataStore::FileSet> objects.

=head1 USAGE

=head2 Import Parameters

This module accepts no arguments to it's C<import> method and exports no
I<symbols>.

=head2 Methods

=head3 Constructors

=over 4

=item * C<new()>

Basic constructor.

    my $parser = DataStore::FileSet::Parser->new(
        base_uri => 'http://example.org/index.txt',
    );

Accepts an optional hash and returns a L<DataStore::FileSet::Parser> object.

=over 4

=item * base_uri

The base of the URI to set in the created L<DataStore::FileSet> objects.

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
        if (scalar @fields < 3) {
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

        my ($fileset, $datetime, $type) = @fields;

        # validate format of fileset and type
        foreach my $field ($fileset, $type) {
            unless ($field =~ $STD_FIELD) {
                carp "line $lineno: field $field:"
                   . " does not conform to $STD_FIELD: $line";
                next LINE;
            }
        }

        # validate format of datetime
        unless ($datetime =~ $TIME_FIELD) {
            carp "line $lineno: field $datetime:"
               . " does not conform to $TIME_FIELD";
            next LINE;
        }

        unless (exists $KNOWN_FILESET_TYPES{$type}) {
            carp "line $lineno: type $type unknown: $line";
            next LINE;
        }

        my @extra = @fields[3 .. $#fields] if $#fields >= 3;

        # fifo
        push @data, DataStore::FileSet->new({
            fileset     => $fileset,
            datetime    => $datetime,
            type        => $type,
            extra       => \@extra,
            uri         => $self->base_uri . $fileset . '/index.txt',
        });
    } continue {
        $lineno++;
    }

    return unless @data;
    return wantarray ? @data : \@data;
}

=back

=head1 SEE ALSO

L<DataStore::FileSet>, L<DataStore::File:Parser>

=cut

1;

__END__
