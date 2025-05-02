#!/usr/bin/env perl

use tap;

&tap::plan_tests (5);

# --- simple fits file ---
$line = `echo simple.fits | fields FLTVAL`;
&tap::ok (!$?, "run fhead simple.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "simple.fits", "field FLTVAL");
&tap::ok ($word[1] == 3.1415, "field FLTVAL");

# --- simple fits file ---
$line = `echo simple.fits | fields FLTVAL INTVAL`;
&tap::ok (!$?, "run fhead simple.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "simple.fits", "field FLTVAL INTVAL");
&tap::ok ($word[1] == 3.1415, "field FLTVAL INTVAL");
&tap::ok ($word[2] == 2, "field FLTVAL INTVAL");


# --- mef fits file (phu) ---
$line = `echo mef.fits | fields FLTVAL`;
&tap::ok (!$?, "run fhead mef.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "mef.fits", "field FLTVAL");
&tap::ok ($word[1] == 3.1415, "field FLTVAL");

# --- mef fits file ---
$line = `echo mef.fits | fields FLTVAL INTVAL`;
&tap::ok (!$?, "run fhead mef.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "mef.fits", "field FLTVAL INTVAL");
&tap::ok ($word[1] == 3.1415, "field FLTVAL INTVAL");
&tap::ok ($word[2] == 2, "field FLTVAL INTVAL");


# --- mef fits file (ext) ---
$line = `echo mef.fits | fields -x 0 FLTVAL`;
&tap::ok (!$?, "run fhead mef.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "mef.fits", "field FLTVAL");
&tap::ok ($word[1] == 5.1415, "field FLTVAL");

# --- mef fits file ---
$line = `echo mef.fits | fields -x 0 FLTVAL INTVAL`;
&tap::ok (!$?, "run fhead mef.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "mef.fits", "field FLTVAL INTVAL");
&tap::ok ($word[1] == 5.1415, "field FLTVAL INTVAL");
&tap::ok ($word[2] == 4, "field FLTVAL INTVAL");


# --- mef fits file (extname) ---
$line = `echo mef.fits | fields -n EXTENSION FLTVAL`;
&tap::ok (!$?, "run fhead mef.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "mef.fits[EXTENSION]", "field FLTVAL");
&tap::ok ($word[1] == 5.1415, "field FLTVAL");

# --- mef fits file ---
$line = `echo mef.fits | fields -n EXTENSION FLTVAL INTVAL`;
&tap::ok (!$?, "run fhead mef.fits");

@word = split (" ", $line);
&tap::ok ($word[0] eq "mef.fits[EXTENSION]", "field FLTVAL INTVAL");
&tap::ok ($word[1] == 5.1415, "field FLTVAL INTVAL");
&tap::ok ($word[2] == 4, "field FLTVAL INTVAL");

&tap::done_tests();
exit 2;
