#!/usr/bin/env mana
# -*-sh-*-

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
$BaseOptions = -type OBJECT 
$BaseOptions = $BaseOptions -filter r 
$BaseOptions = $BaseOptions -skymags 20.86 
$BaseOptions = $BaseOptions -ra 270.70 
$BaseOptions = $BaseOptions -dec -23.70 
$BaseOptions = $BaseOptions -pa 0.0
# $BaseOptions = $BaseOptions -Df PSASTRO:DVO.GETSTAR.MAX.RHO 50000.0

# options for the reference image
$RefOptions = $BaseOptions 
$RefOptions = $RefOptions -exptime 100.0
$RefOptions = $RefOptions -seeing 1.0
$RefOptions = $RefOptions -D PSF.MODEL PS_MODEL_GAUSS
$RefOptions = $RefOptions -Df STARS.DENSITY 10.0
$RefOptions = $RefOptions -Df STARS.SIGMA.LIM 0.5
$RefOptions = $RefOptions -nx 3000 -ny 3000

if (not($?PSFMODEL))
  $PSFMODEL = PS1_V1
end

macro reset.options
# options for the simulated images (using the refimage for the stars)
$FakeOptions = $BaseOptions
$FakeOptions = $FakeOptions -exptime 30.0
# $FakeOptions = $FakeOptions -D PSF.MODEL PS_MODEL_GAUSS
$FakeOptions = $FakeOptions -D PSF.MODEL PS_MODEL_$PSFMODEL
$FakeOptions = $FakeOptions -nx 3000 -ny 3000
  
# sample alternate options:
# $FakeOptions = $FakeOptions -D PSF.MODEL PS_MODEL_PS1_V1
# $FakeOptions = $FakeOptions -Df PSF.ARATIO 1.2
# $FakeOptions = $FakeOptions -Df PSF.THETA +30.0

# create an image with fake sources (these are then inserted into the catdir)
$RefConfig = -camera SIMTEST
$RefConfig = $RefConfig -recipe PPSIM STACKTEST.MAKE

# basic config for ppSim with randomly distributed stars and gridded galaxies
$FakeConfig = -camera SIMTEST
$FakeConfig = $FakeConfig -recipe PPSIM STACKTEST.RUN
$FakeConfig = $FakeConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref
$FakeConfig = $FakeConfig -Db STARS.FAKE F                         ; # only use stars from catdir.ref
$FakeConfig = $FakeConfig -Db MATCH.DENSITY F
$FakeConfig = $FakeConfig -Db PSF.CONVOLVE T
$FakeConfig = $FakeConfig -Db GALAXY.FAKE T                        ; # generate a "realistic" distribution of galaxies
$FakeConfig = $FakeConfig -Df GALAXY.MAG 17.0
$FakeConfig = $FakeConfig -Db GALAXY.GRID T                        ; # generate a grid of galaxies (constant mag)
#$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_GAUSS
#$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_EXP
#$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_SERSIC
#$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_DEV
$FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MIN 10.0
$FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MAX 10.0
$FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MIN 0.25
$FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MAX 0.25
$FakeConfig = $FakeConfig -Df GALAXY.THETA.MIN 0 
$FakeConfig = $FakeConfig -Df GALAXY.THETA.MAX 180
$FakeConfig = $FakeConfig -Df GALAXY.INDEX.MIN 1.66
$FakeConfig = $FakeConfig -Df GALAXY.INDEX.MAX 1.66
$FakeConfig = $FakeConfig -Di GALAXY.GRID.DX 300
$FakeConfig = $FakeConfig -Di GALAXY.GRID.DY 300
end

if (not($?FakeConfig)) reset.options

list fwhm 
 1.0 
 1.1 
 1.2 
 1.5
end

if (not($?CONVOLVE_NSIGMA)) set CONVOLVE_NSIGMA = 5.0

# generate fake images and run psphot on them
macro mkexp.devexp.single
  if ($0 != 5)
    echo "USAGE: mkexp.devexp.single (basename) (type) (Rmajor) (fwhm)"  
    break
  end

  $basename = $1
  $type = $2
  $Rmajor = $3
  $fwhm = $4

  $Aratio = 1.0

  $FakeConfig = -camera SIMTEST
  $FakeConfig = $FakeConfig -recipe PPSIM STACKTEST.RUN
  $FakeConfig = $FakeConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref
  $FakeConfig = $FakeConfig -Db STARS.FAKE F                         ; # only use stars from catdir.ref
  $FakeConfig = $FakeConfig -Db MATCH.DENSITY F
  $FakeConfig = $FakeConfig -Db PSF.CONVOLVE T
  $FakeConfig = $FakeConfig -Db GALAXY.FAKE T                        ; # generate a "realistic" distribution of galaxies
  $FakeConfig = $FakeConfig -Df GALAXY.MAG 17.0
  $FakeConfig = $FakeConfig -Df GALAXY.GRID.MAG 14.5
  $FakeConfig = $FakeConfig -Db GALAXY.GRID T                        ; # generate a grid of galaxies (constant mag)
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MIN 0 
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MAX 180
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DX 300
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DY 300
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MIN 1.0
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MAX 1.0
  $FakeConfig = $FakeConfig -Df CONVOLVE.NSIGMA $CONVOLVE_NSIGMA 
  $BaseConfig = $FakeConfig

  $FakeConfig = $BaseConfig
  $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MIN $Rmajor
  $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MAX $Rmajor
  $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MIN $Aratio
  $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MAX $Aratio
	  
  mkexp $basename $fwhm $type
end

