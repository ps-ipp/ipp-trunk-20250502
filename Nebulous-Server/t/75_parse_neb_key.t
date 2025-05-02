#!/usr/bin/perl

# Copryight (C) 2004-2005  Joshua Hoblitt
#
# $Id: 75_parse_neb_key.t,v 1.5 2008-09-09 02:18:47 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More;

plan tests => 106;

use lib qw( ./t ./lib );

use Nebulous::Key qw( parse_neb_key );
use Test::Nebulous;

# neb://<volume name>/...
# neb:path...
# neb:/path... (same as neb:path, leading '/' is stripped)
# neb:///path... (same as neb:path)

# key
{
    my $key = parse_neb_key('foo/bar/baz/quix');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('/foo/bar/baz/quix');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('//foo/bar/baz/quix');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('///foo/bar/baz/quix');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('foo////bar/baz/quix');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('foo/bar/baz/quix/');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

# key w/ volume argument
{
    my $key = parse_neb_key('foo/bar/baz/quix', 'boing');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, 'boing', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('foo/bar/baz/quix', '~boing');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, 'boing', 'volume name');
    is($key->hard_volume, 1, 'soft volume name');
}

# URI w/ volume name
{
    my $key = parse_neb_key('neb://foo/bar/baz/quix');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://foo//bar/baz/quix');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://foo///bar/baz/quix');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://foo/bar///baz/quix');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://foo/bar/baz/quix/');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

# URI w/ hard volume name
{
    my $key = parse_neb_key('neb://~foo/bar/baz/quix/');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, 1, 'soft volume name');
}

# URI w/ volume name and volume argument
{
    my $key = parse_neb_key('neb://foo/bar/baz/quix', 'boing');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'boing', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://foo/bar/baz/quix', '~boing');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'boing', 'volume name');
    is($key->hard_volume, 1, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://foo/bar/baz/quix', undef);

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

# URI w/ ~any volume name
{
    my $key = parse_neb_key('neb://~any/bar/baz/quix/');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

# URI w/ any volume argument
{
    my $key = parse_neb_key('neb://boing/bar/baz/quix/', 'any');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://boing/bar/baz/quix/', '~any');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

# URI w/ hard volume name
{
    my $key = parse_neb_key('neb://~foo/bar/baz/quix/');

    is($key->path, 'bar/baz/quix', 'path');
    is($key->volume, 'foo', 'volume name');
    is($key->hard_volume, 1, 'soft volume name');
}

# URI w/o volume name

{
    my $key = parse_neb_key('neb:/foo/bar/baz/quix');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb:///foo/bar/baz/quix');

    is($key->volume, undef, 'volume name');
    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb://///foo/bar/baz/quix');

    is($key->path, 'foo/bar/baz/quix', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

# root volume references
{
    my $key = parse_neb_key('neb:///');

    is($key->path, '', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb:///.');

    is($key->path, '', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('neb:///..');

    is($key->path, '', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('/');

    is($key->path, '', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('.');

    is($key->path, '', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

{
    my $key = parse_neb_key('..');

    is($key->path, '', 'path');
    is($key->volume, undef, 'volume name');
    is($key->hard_volume, undef, 'soft volume name');
}

# stringification

{
    my $txt = 'neb:///bar/baz';
    my $key = parse_neb_key($txt);

    is("$key", $txt, "stringified");
}

{
    my $txt = 'neb://foo.0/bar/baz';
    my $key = parse_neb_key($txt);

    is("$key", $txt, "stringified");
}

{
    my $txt = 'neb://~foo.0/bar/baz';
    my $key = parse_neb_key($txt);

    is("$key", $txt, "stringified");
}

# key w/ whitespace
eval {
    parse_neb_key('/ foo/bar/baz/quix');
};
like( $@, qr/may not contain whitespace/, "whitespace" );

eval {
    parse_neb_key(' /foo/bar/baz/quix');
};
like( $@, qr/may not contain whitespace/, "whitespace" );

eval {
    parse_neb_key('/foo/bar/baz/quix ');
};
like( $@, qr/may not contain whitespace/, "whitespace" );

# URI w/ whitespace

eval {
    parse_neb_key('neb ://foo/bar/baz/quix');
};
like( $@, qr/may not contain whitespace/, "whitespace" );

eval {
    parse_neb_key('neb:// foo/bar/baz/quix');
};
like( $@, qr/may not contain whitespace/, "whitespace" );

eval {
    parse_neb_key(' neb://foo/bar/baz/quix');
};
like( $@, qr/may not contain whitespace/, "whitespace" );

eval {
    parse_neb_key('neb://foo/bar/baz/quix ');
};
like( $@, qr/may not contain whitespace/, "whitespace" );

# URI w/o volume requires leading slash
eval {
    my $key = parse_neb_key('neb:foo/bar/baz/quix');
};
like( $@, qr/requires a leading slash/, "leading slash" );

# URI w/ volume but w/o path
eval {
    my $key = parse_neb_key('neb://foo');
};
like( $@, qr/requires a path/, "no path" );

# params
eval {
    my $key = parse_neb_key();
};
like( $@, qr/key param is not optional/, "no params" );

eval {
    my $key = parse_neb_key(undef);
};
like( $@, qr/key param is not optional/, "key is undef" );

eval {
    my $key = parse_neb_key(undef, 'bar');
};
like( $@, qr/key param is not optional/, "key is undef" );

eval {
    my $key = parse_neb_key('foo', 'bar', 'foo');
};
like( $@, qr/too many params/, "too many params" );
