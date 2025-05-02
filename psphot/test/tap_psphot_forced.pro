#!/usr/bin/env mana
# -*-sh-*-

# forced photometry

# config for ppImage to generate chip, mask, weight
$ppImageConfig = -recipe PPIMAGE PPIMAGE_N
$ppImageConfig = $ppImageConfig -Db BACKGROUND T
$ppImageConfig = $ppImageConfig -Db CHIP.FITS T
$ppImageConfig = $ppImageConfig -Db CHIP.MASK.FITS T
$ppImageConfig = $ppImageConfig -Db CHIP.VARIANCE.FITS T
$ppImageConfig = $ppImageConfig -Db BASE.FITS F
$ppImageConfig = $ppImageConfig -Db VARIANCE.BUILD T
$ppImageConfig = $ppImageConfig -Db PHOTOM T

# basic options for the these images (filter, location, obstype)
$BaseOptions = -type OBJECT -filter r -skymags 20.86 -ra 270.70 -dec -23.70 -pa 0.0
$BaseOptions = $BaseOptions -Df PSASTRO:DVO.GETSTAR.MAX.RHO 50000.0
$BaseOptions = $BaseOptions -nx 2500 -ny 2500

# options for the reference image
$RefOptions = $BaseOptions -exptime 100.0 -seeing 1.0
$RefOptions = $RefOptions -D PSF.MODEL PS_MODEL_GAUSS
$RefOptions = $RefOptions -Df STARS.SIGMA.LIM 0.5
$RefOptions = $RefOptions -Db PSF.CONVOLVE T
# add the density in mkref
# $RefOptions = $RefOptions -Df STARS.DENSITY 10.0

# options for the repeated images
$FakeOptions = $BaseOptions -exptime 30.0
  
# basic config for ppSim with randomly distributed stars and gridded galaxies
$RealConfig = -camera SIMTEST -recipe PPSIM STACKTEST.RUN
$RealConfig = $RealConfig -Db STARS.FAKE F -Db MATCH.DENSITY F -Db PSF.CONVOLVE T
# we add the catdir in mkexp so we can change the refbase
# $RealConfig = $RealConfig -D PSASTRO:PSASTRO.CATDIR catdir.$refbase

# add galaxies for this test or not?
if (1) 
 $RealConfig = $RealConfig -Db GALAXY.FAKE F
else
 $RealConfig = $RealConfig -Db GALAXY.FAKE T -Df GALAXY.MAG 17.0
 $RealConfig = $RealConfig -Db GALAXY.GRID T -D GALAXY.MODEL PS_MODEL_SERSIC
 $RealConfig = $RealConfig -Df GALAXY.ARATIO.MIN 0.5
 $RealConfig = $RealConfig -Df GALAXY.ARATIO.MAX 0.5
 $RealConfig = $RealConfig -Df GALAXY.THETA.MAX 180
 $RealConfig = $RealConfig -Df GALAXY.INDEX.MIN 1
 $RealConfig = $RealConfig -Df GALAXY.INDEX.MAX 1
 $RealConfig = $RealConfig -Di GALAXY.GRID.DX 120
 $RealConfig = $RealConfig -Di GALAXY.GRID.DY 120
end

$ppSimOptions = $FakeOptions -D PSF.MODEL PS_MODEL_GAUSS

list fwhm 
 1.0 
 1.1 
 1.2 
 1.5
end

macro fulltest
  if ($0 != 2)
    echo "USAGE: fulltest (Npass)"
    break
  end

  local i Npass
  $Npass = $1

  # use a limiting stellar density of 1 / deg^2
  mkref ref 1.0

  # use a FWHM of 1.0 arcsec
  mkexp image/image 1.0 ref

  for i 0 $Npass
    mkforce image/image v$i
    forcedCompare image/image.cmf image/image.v$i.frc.cmf image/image.v$i.frc.png 1.0
  end
end

# create a reference database of fake stars to be used by ppSim below
macro mkref
  if ($0 != 3)
    echo "mkref (refbase) (density)"
    break
  end

  local refbase
  $refbase = $1

  exec rm -rf $refbase.catdir
  exec rm -f $refbase.fits
  
  $RefOptions = $RefOptions -Df STARS.DENSITY $2

  # create an image with fake sources and insert the resulting cmf file into a dvodb
  $RefConfig = -camera SIMTEST -recipe PPSIM STACKTEST.MAKE -D PSASTRO:PSASTRO.CATDIR $refbase.catdir

  exec ppSim $RefOptions $RefConfig $refbase
  
  file synth.photcodes found
  if (not($found))
    echo "making photcodes file"
    mkphotcodes synth.photcodes
  end

  exec addstar -D CAMERA simtest -D CATDIR $refbase.catdir -accept-astrom -photcode SYNTH.r -D PHOTCODE_FILE synth.photcodes $refbase.cmf
  exec relphot -averages -D CATDIR $refbase.catdir -update -region 260 280 -33 -13
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkexp
  if ($0 != 4)
    echo "USAGE: mkexp basename fwhm refbase"
    break
  end

  local fwhm basename refbase
  $basename = $1
  $fwhm = $2
  $refbase = $3

  $dirname = `dirname $basename`
  exec mkdir -p $dirname
 
  $RealConfig = $RealConfig -D PSASTRO:PSASTRO.CATDIR $refbase.catdir

  # create the raw image
  echo ppSim $ppSimOptions $RealConfig $basename -seeing $fwhm
  exec ppSim $ppSimOptions $RealConfig $basename -seeing $fwhm
  exec /bin/mv -f $basename.cmf $basename.in.cmf

  # create the chip output
  echo ppImage $ppImageConfig -file $basename.fits $basename -seed 12345
  exec ppImage $ppImageConfig -file $basename.fits $basename -seed 12345