# generate fake images and run psphot on them
macro normtest.mkexp.devexp
  $FakeConfig = -camera SIMTEST
  $FakeConfig = $FakeConfig -recipe PPSIM STACKTEST.RUN
  $FakeConfig = $FakeConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref
  $FakeConfig = $FakeConfig -Db STARS.FAKE F                         ; # only use stars from catdir.ref
  $FakeConfig = $FakeConfig -Db MATCH.DENSITY F
  $FakeConfig = $FakeConfig -Db PSF.CONVOLVE T
  $FakeConfig = $FakeConfig -Db GALAXY.FAKE T                        ; # generate a "realistic" distribution of galaxies
  $FakeConfig = $FakeConfig -Df GALAXY.MAG 17.0
  $FakeConfig = $FakeConfig -Df GALAXY.GRID.MAG 14.5
  $FakeConfig = $FakeConfig -Db GALAXY.GRID T                        ; # generate a grid of galaxies (constant mag)
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MIN 0 
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MAX 180
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DX 300
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DY 300
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MIN 1.0
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MAX 1.0
  $BaseConfig = $FakeConfig

  # $FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_GAUSS
  # $FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_EXP
  # $FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_SERSIC
  # $FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_DEV
  # $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MIN 10.0
  # $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MAX 10.0
  # $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MIN 0.25
  # $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MAX 0.25
  # $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MIN 1.66
  # $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MAX 1.66

  mkdir normtest

  $Nseq = 0
  foreach type EXP DEV
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 1.0
          $FakeConfig = $BaseConfig
          $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MIN $Rmajor
          $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MAX $Rmajor
          $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MIN $Aratio
          $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MAX $Aratio
	  
          sprint name "normtest/test.%02d" $Nseq
          mkexp $name $fwhm $type
	  $Nseq ++
        end
      end
    end
  end
end

# generate fake images and run psphot on them
macro grid.mkexp.devexp
  if ($0 != 2)
    echo "USAGE: grid.mkexp.devexp (dir)"
    break
  end

  mkdir $1

  $FakeConfig = -camera SIMTEST
  $FakeConfig = $FakeConfig -recipe PPSIM STACKTEST.RUN
  $FakeConfig = $FakeConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref
  $FakeConfig = $FakeConfig -Db STARS.FAKE F                         ; # only use stars from catdir.ref
  $FakeConfig = $FakeConfig -Db MATCH.DENSITY F
  $FakeConfig = $FakeConfig -Db PSF.CONVOLVE T
  $FakeConfig = $FakeConfig -Db GALAXY.FAKE T                        ; # generate a "realistic" distribution of galaxies
  $FakeConfig = $FakeConfig -Df GALAXY.MAG 17.0
  $FakeConfig = $FakeConfig -Df GALAXY.GRID.MAG 14.5
  $FakeConfig = $FakeConfig -Db GALAXY.GRID T                        ; # generate a grid of galaxies (constant mag)
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MIN 0 
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MAX 180
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DX 300
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DY 300
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MIN 1.0
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MAX 1.0
  $BaseConfig = $FakeConfig

  $Nseq = 0
  foreach type EXP DEV
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          $FakeConfig = $BaseConfig
          $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MIN $Rmajor
          $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MAX $Rmajor
          $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MIN $Aratio
          $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MAX $Aratio
	  
          sprint name "$1/sample.%02d" $Nseq
          mkexp $name $fwhm $type
	  $Nseq ++
        end
      end
    end
  end
end

# generate fake images and run psphot on them
macro grid.fitexp.devexp
  if ($0 != 3)
    echo "USAGE: grid.fitexp.devexp (srcdir) (outdir)"
    break
  end

   #$Nseq = 0
   #foreach type EXP DEV

  mkdir $2

  $Nseq = 0
  foreach type EXP DEV
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          sprint srcname "$1/sample.%02d" $Nseq
          sprint outname "$2/sample.%02d.fit" $Nseq
          fitexp $srcname $outname $type\_CONV
	  $Nseq ++
        end
      end
    end
  end
end

macro grid.load.devexp
  if ($0 != 3)
    echo "USAGE: grid.load.devexp (srcdir) (fitdir)"
    break
  end

  delete -q Xin_s Yin_s Min_s Tin_s Rin_s rin_s
  delete -q Xot_s Yot_s Mot_s Tot_s Rot_s rot_s
  delete -q min_S Min_S

  $Nseq = 0
  foreach type EXP DEV
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          sprint name "sample.%02d" $Nseq
          cmf.load.concat $1/$name.dat $2/$name.fit.cmf $type
	  $Nseq ++
        end
      end
    end
  end
end

macro grid.plots.devexp
  if ($0 != 2)
    echo "USAGE: grid.plot.devexp (version)"
    break
  end
  # things to examine: theta, Rmajor, AR

  # go.grid.check.devexp

  # check on theta
  set dT = Tot_s - Tin_s
  set ARin = rin_s / Rin_s
  # lim ARin dT; clear; box; plot ARin dT

  subset dTx = dT if (ARin < 0.95)
  histogram dTx NdT -8 8 0.1 -range dTi
  lim -n 0$1 dTi NdT; clear; box; plot -x 1 dTi NdT
  label -x "angle offset (degrees, only non-circular)" -y "number count"
  resize 700 320

  # check on Rmajor
  set dR = Rin_s - Rot_s
  # lim Rin_s dR; clear; box; plot Rin_s dR
  # lim Rot_s dR; clear; box; plot Rot_s dR
  # lim Rin_s dR; clear; box; plot Rin_s dR
  create n 0 dR[]
  set dRf = dR / Rin_s
  lim -n 1$1 n -0.5 0.5; clear; box; plot n dRf
  label -x sequence -y "1 - R_out| / R_in|"
  resize 700 320

  # check on A.Ratio
  set ARot = rot_s / Rot_s
  # lim ARin ARot; clear; box; plot ARin ARot
  set fAR = ARot / ARin
  lim -n 2$1 n 0.5 1.5; clear; box; plot n fAR
  label -x sequence -y "AR_out| / AR_in|"
  resize 700 320

  # check on magnitude
  set dM = Mot_s - Min_s
  lim -n 3$1 n -0.5 0.5; clear; box; plot n dM    
  label -x sequence -y "M_out| - M_in|"
  resize 700 320

  # check on magnitude
  set dI = Iot_s - Iin_s
  lim -n 4$1 n -1.0 1.0; clear; box; plot n dI
  label -x sequence -y "I_out| - I_in|"
  resize 700 320
end

