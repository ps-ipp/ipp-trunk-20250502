#!/usr/bin/env perl
$DEBUG = 0;

# this program takes the pmSourceIO_CMF.in.c template file and generates the .c version based on the given I/O format made

if (@ARGV != 3) { die "USAGE: mksource (template) (cmfmode) (output)\n"; }

$template = $ARGV[0];
$cmfmode = $ARGV[1];
$output = $ARGV[2];

if (! -f $template) { die "missing template file $template\n"; }

# validate the cmfmode

# keep the series (V1,SV1,DV1) separate)
%cmfmodes_v = ("PS1_V1", 1,
	       "PS1_V2", 2,
	       "PS1_V3", 3,
	       "PS1_V4", 4,
	       "PS1_V5", 5,
    );
%cmfmodes_dv = ("PS1_DV1", 1,
		"PS1_DV2", 2,
		"PS1_DV3", 3,
		"PS1_DV4", 4,
		"PS1_DV5", 5,
    );
%cmfmodes_sv = ("PS1_SV1", 1,
		"PS1_SV2", 2,
		"PS1_SV3", 3,
		"PS1_SV4", 4,
    );

open (FILE, "$template") || die "failed to open template $template\n";
@list = <FILE>;
close (FILE);

open (FILE, ">$output");

# operations we can perform:
# @CMFMODE@ : replace with $cmfmode
# @ALL@ : remove and keep the rest of the line
# @=MODE@ : remove and keep if cmfmode == MODE
# @>MODE@ : remove and keep if cmfmode > MODE
# @<MODE@ : remove and keep if cmfmode > MODE

# XXX need to add features: split @foo,bar,baz@ by commas
# treat each chunk as a rule
# add the following options:
# !MODE -- exclude the given entry (defaults to all, or is ALL required?)
# * and ? regexp 

# some examples:
# @ALL,!PS1_V1@
# @PS1_DV?@
# @PS1_V?,!PS1_V1@

foreach $line (@list) {

    # replace @CMFMODE@ wherever it appears
    $line =~ s|\@CMFMODE\@|$cmfmode|g;
    
    # print and continue if we do not match @RULES@
    unless ($line =~ m|\@.*\@|) { 
	print "no rule\n" if $DEBUG;
	print FILE $line;
	next;
    }

    # grab the rules and the rest of the line
    ($prefix,$rules,$content) = ($line =~ m|(.*)\@(.*)\@\s*(.*)|);
    
    # split the rules into separate items
    @rules = split (",", $rules);

    $keepLine = 0;
    # does $cmfmode match any of the rules?
    foreach $rule (@rules) {
	print "rule: $rule\n" if $DEBUG;

	# special rule "ALL"
	if ($rule eq "ALL") { 
	    print "ALL match\n" if $DEBUG;
	    $keepLine = 1; 
	    next; 
	} # look for other rules (esp !foo)

	# pure match
	if ($cmfmode eq $rule) { 
	    print "simple match\n" if $DEBUG;
	    $keepLine = 1; 
	    next; 
	} # skip to end?

	# NOT match
	if ($rule =~ m|^!|) {
	    print "NOT rule: $rule\n" if $DEBUG;
	    ($realrule) = ($rule =~ m|^!(.*)|);
	    if ($cmfmode eq $realrule) { $keepLine = 0; } # skip to end?
	    next; 
	}

	# simple regexp: foo*
	if ($rule =~ m|\*$|) {
	    print "regexp * rule: $rule\n" if $DEBUG;
	    ($realrule) = ($rule =~ m|(.*)\*$|);
	    if ($cmfmode =~ m|$realrule|) { $keepLine = 1; } # skip to end?
	    next; 
	}

	# simple regexp: foo?
	if ($rule =~ m|\?$|) {
	    print "regexp ? rule: $rule\n" if $DEBUG;
	    ($realrule) = ($rule =~ m|(.*)\?$|);
	    if (substr($cmfmode,0,-1) eq $realrule) { $keepLine = 1; } # skip to end?
	    next; 
	}

	# rule: =FOO
	if ($rule =~ m|^=|) {
	    print "= rule: $rule\n" if $DEBUG;
	    $realrule = substr($cmfmode,1);
	    if ($cmfmode eq $realrule) { $keepLine = 1; } # skip to end?
	    next; 
	}

	# only apply the < > <= >= rules if cmfmode is one of cmfmodes
	# rule: >=FOO
	if ($rule =~ m|^>=|) {
	    print ">= rule: $rule\n" if $DEBUG;
	    # find the cmfmode series (v, dv, sv)
	    $realrule = substr($rule,2);
	    %series = &cmf_series ($realrule);
	    if (! %series) { next; }
	    if ($series{$cmfmode} == 0) { next; }
	    $thisLevel = $series{$realrule};
	    $myLevel = $series{$cmfmode};
	    print "levels: $cmfmode, $realrule, $myLevel, $thisLevel\n" if $DEBUG;
	    if ($myLevel >= $thisLevel) { $keepLine = 1; }
	    next; 
	}

	# rule: >FOO
	if ($rule =~ m|^>|) {
	    print "> rule: $rule\n" if $DEBUG;
	    # find the cmfmode series (v, dv, sv)
	    $realrule = substr($rule,1);
	    %series = &cmf_series ($realrule);
	    if (! %series) { next; }
	    if ($series{$cmfmode} == 0) { next; }
	    $thisLevel = $series{$realrule};
	    $myLevel = $series{$cmfmode};
	    print "levels: $cmfmode, $realrule, $myLevel, $thisLevel\n" if $DEBUG;
	    if ($myLevel > $thisLevel) { $keepLine = 1; }
	    next; 
	}

	# rule: <=FOO
	if ($rule =~ m|^<=|) {
	    print "<= rule: $rule\n" if $DEBUG;
	    # find the cmfmode series (v, dv, sv)
	    $realrule = substr($rule,2);
	    %series = &cmf_series ($realrule);
	    if (! %series) { next; }
	    if ($series{$cmfmode} == 0) { next; }
	    $thisLevel = $series{$realrule};
	    $myLevel = $series{$cmfmode};
	    print "levels: $cmfmode, $realrule, $myLevel, $thisLevel\n" if $DEBUG;
	    if ($myLevel <= $thisLevel) { $keepLine = 1; }
	    next; 
	}

	# rule: <FOO
	if ($rule =~ m|^<|) {
	    print "< rule: $rule\n" if $DEBUG;
	    # find the cmfmode series (v, dv, sv)
	    $realrule = substr($rule,1);
	    %series = &cmf_series ($realrule);
	    if (! %series) { next; }
	    if ($series{$cmfmode} == 0) { next; }
	    $thisLevel = $series{$realrule};
	    $myLevel = $series{$cmfmode};
	    print "levels: $cmfmode, $realrule, $myLevel, $thisLevel\n" if $DEBUG;
	    if ($myLevel < $thisLevel) { $keepLine = 1; }
	    next; 
	}

    }
    print "line: $line\n" if $DEBUG;

    if ($keepLine) {
	print FILE "$prefix $content\n";
    }
}

close (FILE);

exit 0;

sub cmf_series {

    my ($rule) = $_[0];

    if ($cmfmodes_v{$rule})  { return %cmfmodes_v;  }
    if ($cmfmodes_sv{$rule}) { return %cmfmodes_sv; }
    if ($cmfmodes_dv{$rule}) { return %cmfmodes_dv; }
    return 0;
}
