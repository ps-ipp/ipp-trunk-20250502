#!/usr/bin/perl

# Copryight (C) 2007-2009  Joshua Hoblitt

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );
plan tests => 44;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::Nebulous;

my $hostport = Apache::Test->config->{ 'hostport' };

my $neb = Nebulous::Client->new(
    proxy => "http://$hostport/nebulous",
);

# setxattr
Test::Nebulous->setup;
{
    my $uri = $neb->create('foo');

    ok($neb->setxattr('foo', 'user.bar', 'baz', 'create'), 'set object xattr');
    {
        my $xattrs = $neb->listxattr('foo');
        is(scalar @$xattrs, 1, 'number of xattrs');
        is(@$xattrs[0], 'user.bar', 'xattr name');
    }

    my $value = $neb->getxattr('foo', 'user.bar');
    is($value, 'baz', 'xattr value');

    ok($neb->removexattr('foo', 'user.bar'), "remove object xattr");
    {
        my $xattrs = $neb->listxattr('foo');
        is(scalar @$xattrs, 0, 'number of xattrs');
    }
}

# multiple xattrs
Test::Nebulous->setup;
{
    my $uri = $neb->create('foo');

    ok($neb->setxattr('foo', 'user.bar', 'baz', 'create'), 'set object xattr');
    ok($neb->setxattr('foo', 'user.bonk', 'quix', 'create'), 'set object xattr');
    
    {
        my $xattrs = $neb->listxattr('foo');
        is(scalar @$xattrs, 2, 'number of xattrs');
        is(@$xattrs[0], 'user.bar', 'xattr name');
        is(@$xattrs[1], 'user.bonk', 'xattr name');
    }

    my $value = $neb->getxattr('foo', 'user.bar');
    is($value, 'baz', 'xattr value');
    $value = $neb->getxattr('foo', 'user.bonk');
    is($value, 'quix', 'xattr value');

    ok($neb->removexattr('foo', 'user.bar'), "remove object xattr");
    ok($neb->removexattr('foo', 'user.bonk'), "remove object xattr");
    {
        my $xattrs = $neb->listxattr('foo');
        is(scalar @$xattrs, 0, 'number of xattrs');
    }
}

# replace xattrs

Test::Nebulous->setup;

{
    my $uri = $neb->create('foo');

    ok($neb->setxattr('foo', 'user.bar', 'baz', 'create'), 'set object xattr');
    ok($neb->setxattr('foo', 'user.bar', 'quix', 'replace'), 're-set object xattr');
    
    {
        my $xattrs = $neb->listxattr('foo');
        is(scalar @$xattrs, 1, 'number of xattrs');
        is(@$xattrs[0], 'user.bar', 'xattr name');
    }

    my $value = $neb->getxattr('foo', 'user.bar');
    is($value, 'quix', 'xattr value');

    ok($neb->removexattr('foo', 'user.bar'), "remove object xattr");
    {
        my $xattrs = $neb->listxattr('foo');
        is(scalar @$xattrs, 0, 'number of xattrs');
    }
}

# setxattr

Test::Nebulous->setup;

eval {
    $neb->setxattr('foo', 'user.bar', 'baz', 'create');
};
like($@, qr/is valid object key/, "create xattr on non-existant key");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->setxattr('foo', 'luser.bar', 'baz', 'create');
};
like($@, qr/xattr is in user. namespace/, "user. namspace");

Test::Nebulous->setup;

eval {
    $neb->setxattr('foo', 'user.bar', 'baz', 'replace');
};
like($@, qr/is valid object key/, "replace xattr on non-existant key");

Test::Nebulous->setup;

eval {
    $neb->setxattr();
};
like($@, qr/4 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->setxattr('foo', 'user.bar');
};
like($@, qr/4 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->setxattr('foo', 'user.bar', 'baz');
};
like($@, qr/4 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->setxattr('foo', 'user.bar', 'baz', 'create', 'quix');
};
like($@, qr/4 were expected/, "too many params");

# getxattr

Test::Nebulous->setup;

eval {
    $neb->getxattr('foo', 'bar');
};
like($@, qr/is valid object key/, "get xattr from non-existant nebulous key");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->getxattr('foo', 'luser.bar');
};
like($@, qr/xattr is in user. namespace/, "user. namespace");

Test::Nebulous->setup;

eval {
    $neb->create('foo');
    $neb->getxattr('foo', 'user.bar');
};
like($@, qr|xattr user.bar does not exist|,
    "get xattr from non-existant xattr key");

Test::Nebulous->setup;

eval {
    $neb->getxattr();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->getxattr('foo');
};
like($@, qr/2 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->getxattr('foo', 'user.bar', 'baz');
};
like($@, qr/2 were expected/, "too many params");

# listxattr

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->listxattr();
};
like($@, qr/1 was expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->listxattr('foo', 'bar');
};
like($@, qr/1 was expected/, "too many params");

# removexattr

Test::Nebulous->setup;

eval {
    $neb->removexattr('foo', 'user.bar');
};
like($@, qr/is valid object key/, "remove xattr from non-existant nebulous key");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->getxattr('foo', 'luser.bar');
};
like($@, qr/xattr is in user. namespace/, "user. namespace");

Test::Nebulous->setup;

eval {
    $neb->create('foo');
    $neb->removexattr('foo', 'user.bar');
};
like($@, qr|xattr user.bar does not exist|,
    "remove xattr from non-existant xattr key");

Test::Nebulous->setup;

eval {
    $neb->removexattr();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->removexattr('foo');
};
like($@, qr/2 were expected/, "too few params");

Test::Nebulous->setup;

eval {
    $neb->create('foo');

    $neb->removexattr('foo', 'user.bar', 'baz');
};
like($@, qr/2 were expected/, "too many params");

Test::Nebulous->cleanup;