macro grid.plot.stars
  if ($0 != 2)
    echo "USAGE: grid.plot.stars (version)"
    break
  end
  # things to examine: theta, Rmajor, AR

  # go.grid.check.devexp

  # check on position
  set dX = Xot_s - int(Xin_s) - 0.5
  set dY = Yot_s - int(Yin_s) - 0.5
  set ARin = rin_s / Rin_s
  set dR = Rin_s - Rot_s

  create n 0 dR[]
  set dRf = dR / Rin_s
  lim -n 1$1 n -0.5 0.5; clear; box; plot n dRf
  label -x sequence -y "1 - R_out| / R_in|"
  resize 700 320

  # check on A.Ratio
  set ARot = rot_s / Rot_s
  # lim ARin ARot; clear; box; plot ARin ARot
  set fAR = ARot / ARin
  lim -n 2$1 n 0.5 1.5; clear; box; plot n fAR
  label -x sequence -y "AR_out| / AR_in|"
  resize 700 320

  # check on magnitude
  set dM = Mot_s - Min_s
  lim -n 3$1 n -0.5 0.5; clear; box; plot n dM    
  label -x sequence -y "M_out| - M_in|"
  resize 700 320

  lim -n 4$1 n -1.5 1.5; clear; box; plot n dX
  label -x sequence -y "X_out| - X_in|"
  resize 700 320

  lim -n 5$1 n -1.5 1.5; clear; box; plot n dY
  label -x sequence -y "Y_out| - Y_in|"
  resize 700 320
end

macro grid.mkexp.sersic
  if ($0 != 2)
    echo "USAGE: grid.mkexp.devexp (dir)"
    break
  end

  mkdir $1

  $FakeConfig = -camera SIMTEST
  $FakeConfig = $FakeConfig -recipe PPSIM STACKTEST.RUN
  $FakeConfig = $FakeConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref
  $FakeConfig = $FakeConfig -Db STARS.FAKE F                         ; # only use stars from catdir.ref
  $FakeConfig = $FakeConfig -Db MATCH.DENSITY F
  $FakeConfig = $FakeConfig -Db PSF.CONVOLVE T
  $FakeConfig = $FakeConfig -Db GALAXY.FAKE T                        ; # generate a "realistic" distribution of galaxies
  $FakeConfig = $FakeConfig -Df GALAXY.MAG 17.0
  $FakeConfig = $FakeConfig -Df GALAXY.GRID.MAG 14.5
  $FakeConfig = $FakeConfig -Db GALAXY.GRID T                        ; # generate a grid of galaxies (constant mag)
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MIN 0 
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MAX 180
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DX 300
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DY 300
  $FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_SERSIC
  $BaseConfig = $FakeConfig

  $Nseq = 0
  foreach index 1 2 3 4
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          $FakeConfig = $BaseConfig
          $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MIN $Rmajor
          $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MAX $Rmajor
          $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MIN $Aratio
          $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MAX $Aratio
  	  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MIN $index
  	  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MAX $index
	  
          sprint name "$1/sersic.%02d" $Nseq
          mkexp $name $fwhm SERSIC
	  echo "$Nseq : $index $Rmajor $Aratio $fwhm"
	  $Nseq ++
        end
      end
    end
  end
end

macro grid.fitexp.sersic.devexp
  if ($0 != 3)
    echo "USAGE: grid.fitexp.sersic.devexp (srcdir) (outdir)"
    break
  end

  mkdir $2

  $Nseq = 0
  foreach index 1 2 3 4
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          sprint name "sersic.%02d" $Nseq
          fitexp $1/$name $2/$name.fit EXP_CONV,DEV_CONV
	  $Nseq ++
        end
      end
    end
  end
end

macro grid.fitexp.sersic
  if ($0 != 3)
    echo "USAGE: grid.fitexp.sersic (srcdir) (outdir)"
    break
  end

  mkdir $2

  $Nseq = 0
  foreach index 1 2 3 4
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          sprint name "sersic.%02d" $Nseq
          fitexp $1/$name $2/$name.fit SER\_CONV
	  $Nseq ++
        end
      end
    end
  end
end

macro grid.load.sersic
  if ($0 != 3)
    echo "USAGE: grid.load.devexp (srcdir) (fitdir)"
    break
  end

  delete -q Xin_s Yin_s Min_s Tin_s Rin_s rin_s MTin_s Iin_s
  delete -q Xot_s Yot_s Mot_s Tot_s Rot_s rot_s MTot_s Iot_s

  $Nseq = 0
  foreach index 1 2 3 4
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          sprint name "sersic.%02d" $Nseq
          cmf.load.concat $1/$name.dat $2/$name.fit.cmf SERSIC
	  $Nseq ++
        end
      end
    end
  end
end

# I want to make plots of Iin_s vs Mkron, Mxx, and similar things
# this means I need to be able to join Chip.xfit things against Chip.psf
# and to join Chip.xfit(DEV) to Chip.xfit(EXP)

# I think I need a generic 'JOIN' function

macro grid.plots.sersic
  if ($0 != 2)
    echo "USAGE: grid.plot.devexp (version)"
    break
  end
  # things to examine: theta, Rmajor, AR

  # go.grid.check.devexp

  # check on theta
  set dT = Tot_s - Tin_s
  set ARin = rin_s / Rin_s
  # lim ARin dT; clear; box; plot ARin dT

  subset dTx = dT if (ARin < 0.95)
  histogram dTx NdT -8 8 0.1 -range dTi
  lim -n 0$1 dTi NdT; clear; box; plot -x 1 dTi NdT
  label -x "angle offset (degrees, only non-circular)" -y "number count"
  resize 700 320

  # check on Rmajor
  set dR = Rin_s - Rot_s
  # lim Rin_s dR; clear; box; plot Rin_s dR
  # lim Rot_s dR; clear; box; plot Rot_s dR
  # lim Rin_s dR; clear; box; plot Rin_s dR
  create n 0 dR[]
  set dRf = dR / Rin_s
  lim -n 1$1 n -0.5 0.5; clear; box; plot n dRf
  label -x sequence -y "1 - R_out| / R_in|"
  resize 700 320

  # check on A.Ratio
  set ARot = rot_s / Rot_s
  # lim ARin ARot; clear; box; plot ARin ARot
  set fAR = ARot / ARin
  lim -n 2$1 n 0.5 1.5; clear; box; plot n fAR
  label -x sequence -y "AR_out| / AR_in|"
  resize 700 320

  # check on magnitude
  set dM = Mot_s - Min_s
  lim -n 3$1 n -0.5 0.5; clear; box; plot n dM    
  label -x sequence -y "M_out| - M_in|"
  resize 700 320

  # check on index
  set dI = Iot_s - Iin_s
  lim -n 4$1 n -0.5 0.5; clear; box; plot n dI    
  label -x sequence -y "I_out| - I_in|"
  resize 700 320
end

