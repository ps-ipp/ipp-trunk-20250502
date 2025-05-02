#!/usr/bin/perl

# Copryight (C) 2009  Joshua Hoblitt

use strict;
use warnings FATAL => qw( all );

use Test::More tests => 13;

use lib qw( ./t ./lib );

use Nebulous::Server;
use Test::URI;
use Test::Nebulous;
use URI;
use File::stat;

my $neb = Nebulous::Server->new(
    dsn         => $NEB_DB,
    dbuser      => $NEB_USER,
    dbpasswd    => $NEB_PASS,
);

Test::Nebulous->setup;

# object with one instance
{
    my $key = "foo";
    my $uri = $neb->create_object($key);

    my $path = URI->new($uri)->path;
    my $st1 = stat($path);

    my $mode = $neb->chmod_object($key, 0440);
    my $st2 = stat($path);

    is($st2->mode &07777, 0440, "chmod() single instance");
    is($mode, 0440, "returned mode");
    is($neb->getxattr_object($key, 'user.mode'), 0440, "xattr user.mode");

    # reset to 600 so the user can delete the file and /tmp entry
    chmod(0600, $path);
}

Test::Nebulous->setup;

# object with two instances
{
    my $key = "foo";
    my $uri1 = $neb->create_object($key);
    my $uri2 = $neb->replicate_object($key);

    my $mode = $neb->chmod_object($key, 0440);

    my $path1 = URI->new($uri1)->path;
    my $path2 = URI->new($uri2)->path;

    my $st1 = stat($path1);
    my $st2 = stat($path2);

    is($st1->mode &07777, 0440, "chmod() first instance");
    is($st2->mode &07777, 0440, "chmod() second instance");
    is($mode, 0440, "returned mode");
    is($neb->getxattr_object($key, 'user.mode'), 0440, "xattr user.mode");

    # reset to 600 so the user can delete the file and /tmp entry
    chmod (0600, $path1);
    chmod (0600, $path2);
}

Test::Nebulous->setup;

eval {
    $neb->chmod_object("foo", 0644);
};
like($@, qr/valid object/, "object does not exist");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->chmod_object("foo", 0640);
};
like($@, qr/allowable mode/, "mode is not 0400");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->chmod_object("foo", 99999);
};
like($@, qr/allowable mode/, "bad mode");

Test::Nebulous->setup;

eval {
    $neb->chmod_object();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->chmod_object("foo");
};
like($@, qr/2 were expected/, "one param");

Test::Nebulous->setup;

eval {
    $neb->create_object("foo");

    $neb->chmod_object("foo", 0440, 2);
};
like($@, qr/2 were expected/, "three params");

Test::Nebulous->cleanup;
