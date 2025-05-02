#!/usr/bin/perl

# Copryight (C) 2009  Joshua Hoblitt

use strict;
use warnings FATAL => qw( all );

use Apache::Test qw( -withtestmore );

plan tests => 13;

use lib qw( ./t ./lib );

use Nebulous::Client;
use Nebulous::Util qw( :standard );
use Test::URI;
use Test::Nebulous;
use URI;
use File::stat;

my $hostport = Apache::Test->config->{ 'hostport' };

my $neb = Nebulous::Client->new(
    proxy => "http://$hostport/nebulous",
);

Test::Nebulous->setup;

# object with one instance
{
    my $key = "foo";
    my $uri = $neb->create($key);

    my $path = URI->new($uri)->path;
    my $st1 = stat($path);

    my $mode = $neb->chmod($key, 0440);
    my $st2 = stat($path);

    is($st2->mode &07777, 0440, "chmod() single instance");
    is($mode, 0440, "returned mode");
    is($neb->getxattr($key, 'user.mode'), 0440, "xattr user.mode");

    # reset to 600 so the user can delete the file and /tmp entry
    chmod(0600, $path);
}

Test::Nebulous->setup;

# object with two instances
{
    my $key = "foo";
    my $uri1 = $neb->create($key);
    my $uri2 = $neb->replicate($key);

    my $mode = $neb->chmod($key, 0440);

    my $path1 = URI->new($uri1)->path;
    my $path2 = URI->new($uri2)->path;

    my $st1 = stat($path1);
    my $st2 = stat($path2);

    is($st1->mode &07777, 0440, "chmod() first instance");
    is($st2->mode &07777, 0440, "chmod() second instance");
    is($mode, 0440, "returned mode");
    is($neb->getxattr($key, 'user.mode'), 0440, "xattr user.mode");

    # reset to 600 so the user can delete the file and /tmp entry
    chmod(0600, $path1);
    chmod(0600, $path2);
}

Test::Nebulous->setup;

{
    my $mode = $neb->chmod("foo", 0644);

    is($mode, undef, "object does not exist");
}

Test::Nebulous->setup;

{
    $neb->create("foo");

    my $mode = $neb->chmod("foo", 0640);

    is($mode, undef, "mode is not 0440");
}

Test::Nebulous->setup;

{
    $neb->create("foo");

    my $mode = $neb->chmod("foo", 999);

    is($mode, undef, "bad mode");
}

Test::Nebulous->setup;

eval {
    $neb->chmod();
};
like($@, qr/2 were expected/, "no params");

Test::Nebulous->setup;

eval {
    $neb->create("foo");

    $neb->chmod("foo");
};
like($@, qr/2 were expected/, "one param");

Test::Nebulous->setup;

eval {
    $neb->create("foo");

    $neb->chmod("foo", 0440, 2);
};
like($@, qr/2 were expected/, "three params");

Test::Nebulous->cleanup;