# create a reference database of fake stars to be used by ppSim below
macro mkref
  exec rm -rf catdir.ref
  exec rm -f refimage.fits
  
  echo ppSim $RefOptions $RefConfig refimage
  exec ppSim $RefOptions $RefConfig refimage
  
  file synth.photcodes found
  if (not($found))
    echo "making photcodes file"
    mkphotcodes synth.photcodes
  end

  exec addstar -D CAMERA simtest -D CATDIR catdir.ref -accept-astrom -photcode SYNTH.r -D PHOTCODE_FILE synth.photcodes refimage.cmf -quick-airmass
  exec relphot -averages -D CATDIR catdir.ref -update -region 260 280 -33 -13
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkexp
  if ($0 != 4)
    echo "USAGE: mkexp basename (fwhm) (model)"
    break
  end

  local fwhm basename
  $basename = $1
  $fwhm = $2

  # create the raw image
  $FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_$3
  echo ppSim $FakeOptions $FakeConfig $basename -seeing $fwhm
  exec ppSim $FakeOptions $FakeConfig $basename -seeing $fwhm
  exec /bin/mv -f $basename.cmf $basename.in.cmf

  # create the chip output
  #echo ppImage $ppImageConfig -file $basename.fits $basename
  #exec ppImage $ppImageConfig -file $basename.fits $basename
end

if (not($?NSIGMA_CONV)) set NSIGMA_CONV = 5.0

# create a realistic distribution of fake stars, GAUSS PSF
macro fitexp
  if ($0 != 4)
    echo "USAGE: fitexp basename outname (fitModel)"
    break
  end

  local basename fitModel outname
  $basename = $1
  $outname  = $2
  $fitModel = $3

  $psphotConfig = 
  $psphotConfig = $psphotConfig -recipe PSPHOT GALAXY_TEST
  $psphotConfig = $psphotConfig -threads 4
  $psphotConfig = $psphotConfig -Db PSPHOT:LMM_FIT_CHISQ_CONVERGENCE F
  $psphotConfig = $psphotConfig -Df PSPHOT:EXT_FIT_MIN_TOL 0.1
  $psphotConfig = $psphotConfig -Di PSPHOT:LMM_FIT_GAIN_FACTOR_MODE 2
  $psphotConfig = $psphotConfig -Db PSPHOT:SAVE.RESID T
  $psphotConfig = $psphotConfig -D  PSPHOT:EXTENDED_SOURCE_MODELS_SELECTION $fitModel
  $psphotConfig = $psphotConfig -D  PSPHOT:PSF_MODEL PS_MODEL_$PSFMODEL

  $psphotConfig = $psphotConfig -Db PSPHOT:PSF.RESIDUALS F
  $psphotConfig = $psphotConfig -Db PSPHOT:POISSON.ERRORS.PHOT.LMM F
  $psphotConfig = $psphotConfig -Db PSPHOT:EXTENDED_SOURCE_FITS_POISSON F
  $psphotConfig = $psphotConfig -Di PSPHOT:EXT_FIT_ITER 15
  $psphotConfig = $psphotConfig -Df PSPHOT:PSF_FIT_RADIUS_SCALE 3.75
  $psphotConfig = $psphotConfig -Df PSPHOT:EXT_FIT_NSIGMA_CONV $NSIGMA_CONV

  # ppImage / psphot on the output
  break -auto off
  echo ppImage $ppImageConfig $psphotConfig -file $basename.fits $outname
  exec echo ppImage $ppImageConfig $psphotConfig -file $basename.fits $outname >& $outname.log
  exec ppImage $ppImageConfig $psphotConfig -file $basename.fits $outname >>& $outname.log
  break -auto on
end

macro cmf.load.reset
  $fields = X Y M T R r MT I
  foreach field $fields
    foreach set in ot
      delete -q $field\$set\_s
    end
  end

  delete -q min_S Min_S
end

macro cmf.load.concat
  if ($0 != 4)
    echo "USAGE: cmf.load.concat (dat) (cmf) (inType)"
    break
  end

  data $1
  read Xin_all 1 Yin_all 2 Fin_all 3 Type 4 Min_all 5 RmajIn_all 7 RminIn_all 8 ThetaIn_all 9 IndexIn_all 10

  $TYPE_S = 83
  $TYPE_D = 68
  $TYPE_E = 69
  if ("$3" == "SERSIC")
    $InType = $TYPE_S
  end
  if ("$3" == "DEV")
    $InType = $TYPE_D
    set IndexIn_all = 0.125 + zero(Xin_all)
  end
  if ("$3" == "EXP")
    $InType = $TYPE_E
    set IndexIn_all = 0.5 + zero(Xin_all)
  end

  # select only the galaxies 
  subset Xin = Xin_all if (Type == 1)
  subset Yin = Yin_all if (Type == 1)
  subset Min = Min_all if (Type == 1)
  subset Fin = Fin_all if (Type == 1)
  set min = -2.5*log(Fin)

  subset Tin_rad = ThetaIn_all if (Type == 1)
  set Tin = Tin_rad * 180 / 3.14159265

  subset RmajIn = RmajIn_all if (Type == 1)
  subset RminIn = RminIn_all if (Type == 1)

  subset IndexIn = IndexIn_all if (Type == 1)

  data $2

  break -auto off
  output -err /dev/null
  read -fits Chip.xfit X_EXT Y_EXT EXT_INST_MAG EXT_WIDTH_MAJ EXT_WIDTH_MIN EXT_THETA MODEL_TYPE EXT_PAR_07
  $reread = not($STATUS)
  output -err stderr
  break -auto on
  if ($reread)
    read -fits Chip.xfit X_EXT Y_EXT EXT_INST_MAG EXT_WIDTH_MAJ EXT_WIDTH_MIN EXT_THETA MODEL_TYPE 
    set EXT_PAR_07 = (MODEL_TYPE:9 == 68)*0.125 + (MODEL_TYPE:9 == 69)*0.5
  end

  set EXT_THETA_ALT = EXT_THETA * (EXT_THETA >= 0.0) + (EXT_THETA + 3.14159265) * (EXT_THETA < 0.0)
  set EXT_THETA = EXT_THETA_ALT * 180 / 3.14159265
  
  match2d X_EXT Y_EXT Xin Yin 1.0 -index1 index1 -index2 index2

  reindex Xot_m = X_EXT using index1
  reindex Yot_m = Y_EXT using index1

  reindex Xin_m = Xin using index2
  reindex Yin_m = Yin using index2

  set MTin_m = $InType + zero(Xin_m)
  reindex MTot_m = MODEL_TYPE:9 using index1

  reindex Mot_m = EXT_INST_MAG using index1
  reindex Tot_m = EXT_THETA using index1

  reindex Min_m = Min using index2
  reindex Tin_m = Tin using index2

  reindex Rot_m = EXT_WIDTH_MAJ using index1
  reindex rot_m = EXT_WIDTH_MIN using index1

  reindex Rin_m = RmajIn using index2
  reindex rin_m = RminIn using index2
  
  reindex Iot_m = EXT_PAR_07 using index1
  reindex Iin_m = IndexIn using index2
  $fields = X Y M T R r MT I
  
  foreach field $fields
    foreach set in ot
      concat $field\$set\_m $field\$set\_s
    end
  end

  concat min min_S
  concat Min Min_S
