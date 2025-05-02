#!/usr/bin/env perl

use tap;

&tap::plan_tests (5);

# --- simple fits file ---
$status = system ("fhead simple.fits > tmp.hdr");
&tap::ok (!$status, "run fhead simple.fits");

$status = system ("diff tmp.hdr simple.hdr 2>&1 >/dev/null");
&tap::ok (!$status, "fhead simple.fits");

# --- mef fits file (phu) ---
$status = system ("fhead mef.fits > tmp.hdr");
&tap::ok (!$status, "run fhead mef.fits");

$status = system ("diff tmp.hdr mef.phu.hdr 2>&1 >/dev/null");
&tap::ok (!$status, "fhead mef.fits");

# --- mef fits file (ext) ---
$status = system ("fhead -x 0 mef.fits >tmp.hdr");
&tap::ok (!$status, "run fhead -x 0 mef.fits");

$status = system ("diff tmp.hdr mef.0.hdr 2>&1 >/dev/null");
&tap::ok (!$status, "fhead -x 0 mef.fits");

# --- mef fits file list ---
$status = system ("ftable -list mef.fits > tmp.hdr");
&tap::ok (!$status, "run ftable -lit mef.fits");

$status = system ("diff tmp.hdr mef.list 2>&1 >/dev/null");
&tap::ok (!$status, "fhead -x 0 mef.fits");

&tap::done_tests();
exit 2;