end

macro mkforce
  if ($0 != 3)
    echo "USAGE: mkforce (basename) (version) "
    break
  end

  local basename
  $basename = $1
  echo $basename

  data $basename.cmf
  read -fits Chip.psf X_PSF Y_PSF 
  write $basename.$2.frc.dat X_PSF Y_PSF
  echo "here"

  $forcedOpt = -file $basename.ch.fits
  $forcedOpt = $forcedOpt -mask $basename.ch.mk.fits
  $forcedOpt = $forcedOpt -variance $basename.ch.wt.fits
  $forcedOpt = $forcedOpt -psf $basename.psf
  $forcedOpt = $forcedOpt -srctext $basename.$2.frc.dat
  exec psphotForced $forcedOpt $basename.$2.frc -seed 12345
end

macro forcedCompare
 if ($0 != 5)
   echo "USAGE: forcedCompare (raw) (out) (output) (radius)"
   break
 end

 load.cmf $1 Chip.psf raw
 load.cmf $2 Chip.psf out

 # if we compare to the pre-convolution deltas, we need to do this:
 # set X_raw = int(X_PSF_raw) + 0.5
 # set Y_raw = int(Y_PSF_raw) + 0.5
 match2d X_PSF_raw Y_PSF_raw X_PSF_out Y_PSF_out $4 -index1 index1 -index2 index2 -closest

 set m1 = PSF_INST_MAG_raw
 set m1err = PSF_INST_MAG_SIG_raw
 set m1err10 = PSF_INST_MAG_SIG_raw / 10.0
 reindex m2 = PSF_INST_MAG_out using index1
 set dm = m1 - m2

 resize 1000 600
 label -fn courier 14 
 lim m1 dm; clear; box; plot m1 dm -dy m1err10

 set chi = dm^2 / m1err^2
 vstat -q chi
 echo "median offset: $MEDIAN, chisq : $TOTAL, reduced chisq : {$TOTAL / $NPTS}"

 sprintf line "median offset: %6.4f, chisq : %6.4f, reduced chisq : %6.4f" $MEDIAN $TOTAL {$TOTAL / $NPTS}
 label -x "standard inst mag" -y "standard mag - forced mag" -ur "$line" 
 label -ul "plotted errors are poisson / 10.0 for clarity"
 png -name $3
end

macro load.cmf
  if ($0 != 4)
   echo "load.cmf (filename) (ext) (label)"
   break
  end

  data $1

  # create the list of fields to load
  delete -q myFields
  for i 0 $fields:n
    if ($?myFields) 
      $myFields = $myFields $fields:$i
    else
      $myFields = $fields:$i
    end
  end

  read -fits $2 $myFields

  # rename the loaded vectors appending the supplied lable
  for i 0 $fields:n
    set $fields:$i\_$3 = $fields:$i
    delete $fields:$i
  end
end

# this list defines the fields to be loaded from file
# for completeness, we only need a few of the fields
list fields
  X_PSF              
  Y_PSF              
  X_PSF_SIG          
  Y_PSF_SIG          
  PSF_INST_MAG       
  PSF_INST_MAG_SIG   
#  PSF_INST_FLUX      
#  PSF_INST_FLUX_SIG  
end

# if we run this test as a stand-alone program somewhere, we may need to create a local copy of the photcode file:
macro mkphotcodes
  if ($0 != 2)
    echo "USAGE: mkphotcodes (filename)"
    break
  end

  exec /bin/rm -f $1
  output $1
  echo "#                                           airmass      color                         astrometry  mag    photom  astrom mask    photom mask"
  echo "# code  name                type    zero  slope offset c1    c2   slope   zero  equiv  sys scale   scale  sys     poor   bad     poor   bad"
  echo "  1     g_SYNTH              sec   0.000  0.000 0.000     1     3 0.0000     0    21   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  2     r_SYNTH              sec   0.000  0.000 0.000     2     3 0.0000     0    22   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3     i_SYNTH              sec   0.000  0.000 0.000     2     3 0.0000     0    23   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  4     z_SYNTH              sec   0.000  0.000 0.000     3     4 0.0000     0    24   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  5     y_SYNTH              sec   0.000  0.000 0.000     4     5 0.0000     0    25   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3001  SYNTH.g              ref   0.000  0.000 0.000     -     - 0.0000     0     1   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3002  SYNTH.r              ref   0.000  0.000 0.000     -     - 0.0000     0     2   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3003  SYNTH.i              ref   0.000  0.000 0.000     -     - 0.0000     0     3   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3004  SYNTH.z              ref   0.000  0.000 0.000     -     - 0.0000     0     4   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  echo "  3005  SYNTH.y              ref   0.000  0.000 0.000     -     - 0.0000     0     5   0.000 0.000 0.000  0.000   0x0000 0x0000  0x0000 0x0000"
  output stdout
end

if ($SCRIPT)
  fulltest 4
  exit 0
end
