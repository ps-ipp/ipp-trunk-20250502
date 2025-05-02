input tap.dvo

## NOTE: in order to run this test suite, you must have a modern version of fpack / funpack.
## The IPP version included in the build tree (3100-p5) does not have support for gzip2 in fpack.

## Acceptable versions can be obtained by downloading a current copy
## of CFITSIO (https://heasarc.gsfc.nasa.gov/fitsio/).

## To avoid installing the library or funpack/fpack in your psconfig
## tree, only build the code locally and do NOT install.  

## in the cfitsio directory, edit the Makefile with the following changes:

## LDFLAGS_BIN =	-g -O2 -Wl,-rpath,${CFITSIO_LIB} -static
## CFLAGS =	-g -Dg77Fortran -fPIC
## (comment out the old versions)

## run psconfigure, make, make fpack funpack

## after building these (static) binaries, link them into this test directory

macro tests
  test_image_all
  test_table_all_rnd
end

$FPACK = ./fpack
$FUNPACK = ./funpack

macro test_image_all 

  tapPLAN 88

  foreach mode NONE NONE_1 NONE_2 GZIP_1 GZIP_2 RICE_1 RICE_ONE
  # foreach mode GZIP_1
  # foreach mode GZIP_2
  # foreach mode RICE_1
    foreach bitpix 8 16 32 -32 -64
    # foreach bitpix 32
      if (($mode == RICE_1)   && ($bitpix < 0)) continue ; # RICE_1 is for int only
      if (($mode == RICE_ONE) && ($bitpix < 0)) continue ; # RICE_1 is for int only
      tapDIAG 2 "===== bitpix = $bitpix, mode = $mode ====="
      test_image_single $bitpix $mode
    end
  end

  tapDONE
end

macro test_image_single
 if ($0 != 3)
   echo "USAGE: test_image_single (bitpix) (mode)"
   break
 end

 local bitpix cmpmode
 $bitpix = $1
 $cmpmode = $2

 delete -q x z b c d e

 mcreate z 8 8
 set x = xramp(z)
 wd x test.raw.fits -bitpix $bitpix -bzero 0 -bscale 1
 wd x test.cmp.fits -bitpix $bitpix -bzero 0 -bscale 1 -compress-mode $cmpmode
 rd b test.cmp.fits -x 0
 set d = x - b
 stat -q d
 tapOK {abs($MEAN)  < 0.001} "read our cmp $cmpmode $bitpix (MEAN  = $MEAN)"
 tapOK {$SIGMA      < 0.001} "read our cmp $cmpmode $bitpix (SIGMA = $SIGMA)"

 if ($cmpmode == NONE) return
 if ($cmpmode == NONE_1) return
 if ($cmpmode == NONE_2) return

 if ($cmpmode == GZIP_1)
   ## mana does not (yet) handle non-zero quantization 
   exec $FPACK -q 0 -g -S test.raw.fits > test.fpk.fits
 end
 if ($cmpmode == GZIP_2)
   exec $FPACK -q 0 -g2 -S test.raw.fits > test.fpk.fits
 end
 if ($cmpmode == RICE_1)
   exec $FPACK -r -S test.raw.fits > test.fpk.fits
 end
 # echo "load fpack version"
 rd c test.fpk.fits -x 0
 set d = x - c
 stat -q d
 tapOK {abs($MEAN)  < 0.001} "read fpack $cmpmode $bitpix (MEAN  = $MEAN)"
 tapOK {$SIGMA      < 0.001} "read fpack $cmpmode $bitpix (SIGMA = $SIGMA)"

 # if ($cmpmode == RICE_1_0) 
 #   echo "*** funpack fails on RICE_1 ****"
 #   return
 # end

 # echo "run funpack on our version"
 exec $FUNPACK -S test.cmp.fits > test.fun.fits
 rd e test.fun.fits
 set d = x - e
 stat -q d
 tapOK {abs($MEAN)  < 0.001} "funpack our cmp $cmpmode $bitpix (MEAN  = $MEAN)"
 tapOK {$SIGMA      < 0.001} "funpack our cmp $cmpmode $bitpix (SIGMA = $SIGMA)"
end

macro test_table_single_mode
 if ($0 != 2)
   echo "USAGE: test_table_single_mode (mode)"
   break
 end

 local cmpmode
 $cmpmode = $1

 delete -q x y ID

 $NPT = 10
 create x 0 $NPT
 set y = x^2
 create ID 1 {$NPT + 1} -int

 $fields = ID x y 
 $format = IEE

 write -fits test test.raw.tbl $fields -format $format
 write -fits test test.cmp.tbl $fields -format $format -compress-mode $cmpmode

 foreach f $fields
   set $f\_raw = $f
   delete $f
 end

 echo "===== raw ====="
 data test.raw.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat dv
 end
 delete -q $fields

 echo "===== cmp ====="
 data test.cmp.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat dv
 end
