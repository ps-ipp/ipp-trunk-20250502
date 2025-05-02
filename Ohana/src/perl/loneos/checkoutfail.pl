#!/usr/bin/perl

sub by_filename {
    @value1 = split (" ", $a);
    @value2 = split (" ", $b);
    $value1[0] cmp $value2[0]; 
}
    
# we read from in and write to out those files with 1st 0 in the mode column
sub parse_outlist {
    $in = $_[0];
    $mode = $_[1];
    
# load inlist
    open (INLIST, $in);
    @inarray = ();
    while ($line = <INLIST>) {
	chop ($line);
	@inarray = (@inarray,$line);
    }
    $Nin = @inarray;
    close (INLIST);
    @inarray = sort by_filename @inarray;

    @outarray = ();
    for ($k = 0; $k < $Nin; $k++) {
	@pvalues = split (" ", $inarray[$k]);
	$more = 1;
	while ($more) {
	    @nvalues = split (" ", $inarray[$k+1]);
	    if ($nvalues[0] cmp $pvalues[0]) {
		$more = 0;
	    } else {
		$pvalues[1] |= $nvalues[1];
		$pvalues[2] |= $nvalues[2];
		$pvalues[3] |= $nvalues[3];
		$pvalues[4] |= $nvalues[4];
		$pvalues[5] |= $nvalues[5];
		$k ++;
	    }
	}
	# check the earlier status values: if failed earlier, skip
	$skip = 0;
	for ($m = 1; $m < $mode; $m++) { if (!$pvalues[$m]) { $skip = 1; } }
	if (! $skip && !$pvalues[$mode]) {
	    $name = $pvalues[0];
	    $name =~ s/$topdir//;
	    $name =~ s/://;
	    @outarray = (@outarray,$name);
	}
    }
    $Nout = @outarray;
    if ($Nout) {
	for ($k = 0; $k < $Nout; $k++) {
	    print STDERR "$mode: $outarray[$k]\n";
	}
    }
    $Nout;
}

$outlist = $ARGV[0];

parse_outlist ($outlist,5);