end

macro cmf.load.stars.concat
  if ($0 != 3)
    echo "USAGE: cmf.load.concat (dat) (cmf)"
    break
  end

  data $1
  read Xin_all 1 Yin_all 2 Type 4 Min_all 5 RmajIn_all 7 RminIn_all 8 ThetaIn_all 9 IndexIn_all 10

  subset Xin     = Xin_all     if (Type == 0)
  subset Yin     = Yin_all     if (Type == 0)
  subset Min     = Min_all     if (Type == 0)
  subset RmajIn  = RmajIn_all  if (Type == 0)
  subset RminIn  = RminIn_all  if (Type == 0)
  subset IndexIn = IndexIn_all if (Type == 0)
  subset Tin_rad = ThetaIn_all if (Type == 0)
  set Tin = Tin_rad * 180 / 3.14159265

  data $2

  break -auto off
  read -fits Chip.psf X_PSF Y_PSF PSF_INST_MAG PSF_MAJOR PSF_MINOR PSF_THETA
  set PSF_THETA_ALT = PSF_THETA * (PSF_THETA >= 0.0) + (PSF_THETA + 3.14159265) * (PSF_THETA < 0.0)
  set PSF_THETA = PSF_THETA_ALT * 180 / 3.14159265
  
  match2d X_PSF Y_PSF Xin Yin 1.0 -index1 index1 -index2 index2

  reindex Xot_m = X_PSF using index1
  reindex Yot_m = Y_PSF using index1

  reindex Xin_m = Xin using index2
  reindex Yin_m = Yin using index2

  reindex Mot_m = PSF_INST_MAG using index1
  reindex Tot_m = PSF_THETA using index1

  reindex Min_m = Min using index2
  reindex Tin_m = Tin using index2

  reindex Rot_m = PSF_MAJOR using index1
  reindex rot_m = PSF_MINOR using index1

  reindex Rin_m = RmajIn using index2
  reindex rin_m = RminIn using index2
  
  $fields = X Y M T R r
  foreach field $fields
    foreach set in ot
      concat $field\$set\_m $field\$set\_s
    end
  end
end

macro grid.load.sersic.test
  if ($0 != 3)
    echo "USAGE: grid.load.devexp.test (srcdir) (fitdir)"
    break
  end

  $fields_bt = X Y M T R r I
  $fields_ot = Pmag Kmag Amag IDx MTot

  foreach field $fields_bt
    foreach set in ot
      delete -q $field\$set\_dev_s
      delete -q $field\$set\_exp_s
    end
  end
  foreach field $fields_ot
    delete -q $field\_dev_s
    delete -q $field\_exp_s
  end

  $Nseq = 0
  foreach index 1 2 3 4
    foreach Rmajor 3 10 30
      foreach Aratio 0.25 0.5 1.0
        foreach fwhm 0.8 1.0 1.5
          sprint name "sersic.%02d" $Nseq
          cmf.load.sersic.test $1/$name.dat $2/$name.fit.cmf
	  $Nseq ++
        end
      end
    end
  end
end