end

macro test_table_all_single_column

  tapPLAN 88

  local format cmpmode

  # foreach cmpmode NONE GZIP_1 GZIP_2 RICE_1 RICE_ONE
  foreach cmpmode GZIP_2
    foreach format B I J K E D
      if (($cmpmode == RICE_1)   && ($format == E)) continue ; # RICE_1 is for int only
      if (($cmpmode == RICE_ONE) && ($format == E)) continue ; # RICE_1 is for int only
      if (($cmpmode == RICE_1)   && ($format == D)) continue ; # RICE_1 is for int only
      if (($cmpmode == RICE_ONE) && ($format == D)) continue ; # RICE_1 is for int only
      if (($cmpmode == RICE_1)   && ($format == K)) continue ; # RICE_1 is for int only
      if (($cmpmode == RICE_ONE) && ($format == K)) continue ; # RICE_1 is for int only
      test_table_single_column $cmpmode $format
    end
  end
  tapDONE
end

macro test_table_single_column
 if ($0 != 3)
   echo "USAGE: test_table_single_column (mode) (format)"
   break
 end

 local cmpmode format
 $cmpmode = $1
 $format = $2

 tapDIAG 2 "===== testing $cmpmode $format ====="

 delete -q x dv

 $NPT = 10000

 if ($format == B)
   create x -128 128
 else
   create x 0 $NPT
 end

 $fields = x 

 write -fits test test.raw.tbl $fields -format $format
 write -fits test test.cmp.tbl $fields -format $format -compress-mode $cmpmode

 foreach f $fields
   set $f\_raw = $f
   delete $f
 end

 tapDIAG 2 "===== $cmpmode $format raw ====="
 data test.raw.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat -q dv
   tapOK {abs($MEAN)  < 0.001} "read our raw $cmpmode $format (MEAN  = $MEAN)"
   tapOK {$SIGMA      < 0.001} "read our raw $cmpmode $format (SIGMA = $SIGMA)"
 end
 delete -q $fields

 tapDIAG 2 "===== $cmpmode $format cmp ====="
 data test.cmp.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat -q dv
   tapOK {abs($MEAN)  < 0.001} "read our cmp $cmpmode $format (MEAN  = $MEAN)"
   tapOK {$SIGMA      < 0.001} "read our cmp $cmpmode $format (SIGMA = $SIGMA)"
 end

 if ($cmpmode == NONE) return; # funpack will not recognize NONE
 if ($cmpmode == RICE_ONE) return; # funpack will not recognize RICE_ONE for tables

 tapDIAG 2 "===== $cmpmode $format fpack ====="

 # try to run fpack on ours
 if (1)
   tapEXEC /bin/cp -f test.raw.tbl test.fpk.tbl
   tapEXEC $FPACK -table -F test.fpk.tbl
   
   data test.fpk.tbl
   read -fits test $fields
   
   foreach f $fields
     set dv = $f - $f\_raw
     vstat -q dv
     tapOK {abs($MEAN)  < 0.001} "read fpack $cmpmode $format (MEAN  = $MEAN)"
     tapOK {$SIGMA      < 0.001} "read fpack $cmpmode $format (SIGMA = $SIGMA)"
   end
   delete -q $fields
 end

 break -auto off
 echo $cmpmode $format
 exec ls test.cmp.tbl test.fpk.tbl | fields -x 0 PCOUNT  ZCTYP1
 break -auto on

 tapDIAG 2 "===== $cmpmode $format funpack ====="

 # try to run fpack on ours
 tapEXEC /bin/cp -f test.cmp.tbl test.fun.tbl
 tapEXEC $FUNPACK -F test.fun.tbl

 data test.fun.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat -q dv
   tapOK {abs($MEAN)  < 0.001} "read funpack $cmpmode $format (MEAN  = $MEAN)"
   tapOK {$SIGMA      < 0.001} "read funpack $cmpmode $format (SIGMA = $SIGMA)"
 end
 delete -q $fields
end

macro test_table_all_multi_column

  local format cmpmode

  foreach cmpmode NONE GZIP_1
    foreach format B I J K E D
      test_table_multi_column $cmpmode $format
    end
  end
end

macro test_table_multi_column
 if ($0 != 3)
   echo "USAGE: test_table_multi_column (mode) (format)"
   break
 end

 local cmpmode
 $cmpmode = $1

 delete -q x y z 

 $NPT = 10000
 create x 0 $NPT
 set y = x^2
 set z = 0.01*x

 local format

 $fields = x y z
 $format = $2\$2\$2

 $fields = x y 
 $format = $2\$2

 write -fits test test.raw.tbl $fields -format $format
 write -fits test test.cmp.tbl $fields -format $format -compress-mode $cmpmode

 foreach f $fields
   set $f\_raw = $f
   delete $f
 end

 echo "===== $cmpmode $format raw ====="
 data test.raw.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat dv
 end
 delete -q $fields

 echo "===== $cmpmode $format cmp ====="
 data test.cmp.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat dv
 end
