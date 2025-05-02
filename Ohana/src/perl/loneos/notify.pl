#!/usr/bin/perl

$target= "gene\@astro.washington.edu";
$targetmachine = "astro.washington.edu";

$last = "archive." . $ARGV[0];
$next = $ARGV[1];
$N = $ARGV[2];

# system ("echo \"done with $last, starting $next ($N)\" | mail $targetname\@$targetmachine");
system ("echo \"done with $last, starting $next ($N)\" | mail $target");