# I have run DEV and EXP against input models of type SERSIC
macro cmf.load.sersic.test
  if ($0 != 3)
    echo "USAGE: cmf.load.sersic.test (dat) (cmf)"
    break
  end

  # input parameters
  data $1
  read Xin_all 1 Yin_all 2 Fin_all 3 Type 4 Min_all 5 RmajIn_all 7 RminIn_all 8 ThetaIn_all 9 IndexIn_all 10

  # galaxies only
  subset Xin = Xin_all if (Type == 1)
  subset Yin = Yin_all if (Type == 1)
  subset Min = Min_all if (Type == 1)
  subset Fin = Fin_all if (Type == 1)
  set min = -2.5*log(Fin)

  subset Tin_rad = ThetaIn_all if (Type == 1)
  set Tin = Tin_rad * 180 / 3.14159265

  subset RmajIn = RmajIn_all if (Type == 1)
  subset RminIn = RminIn_all if (Type == 1)

  subset IndexIn = IndexIn_all if (Type == 1)

  $TYPE_S = 83
  $TYPE_D = 68
  $TYPE_E = 69

  data $2

  # load measured values from xfit
  read -fits Chip.xfit IPP_IDET X_EXT Y_EXT EXT_INST_MAG EXT_WIDTH_MAJ EXT_WIDTH_MIN EXT_THETA MODEL_TYPE PSF_INST_MAG AP_MAG KRON_MAG
  set EXT_PAR_07 = (MODEL_TYPE:9 == $TYPE_D)*4 + (MODEL_TYPE:9 == $TYPE_E)

  set EXT_THETA_ALT = EXT_THETA * (EXT_THETA >= 0.0) + (EXT_THETA + 3.14159265) * (EXT_THETA < 0.0)
  set EXT_THETA = EXT_THETA_ALT * 180 / 3.14159265
  set IPP_IDET_EXT = IPP_IDET
  
  match2d X_EXT Y_EXT Xin Yin 1.0 -index1 index1 -index2 index2

  reindex Xot_m = X_EXT using index1
  reindex Yot_m = Y_EXT using index1

  reindex Xin_m = Xin using index2
  reindex Yin_m = Yin using index2

  reindex Mot_m = EXT_INST_MAG using index1
  reindex Tot_m = EXT_THETA using index1

  reindex Min_m = Min using index2
  reindex Tin_m = Tin using index2

  reindex Rot_m = EXT_WIDTH_MAJ using index1
  reindex rot_m = EXT_WIDTH_MIN using index1

  reindex Rin_m = RmajIn using index2
  reindex rin_m = RminIn using index2
  
  reindex Pmag_m = PSF_INST_MAG using index1
  reindex Kmag_m = KRON_MAG using index1
  reindex Amag_m = AP_MAG using index1

  reindex IDx_m = IPP_IDET_EXT using index1

  reindex MTot_m = MODEL_TYPE:9 using index1

  reindex Iot_m = EXT_PAR_07 using index1
  reindex Iin_m = IndexIn using index2
  
  # load moments and other kron values from Chip.psf
  read -fits Chip.psf IPP_IDET X_PSF Y_PSF MOMENTS_XX MOMENTS_XY MOMENTS_YY KRON_FLUX_INNER MOMENTS_R1 MOMENTS_RH
  set IPP_IDET_PSF = IPP_IDET

  join -outer IPP_IDET_PSF IDx_m 
  reindex Xp = X_PSF using index2
  reindex Yp = Y_PSF using index2

  reindex Mxx_m = MOMENTS_XX using index2
  reindex Mxy_m = MOMENTS_XY using index2
  reindex Myy_m = MOMENTS_YY using index2
  reindex Mr1_m = MOMENTS_R1 using index2
  reindex Mrh_m = MOMENTS_RH using index2
  reindex Kfi_m = KRON_FLUX_INNER using index2
  set Kmi_m = -2.5*log(Kfi_m)

  $fields_bt = X Y M T R r I
  $fields_ot = Pmag Kmag Amag IDx MTot Mxx Mxy Myy Mr1 Mrh Kmi

  foreach field $fields_ot
    subset $field\_exp_m = $field\_m where (MTot_m == $TYPE_E)
    subset $field\_dev_m = $field\_m where (MTot_m == $TYPE_D)
  end
  foreach field $fields_bt
    foreach set in ot
      subset $field\$set\_exp_m = $field\$set\_m where (MTot_m == $TYPE_E)
      subset $field\$set\_dev_m = $field\$set\_m where (MTot_m == $TYPE_D)
    end
  end

  join IDx_exp_m IDx_dev_m
  foreach field $fields_ot
    reindex $field\_exp_mr = $field\_exp_m using index1
    reindex $field\_dev_mr = $field\_dev_m using index2
  end
  foreach field $fields_bt
    foreach set in ot
      reindex $field\$set\_exp_mr = $field\$set\_exp_m using index1
      reindex $field\$set\_dev_mr = $field\$set\_dev_m using index2
    end
  end

  # concat
  foreach field $fields_bt
    foreach set in ot
      concat $field\$set\_dev_mr $field\$set\_dev_s
      concat $field\$set\_exp_mr $field\$set\_exp_s
    end
  end
  foreach field $fields_ot
    concat $field\_dev_mr $field\_dev_s
    concat $field\_exp_mr $field\_exp_s
  end

  concat min min_S
  concat Min Min_S
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

macro sersic.integral
  if ($0 != 2)
    echo "sersic.integral (index)"
    break
  end

  $index = $1
  create r 0.0 200.0 0.001
  set q = r^(1/$index)
  set f = exp(-q)
  set rf = f*r
  integrate r rf r[0] r[-1]
end

macro sersic.integral.rmax
  if ($0 != 4)
    echo "sersic.integral.rmax (index) (kappa) (rmax)"
    echo "kappa is a guess for kappa"
    break
  end

  local index kappa

  $index = $1
  $kappa = $2

  create r 0.0 $3 0.003
  set q = r^(1/$index)
  set f = exp(-$kappa*q)
  set rf = f*r
  integrate r rf r[0] r[-1]
end

# S(r) = exp(-kappa*(r/Reff)^(1/index))
# integrate S(r) r dr [ignores 2pi and change-of-variable factors (Rmaj*Rmin)]
macro sersic.integral.reff.rmax
  if ($0 != 5)
    echo "sersic.integral.rmax (index) (kappa) (reff) (rmax)"
    echo "kappa is a guess for kappa"
    break
  end

  local index kappa Reff

  $index = $1
  $kappa = $2
  $Reff = $3

  create r 0.0 $4 0.01
  set q = (r/$Reff)^(1/$index)
  set f = exp(-$kappa*q)
  set rf = f*r
  integrate r rf r[0] r[-1]
end

# integrate to r = reff (rho = 1), applying kappa
macro sersic.integral.reff
  if ($0 != 3)
    echo "sersic.integral.reff (index) (kappa)"
    break
  end

  local index kappa

  $index = $1
  $kappa = $2
  create r 0.0 100.0 0.001
  set q = r^(1/$index)
  set f = exp(-$kappa*q)
  set rf = f*r
  integrate r rf r[0] r[-1]
  $S0 = $sum
  integrate r rf r[0] 1.0
  $S1 = $sum
  echo $S0 $S1 {$S1 / $S0}
end

macro sersic.integral.find.reff
  if ($0 != 3)
    echo "sersic.integral.find.reff (index) (kappa)"
    echo "kappa is a guess for kappa"
    break
  end

  local kappa index Rmax i
  $index = $1
  $kappa = $1

  $Rmin = 0; $Vmin = 0
  $Rmax = 4000
  sersic.integral.rmax $index $kappa $Rmax
  $Vmax = $sum

  $Rtry = 0.5*($Rmin + $Rmax)

  while (abs($Rmax - $Rmin) > 0.001)
    sersic.integral.rmax $index $kappa $Rtry
    $Vtry = $sum

    $Rold = $Rtry
    if ($Vtry > 0.5*$Vmax)
      $Rtry = 0.5*($Rmin + $Rtry)
      $Rmax = $Rold
    else
      $Rtry = 0.5*($Rmax + $Rtry)
      $Rmin = $Rold
    end
    # fprintf "%5.2f %5.2f %5.2f | %5.2f | %6.3f %6.3f | %6.3f" $Rmin $Rold $Rmax $Rtry {$Vtry / $Vmax} $Vmax $Vtry
  end
  # echo $Rtry
  # echo {$kappa * $Rtry ^ (1.0/$index)}
  $KappaReal = $kappa * $Rtry ^ (1.0/$index)
end