end

macro test_table_all_rnd

  local format cmpmode

  foreach cmpmode NONE NONE_1 NONE_2 GZIP_1 GZIP_2 RICE_1 RICE_ONE
    test_table_rnd $cmpmode
  end
end

macro test_table_rnd
 if ($0 != 2)
   echo "USAGE: test_table_rnd (mode)"
   break
 end

 local cmpmode
 $cmpmode = $1

 tapDIAG 2 "===== testing $cmpmode ====="

 delete -q Achar Ashort Aint Along
 delete -q Ffloat Fdouble

 # lrnd generates rnd int between 0 and 0x7fffff (2^31 - 1 inclusive)

 $NPT = 1000000
 # $NPT = 30
 create ni 0 $NPT -int
 create nf 0 $NPT

 # set Achar  = lrnd(ni) & 0x7f
 set Achar  = int(255.999*(drnd(ni) - 0.5))
 set Ashort = lrnd(ni) & 0x7fff
 set Aint   = lrnd(ni)

 # I cannot represent the full dynamic range of 64 bit with mana vector ints, but 
 # I can with the (double) floating point vector. however, I get the wrong bits
 set Along_dbl  = 9.223371e18 * (2.0*drnd(nf) - 1.0); 

 cast Along = Along_dbl as int64

 set Ffloat_dbl = 3.402822e+38  * (2.0*drnd(nf) - 1.0)
 cast Ffloat = Ffloat_dbl as single
 set Fdouble    = 1.797692e+308 * (2.0*drnd(nf) - 1.0)

 local format

 $fields = Achar Ashort Aint Along Ffloat Fdouble
 $format = BIJKED

 #$fields = Along
 #$format = K

 write -fits test test.raw.tbl $fields -format $format
 write -fits test test.cmp.tbl $fields -format $format -compress-mode $cmpmode

 foreach f $fields
   set $f\_raw = $f
   delete $f
 end

 tapDIAG 2 "===== $cmpmode $format raw ====="
 data test.raw.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat -q dv
   tapOK {abs($MEAN)  < 0.001} "read our raw $f $cmpmode $format (MEAN  = $MEAN)"
   tapOK {$SIGMA      < 0.001} "read our raw $f $cmpmode $format (SIGMA = $SIGMA)"
 end
 delete -q $fields

 tapDIAG 2 "===== $cmpmode $format cmp ====="
 data test.cmp.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat -q dv
   tapOK {abs($MEAN)  < 0.001} "read our cmp $f $cmpmode $format (MEAN  = $MEAN)"
   tapOK {$SIGMA      < 0.001} "read our cmp $f $cmpmode $format (SIGMA = $SIGMA)"
 end
 delete -q $fields

 tapDIAG 2 "===== $cmpmode $format fpack ====="

 # try to run fpack on ours
 if (1)
   tapEXEC /bin/cp -f test.raw.tbl test.fpk.tbl
   tapEXEC $FPACK -table -F test.fpk.tbl
   
   data test.fpk.tbl
   read -fits test $fields
   
   foreach f $fields
     set dv = $f - $f\_raw
     vstat -q dv
     tapOK {abs($MEAN)  < 0.001} "read fpack $f $cmpmode $format (MEAN  = $MEAN)"
     tapOK {$SIGMA      < 0.001} "read fpack $f $cmpmode $format (SIGMA = $SIGMA)"
   end
   delete -q $fields
 end

 break -auto off
 echo $cmpmode $format
 exec ls test.cmp.tbl test.fpk.tbl | fields -x 0 PCOUNT  ZCTYP1 ZCTYP2 ZCTYP3 ZCTYP4 ZCTYP5 ZCTYP6
 break -auto on

 if ($cmpmode == NONE) return; # funpack will not recognize NONE
 if ($cmpmode == NONE_1) return; # funpack will not recognize NONE
 if ($cmpmode == NONE_2) return; # funpack will not recognize NONE
 if ($cmpmode == RICE_ONE) return; # funpack will not recognize RICE_ONE for tables

 tapDIAG 2 "===== $cmpmode $format funpack ====="

 # try to run fpack on ours
 tapEXEC /bin/cp -f test.cmp.tbl test.fun.tbl
 tapEXEC $FUNPACK -F test.fun.tbl

 data test.fun.tbl
 read -fits test $fields

 foreach f $fields
   set dv = $f - $f\_raw
   vstat -q dv
   tapOK {abs($MEAN)  < 0.001} "read funpack $f $cmpmode $format (MEAN  = $MEAN)"
   tapOK {$SIGMA      < 0.001} "read funpack $f $cmpmode $format (SIGMA = $SIGMA)"
 end
 delete -q $fields
end

