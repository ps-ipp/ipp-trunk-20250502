#!/usr/bin/perl

# Copryight (C) 2007  Joshua Hoblitt
#
# $Id: 14_server_xattr.t,v 1.8 2008-07-09 23:32:35 jhoblitt Exp $

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 44;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::Nebulous;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

# 1 key / xattr

Test::Nebulous->setup;

{
    my $uri = $neb->create_object('foo');

    ok($neb->setxattr_object('foo', 'user.bar', 'baz', 'create'), 'set object xattr');
    {
        my $xattrs = $neb->listxattr_object('foo');
        is(scalar @$xattrs, 1, 'number of xattrs');
        is(@$xattrs[0], 'user.bar', 'xattr name');
    }

    my $value = $neb->getxattr_object('foo', 'user.bar');
    is($value, 'baz', 'xattr value');

    ok($neb->removexattr_object('foo', 'user.bar'), "remove object xattr");
    {
        my $xattrs = $neb->listxattr_object('foo');
        is(scalar @$xattrs, 0, 'number of xattrs');
    }
}

# multiple xattrs

Test::Nebulous->setup;

{
    my $uri = $neb->create_object('foo');

    ok($neb->setxattr_object('foo', 'user.bar', 'baz', 'create'), 'set object xattr');
    ok($neb->setxattr_object('foo', 'user.bonk', 'quix', 'create'), 'set object xattr');
    
    {
        my $xattrs = $neb->listxattr_object('foo');
        is(scalar @$xattrs, 2, 'number of xattrs');
        is(@$xattrs[0], 'user.bar', 'xattr name');
        is(@$xattrs[1], 'user.bonk', 'xattr name');
    }

    my $value = $neb->getxattr_object('foo', 'user.bar');
    is($value, 'baz', 'xattr value');
    $value = $neb->getxattr_object('foo', 'user.bonk');
    is($value, 'quix', 'xattr value');

    ok($neb->removexattr_object('foo', 'user.bar'), "remove object xattr");
    ok($neb->removexattr_object('foo', 'user.bonk'), "remove object xattr");
    {
        my $xattrs = $neb->listxattr_object('foo');
        is(scalar @$xattrs, 0, 'number of xattrs');
    }
}

# replace xattrs

Test::Nebulous->setup;

{
    my $uri = $neb->create_object('foo');

    ok($neb->setxattr_object('foo', 'user.bar', 'baz', 'create'), 'set object xattr');
    ok($neb->setxattr_object('foo', 'user.bar', 'quix', 'replace'), 're-set object xattr');
    
    {
        my $xattrs = $neb->listxattr_object('foo');
        is(scalar @$xattrs, 1, 'number of xattrs');
        is(@$xattrs[0], 'user.bar', 'xattr name');
    }

    my $value = $neb->getxattr_object('foo', 'user.bar');
    is($value, 'quix', 'xattr value');

    ok($neb->removexattr_object('foo', 'user.bar'), "remove object xattr");
    {
        my $xattrs = $neb->listxattr_object('foo');
        is(scalar @$xattrs, 0, 'number of xattrs');
    }
}

# setxattr_object

Test::Nebulous->setup;

eval {
    $neb->setxattr_object('foo', 'user.bar', 'baz', 'create');
};
like($@, qr/is valid object key/, "create xattr on non-existant key");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->setxattr_object('foo', 'luser.bar', 'baz', 'create');
};
like($@, qr/xattr is in user. namespace/, "user. namspace");

Test::Nebulous->setup;

eval {
    $neb->setxattr_object('foo', 'user.bar', 'baz', 'replace');
};
like($@, qr/is valid object key/, "replace xattr on non-existant key");

Test::Nebulous->setup;

eval {
    $neb->setxattr_object();
};
like($@, qr/4 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->setxattr_object('foo', 'user.bar');
};
like($@, qr/4 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->setxattr_object('foo', 'user.bar', 'baz');
};
like($@, qr/4 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->setxattr_object('foo', 'user.bar', 'baz', 'create', 'quix');
};
like($@, qr/4 were expected/, "too many params");

# getxattr_object

Test::Nebulous->setup;

eval {
    $neb->getxattr_object('foo', 'bar');
};
like($@, qr/is valid object key/, "get xattr from non-existant nebulous key");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->getxattr_object('foo', 'luser.bar');
};
like($@, qr/xattr is in user. namespace/, "user. namespace");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');
    $neb->getxattr_object('foo', 'user.bar');
};
like($@, qr|xattr neb:///foo:user.bar does not exist|, "get xattr from non-existant xattr key");

Test::Nebulous->setup;

eval {
    $neb->getxattr_object();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->getxattr_object('foo');
};
like($@, qr/2 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->getxattr_object('foo', 'user.bar', 'baz');
};
like($@, qr/2 were expected/, "too many params");

# listxattr_object

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->listxattr_object();
};
like($@, qr/1 was expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->listxattr_object('foo', 'bar');
};
like($@, qr/1 was expected/, "too many params");

# removexattr_object

Test::Nebulous->setup;

eval {
    $neb->removexattr_object('foo', 'user.bar');
};
like($@, qr/is valid object key/, "remove xattr from non-existant nebulous key");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->getxattr_object('foo', 'luser.bar');
};
like($@, qr/xattr is in user. namespace/, "user. namespace");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');
    $neb->removexattr_object('foo', 'user.bar');
};
like($@, qr|xattr neb:///foo:user.bar does not exist|,
    "remove xattr from non-existant xattr key");

Test::Nebulous->setup;

eval {
    $neb->removexattr_object();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->removexattr_object('foo');
};
like($@, qr/2 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create_object('foo');

    $neb->removexattr_object('foo', 'user.bar', 'baz');
};
like($@, qr/2 were expected/, "too many params");

Test::Nebulous->cleanup;