macro sersic.integral.index

  local i kappa

  delete -q sersic_sum sersic_norm 
  # vlist idx 0.5 0.75 1.0 1.5 2.0 2.5 3.0 3.5 4.0 4.5 5.0
  # create idx 0.5 5.1 0.1
  vlist idx 1 2 4
  for i 0 idx[]
    $kappa =  -0.275552 + 1.972625*idx[$i] + 0.003487*idx[$i]*idx[$i]
    sersic.integral.rmax idx[$i] $kappa 300
    concat $sum sersic_sum

    $bn = 1.9992*idx[$i] - 0.3271;
    $Io = exp($bn);
    	    
    # the integral of a Sersic (supposedly) has an analytical form as follows:
    $logGamma = lgamma(2.0*idx[$i]);
    $bnFactor = $bn^(2.0*idx[$i]);
    $norm = idx[$i] * $Io * exp($logGamma) / $bnFactor;
    concat $norm sersic_norm

    echo idx[$i] $kappa $sum $logGamma $bn $bnFactor $norm
  end
  set lsersic_sum = ln(sersic_sum)
  break
  
  $order = 2
  delete -q sersic_fit fit_idx

  # fit to ranges:
  delete lsersic_fit fit_idx
  vlist bound 0.0 1.0 2.0 3.0 4.0 5.5
  for i 0 {bound[] - 1}
    subset idxs = idx if (idx >= bound[$i]) && (idx < bound[$i+1]) 
    subset sums = lsersic_sum if (idx >= bound[$i]) && (idx < bound[$i+1]) 
    fit idxs sums $order
    applyfit idxs fits
    concat fits lsersic_fit
    concat idxs fit_idx
  end
  set sersic_fit = exp(lsersic_fit)

  lim -n 1 idx sersic_sum; clear; box; plot idx sersic_sum; plot -c blue -pt 7 idx sersic_norm; plot -x 0 -c red fit_idx sersic_fit
  lim -n 2 idx lsersic_sum; clear; box; plot idx lsersic_sum; plot -x 0 -c red fit_idx lsersic_fit
end

macro sersic.kappa.index

  local i

  delete -q sersic_kappa
  # vlist idx 0.5 0.75 1.0 1.5 2.0 2.5 3.0 3.5 4.0 4.5 5.0
  create idx 0.5 5.1 0.1
  for i 0 idx[]
    sersic.integral.find.reff idx[$i] 10
    concat $KappaReal sersic_kappa
  end
  
  # $order = 2
  # delete -q sersic_kfit kfit_idx
  # 
  # # fit to ranges:
  # vlist bound 0.0 1.0 2.0 3.0 4.0 5.5
  # for i 0 {bound[] - 1}
  #   subset idxs = idx if (idx >= bound[$i]) && (idx < bound[$i+1]) 
  #   subset sums = lsersic_sum if (idx >= bound[$i]) && (idx < bound[$i+1]) 
  #   fit idxs sums $order
  #   applyfit idxs fits
  #   concat fits lsersic_fit
  #   concat idxs fit_idx
  # end
  # set sersic_fit = exp(lsersic_fit)

  lim -n 1 idx sersic_kappa; clear; box; plot idx sersic_kappa; # plot -c blue -pt 7 idx sersic_norm; plot -x 0 -c red fit_idx sersic_fit
  # lim -n 2 idx lsersic_sum; clear; box; plot idx lsersic_sum; plot -x 0 -c red fit_idx lsersic_fit

  # kappa(index) :
  # y = -0.275552 x^0 1.972625 x^1 0.003487 x^2 
end

macro sersic.central.pixel
  if ($0 != 3)
    echo "USAGE: sersic.central.pixel (index) (Reff)"
    break
  end

  local index Reff

  $index = $1
  $Reff = $2

  $kappa = -0.275552 + 1.972625*$index + 0.003487 * $index^2
  # echo "kappa: $kappa"

  sersic.norm $index
  sersic.integral.reff.rmax $index $kappa $Reff {10*$Reff}; set sumFull = $sum; echo $sum {$Reff^2*$myNorm} {$sum / ($Reff^2*$myNorm)}
  # sersic.integral.reff.rmax $index $kappa $Reff $Reff     ; # echo $sum {$sum / $sumFull}
  # sersic.integral.reff.rmax $index $kappa $Reff 0.1       ; # echo $sum {$sum / $sumFull}
  # sersic.integral.reff.rmax $index $kappa $Reff 0.2       ; # echo $sum {$sum / $sumFull}
  # sersic.integral.reff.rmax $index $kappa $Reff 0.5       ; # echo $sum {$sum / $sumFull}
  # sersic.integral.reff.rmax $index $kappa $Reff 1.0       ; # echo $sum {$sum / $sumFull}
  # sersic.integral.reff.rmax $index $kappa $Reff 2.0       ; # echo $sum {$sum / $sumFull}
  sersic.integral.reff.rmax $index $kappa $Reff 0.564     ; echo $index $Reff {$sum / $sumFull}

end

macro sersic.norm
  if ($0 != 2)
    echo "USAGE: sersic.norm (index)"
    break
  end

  local index
  $index = $1

  if (($index >= 0.0) && ($index < 1.0))
      $norm = 0.201545  - 0.950965 * $index - 0.315248 * $index^2
      echo $norm {exp($norm)}
      $myNorm = exp($norm)
      return
  end

  if (($index >= 1.0) && ($index < 2.0)) 
      $norm = 0.402084  - 1.357775 * $index - 0.105102 * $index^2
      echo $norm {exp($norm)}
      $myNorm = exp($norm)
      return
  end

  if (($index >= 2.0) && ($index < 3.0))
      $norm = 0.619093 - 1.591674 * $index - 0.041576 * $index^2
      echo $norm {exp($norm)}
      $myNorm = exp($norm)
      return
  end

  if (($index >= 3.0) && ($index < 4.0))
      $norm = 0.770263 - 1.696421 * $index - 0.023363 * $index^2
      echo $norm {exp($norm)}
      $myNorm = exp($norm)
      return
  end

  if (($index >= 4.0) && ($index < 5.5)) 
      $norm = 0.885891 - 1.755684 * $index - 0.015753 * $index^2
      echo $norm {exp($norm)}
      $myNorm = exp($norm)
      return
  end
end

macro load.model.im
  if ($0 != 2)
    echo "USAGE: load.model.im (N)"
    break
  end

  rd obj$1 obj.$1.fits
  rd cnv$1 cnv.$1.fits
  rd var$1 var.$1.fits
  rd msk$1 msk.$1.fits
  for i 1 7
    rd dpar$i.$1 dpar.$i.$1.fits
  end
  set dC$1 = (msk$1 == 0) * (obj$1 - cnv$1)^2 / var$1
  tv dC$1 -0.01 3.0
end

macro load.normdata
  if ($0 != 3)
    echo "USAGE: load.normdata (file) [clear/noclear]"
    break
  end

  data $1
  read f 3 t 4 m 5
  set M = -2.5*log(f)
  subset Mg = M if (t == 1)
  subset mg = m if (t == 1)
  set dm = mg - Mg
  set n = ramp(dm)
  if ("$2" == "clear")
    lim n dm; clear; box; 
  end
  plot n dm
end

if ($SCRIPT)
  echo "no default action defined"
  exit 0
end

# note that t1 = original confi
# t2 = no residuals
# t3 = const weight PSF fit
# t4 = const weight galaxy fits

macro run.radius.loop
  local radius Nrun 

  if (1)
    $Nrun = 0
    foreach radius 1.0 1.5 2.0 2.5 3.0 4.0 6.0 8.0
      mkexp.devexp.single tests.20131120/testrad.ps1v1.exp.$Nrun EXP $radius 1.0
      fitexp tests.20131120/testrad.ps1v1.exp.$Nrun tests.20131120/testrad.ps1v1.exp.$Nrun.t4b EXP_CONV
      $Nrun ++
    end
  end

  $Nrun = 0
  foreach radius 1.0 1.5 2.0 2.5 3.0 4.0 6.0 8.0
    cmf.load.concat tests.20131120/testrad.ps1v1.exp.$Nrun.dat tests.20131120/testrad.ps1v1.exp.$Nrun.t4b.cmf EXP
    echo -no-return $radius ""
    check.fit tests.20131120/testrad.ps1v1.exp.$Nrun.dat tests.20131120/testrad.ps1v1.exp.$Nrun.t4b.cmf EXP
    $Nrun ++
  end

  grid.plots.devexp a
end

macro check.radius.loop
  if ($0 != 2)
    echo "USAGE: check.radius.loop (ext)"
    break
  end

  # cmf.load.reset

  local radius Nrun 

  $Nrun = 0
  foreach radius 1.0 1.5 2.0 2.5 3.0 4.0 6.0 8.0
    echo -no-return $radius ""
    check.fit tests.20131120/testrad.ps1v1.exp.$Nrun.dat tests.20131120/testrad.ps1v1.exp.$Nrun.$1.cmf EXP
    $Nrun ++
  end

  grid.plots.devexp a
end

macro check.fit
  if ($0 != 4)
    echo "USAGE: check.fit (dat) (cmf) (type)"
    break
  end

  cmf.load.concat $1 $2 $3
  set dM_m = Mot_m - Min_m
  vstat -q dM_m
  $mag_off = $MEDIAN
  set dR_m = Rot_m - Rin_m
  vstat -q dR_m
  $rad_off = $MEDIAN
  echo $mag_off $rad_off
end

macro make.circles

  delete Xo Yo Ro

  for ix 0 16
    for iy $ix 16
      $r2 = $ix^2 + $iy^2
      if ($r2 > 15^2) continue
      concat $ix Xo
      concat $iy Yo
      concat $r2 Ro
    end
  end

  sort Ro Xo Yo
  $Rold = -1
  $N = -1
  for i 0 Ro[]
    if (Ro[$i] != $Rold)
      $N ++
      $Rold = Ro[$i]
    end
    if ((Xo[$i] == 0) && (Yo[$i] == 0))
      fprintf "// center is 0,0"
      continue
    end
    if (Xo[$i] == 0)
      fprintf "ADD_AXIS (%3d, %2d)     // r^2 = %3d" $N Yo[$i] Ro[$i]
      continue
    end
    if (Xo[$i] == Yo[$i])
      fprintf "ADD_DIAG (%3d, %2d)     // r^2 = %3d" $N Xo[$i] Ro[$i]
      continue
    end

    fprintf "ADD_RAND (%3d, %2d, %2d) // r^2 = %3d" $N Xo[$i] Yo[$i] Ro[$i]
 end
end

# EXP : tests.20131120/test.nsig
# DEV : tests.20131120/test.dev

# run.convolve.loop tests.20131120/test.nsig tests.20131120/test.nsig   EXP EXP_CONV
# run.convolve.loop tests.20131120/test.dev  tests.20131120/test.dev    DEV DEV_CONV
# run.convolve.loop tests.20131120/test.nsig tests.20131120/test.serexp EXP SER_CONV
# run.convolve.loop tests.20131120/test.dev  tests.20131120/test.serdev DEV SER_CONV

macro run.convolve.loop
  if ($0 != 5)
    echo "run.convolve.loop (inName) (fitName) (inType) (fitType)"
    break
  end

  local radius
  $radius = 3.0
  cmf.load.reset

  # foreach Cin 3 5 7 9 11
  foreach Cin 11
    $CONVOLVE_NSIGMA = $Cin
    sprintf nameIn %s.%02d $1 $Cin
    mkexp.devexp.single $nameIn $3 $radius 1.0
    foreach Cot 3 5 7 9 11
      $NSIGMA_CONV = $Cot
      sprintf nameOt %s.%02d.%02d $2 $Cin $Cot
      fitexp $nameIn $nameOt $4
    end
  end

  # foreach Cin 3 5 7 9 11
  foreach Cin 11
    sprintf nameIn %s.%02d $1 $Cin
    foreach Cot 3 5 7 9 11
      sprintf nameOt %s.%02d.%02d $2 $Cin $Cot
      check.fit $nameIn.dat $nameOt.cmf 
    end
  end
  grid.plots.devexp a
end

macro check.convolve.loop
  if ($0 != 4)
    echo "run.convolve.loop (inName) (fitName) (inType)"
    break
  end

  local radius
  $radius = 3.0
  cmf.load.reset

  # foreach Cin 3 5 7 9 11
  foreach Cin 11
    sprintf nameIn %s.%02d $1 $Cin
    foreach Cot 3 5 7 9 11
      sprintf nameOt %s.%02d.%02d $2 $Cin $Cot
      echo -no-return $Cin $Cot " "
      check.fit $nameIn.dat $nameOt.cmf $3
    end
  end
  grid.plots.devexp a
end
