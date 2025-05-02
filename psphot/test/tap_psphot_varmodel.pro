#!/usr/bin/env mana
# -*-sh-*-

# This script includes a set of tests to demonstrate the dependence of the faint-end bias on the weighting scheme
# We have 3 weighting schemes :
# CONSTANT -- per-pixel weight is fixed (disadvantage: lower S/N, especially for higher sky?)
# IMAGE_VAR - per-pixel weight is Poisson from image (disadvantage: faint-end bias)
# MODEL_VAR - per-pixel weight is Poisson from model (disadvantage: 2 linear-fit passes)

# Functions:
# 
# init  : initialize variables
# mkref : generate a fake reference catalog in a DVO database
# mkexp : generate a fake exposure from fake catalog and detrend it (saves basename.in.cmf & basename.fits)
# runphot : run psphot on an exposure (saves basename.cmf)
# ckchip.mags : generate a set of summary plots for the psphot analysis

# I would like to produce a grid of tests:
# sky (bright, middle, dark)
# fwhm (1.0, 1.3, 1.6)
# PSF_MODEL input (GAUSS PS1_V1)
# PSF_MODEL apply (GAUSS PS1_V1)
# variance mode: CONSTANT, IMAGE_VAR, MODEL_VAR

macro ppsimtest
  data $1
  # sf1 : measured flux inserted in image
  # sm2 : theoretical magnitude for star
  read x 1 y 2 sf1 3 sm2 5 Io 12
  set sm1 = -2.5*log(sf1)
  set dsm = sm1 - sm2
  lim -n 2 sm1 -0.2 0.2; clear; box; plot sm1 dsm
end

macro go.one
  mkdir test

  local sky fwhm psf_in psf_out

  # mkref creates refimage.* and catdir.ref
  file catdir.ref found
  if (not($found))
    mkref
  end

  $KAPA = kapa -noX
  if (not($?RefConfig)) init

  foreach sky 20.0
    foreach fwhm 1.0
      foreach psf_in PS1_V1

        sprintf name "test/test.%02d.%02d.%s" $sky {10*$fwhm} $psf_in
        mkexp $name $sky $fwhm $psf_in
	data $name.dat
	read fluxIn 3 MagPredIn 5
	set MagRealIn = -2.5*log(fluxIn)
	set dMagIn = MagRealIn - MagPredIn
        
        foreach psf_out GAUSS PS1_V1
          foreach mode CONSTANT IMAGE_VAR MODEL_VAR
            runphot $name $name.$psf_out.$mode "-D LINEAR_FIT_VARIANCE_MODE $mode -D PSF_MODEL PS_MODEL_$psf_out"
            ckchip.mags $name.in.cmf $name.$psf_out.$mode.cmf $name.$psf_out.$mode 0.0
          end
        end
      end
    end
  end
end

macro go
  mkdir test

  local sky fwhm psf_in psf_out

  # mkref creates refimage.* and catdir.ref
  file catdir.ref found
  if (not($found))
    mkref
  end

  $KAPA = kapa -noX
  if (not($?RefConfig)) init

  foreach sky 19.0 20.0 21.0
    foreach fwhm 1.0 1.3 1.6
      foreach psf_in GAUSS PS1_V1

        sprintf name "test/test.%02d.%02d.%s" $sky {10*$fwhm} $psf_in
        # mkexp $name $sky $fwhm $psf_in
	data $name.dat
	read fluxIn 3 MagPredIn 5
	set MagRealIn = -2.5*log(fluxIn)
	set dMagIn = MagRealIn - MagPredIn
        
        foreach psf_out GAUSS PS1_V1
          foreach mode CONSTANT IMAGE_VAR MODEL_VAR
            # runphot $name $name.$psf_out.$mode "-D LINEAR_FIT_VARIANCE_MODE $mode -D PSF_MODEL PS_MODEL_$psf_out"
            ckchip.mags $name.in.cmf $name.$psf_out.$mode.cmf $name.$psf_out.$mode 0.0
          end
        end
      end
    end
  end
end

# create a reference database of fake stars to be used by ppSim below
macro mkref
  if (not($?RefConfig)) init

  exec rm -rf catdir.ref
  exec rm -f refimage.fits
  
  exec time ppSim $RefConfig $RefOptions refimage -nx 3000 -ny 3000
  
  file synth.photcodes found
  if (not($found))
    echo "making photcodes file"
    mkphotcodes synth.photcodes
  end

  exec addstar -D CAMERA simtest -D CATDIR catdir.ref -accept-astrom -photcode SYNTH.r -D PHOTCODE_FILE synth.photcodes refimage.cmf
  exec relphot -averages -D CATDIR catdir.ref -update -region 260 280 -33 -13
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkexp
  if ($0 != 5)
    echo "USAGE: mkexp basename sky fwhm psf_model"
    break
  end

  local fwhm basename psf_model
  $basename = $1
  $sky = $2
  $fwhm = $3
  $psf_model = $4

  $ExtraOptions = -D PSF.MODEL PS_MODEL_$psf_model

  # create the raw image
  echo ppSim -seeing $fwhm -skymags $sky -nx 3000 -ny 3000 $RealConfig $RealOptions $ExtraOptions $basename
  exec ppSim -seeing $fwhm -skymags $sky -nx 3000 -ny 3000 $RealConfig $RealOptions $ExtraOptions $basename
  exec /bin/mv -f $basename.cmf $basename.in.cmf

  # create the chip output
  echo ppImage $ppImageConfig -file $basename.fits $basename
  exec ppImage $ppImageConfig -file $basename.fits $basename
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkexp.deep
  if ($0 != 3)
    echo "USAGE: mkexp basename fwhm"
    break
  end

  local fwhm basename
  $basename = $1
  $fwhm = $2

  $RealOptionsDeep = $BaseOptions -exptime 100

  # create the raw image
  echo ppSim -seeing $fwhm -nx 3000 -ny 3000 $RealConfig $RealOptionsDeep $ExtraOptions $basename
  exec ppSim -seeing $fwhm -nx 3000 -ny 3000 $RealConfig $RealOptionsDeep $ExtraOptions $basename
  exec /bin/mv -f $basename.cmf $basename.in.cmf

  # create the chip output
  echo ppImage $ppImageConfig -file $basename.fits $basename
  exec ppImage $ppImageConfig -file $basename.fits $basename
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkexp.bright
  if ($0 != 3)
    echo "USAGE: mkexp basename fwhm"
    break
  end

  local fwhm basename
  $basename = $1
  $fwhm = $2

  # basic options for the these images (filter, location, obstype)
  $BaseOptions = -type OBJECT -filter r -skymags 20.0 -ra 270.70 -dec -23.70 -pa 0.0
  $BaseOptions = $BaseOptions -Df PSASTRO:DVO.GETSTAR.MAX.RHO 50000.0
  $RealOptionsDeep = $BaseOptions -exptime 100

  # create the raw image
  echo ppSim -seeing $fwhm -nx 3000 -ny 3000 $RealConfig $RealOptionsDeep $ExtraOptions $basename
  exec ppSim -seeing $fwhm -nx 3000 -ny 3000 $RealConfig $RealOptionsDeep $ExtraOptions $basename
  exec /bin/mv -f $basename.cmf $basename.in.cmf

  # create the chip output
  echo ppImage $ppImageConfig -file $basename.fits $basename
  exec ppImage $ppImageConfig -file $basename.fits $basename
end

macro runphot
  if ($0 != 4)
    echo "USAGE: runphot basename outname options"
    break
  end

  local basename
  $basename = $1
  $outname = $2
  $options = $3

  # create the chip output
  echo psphot -threads 4 -file $basename.ch.fits -mask $basename.ch.mk.fits -variance $basename.ch.wt.fits $outname $options
  exec psphot -threads 4 -file $basename.ch.fits -mask $basename.ch.mk.fits -variance $basename.ch.wt.fits $outname $options
end

# compare two cmf files with extname Chip.psf 
# things to compare:
# * completeness (which sources in (1) are not detected in (2)
# * positions (X_PSF, Y_PSF) 
# * instrumental psf mags
# * position errors (no input errors; use a model?)
# * measured FWHM?
# * kron mags (fluxes)
# * etc, etc
macro ckchip.mags
  if ($0 != 5)
    echo "USAGE: ckchip.mags (raw) (out) (output) (zpt_off)"
    break
  end

list pairs
  PSF_INST_MAG_out      M_raw                 PSF_INST_MAG_raw 2 -0.21 0.21 V
  AP_MAG_out            M_raw                 PSF_INST_MAG_raw 2 -0.21 0.21 V
end

  load.cmf $1 Chip.psf raw
  load.cmf $2 Chip.psf out

  # images generated with convolution will not have the right output positions
  set X_raw = int(X_PSF_raw) + 0.5
  set Y_raw = int(Y_PSF_raw) + 0.5
  set M_raw = PSF_INST_MAG_raw + $4
  set K_out = -2.5*log(KRON_FLUX_out)
  match2d X_PSF_out Y_PSF_out X_PSF_raw Y_PSF_raw 1.5 -index1 index1 -index2 index2

  local i NX NY nx ny N

  resize 1000 1000

  clear
  section a0 0.0 0.0 1.0 0.5
  label -fn courier 14; 
  style -pt 0 -sz 0.4
  show.pair 0
  reindex ap = AP_MAG_out using index1
  set dap = ap - v2
  plot -c red -pt 2 -sz 0.5 rv dap

  delete -q imag_V dmag_V smag_V Dmag_V dmag_Vraw
  for imag -11 -6 0.2
    subset dmsub = delta if (rv > $imag) && (rv < $imag + 0.2)
    vstat -q dmsub
    concat $imag imag_V
    concat $MEAN dmag_V
    concat $MEDIAN Dmag_V
    concat $SIGMA smag_V
  end

  vstat -q dmag_V
  $offset = $MEDIAN
  set dmag_V = dmag_V - $offset
  set Dmag_V = Dmag_V - $offset

  section a1 0.0 0.50 1.0 0.25
  lim rv -0.025 0.075; label -fn courier 14; box; 
  # plot -c black -pt 0 -sz 0.3 MagPredIn dMagIn
  plot -c red   -pt 7 -sz 2.0 imag_V dmag_V
  plot -c darkgreen -pt 3 -sz 2.0 imag_V Dmag_V
  # plot -c gold -pt 4 -sz 2.0 imag_V dmag_Vraw
  plot -c blue  -pt 2 -sz 2.0 imag_V smag_V
  # label -y "mean (red), median (green), unclipped mean (gold), sigma (blue) of delta"
  label -y "mean (red), median (green), sigma (blue) of delta"
  sprintf line "OFFSET: %6.3f" $offset
  textline -frac 0.1 0.8 -fn courier 24 "$line"

  subset dmsub = delta if (rv < -13) && (rv > -16)
  vstat -q dmsub
  sprintf line "STDEV: %6.3f" $SIGMA
  textline -frac 0.1 0.7 -fn courier 24 "$line"


  set lChiNorm = log(PSF_CHISQ_out / PSF_NDOF_out)
  reindex chi = lChiNorm using index1
  section a2 0.0 0.75 1.0 0.25
  label -fn courier 14; 
  lim rv chi; box; 
  plot -c red -pt 0 -sz 0.5 rv chi

  label -y "chisq"
  png -name $3.png

  # section a1 0.0 0.5 1.0 0.5
  # style -pt 0 -sz 0.4
  # show.pair 1
end

macro go.phot
 runphot test.00 test.00.varmode.C "-Db SAVE.RESID T -D LINEAR_FIT_VARIANCE_MODE CONSTANT"
 runphot test.00 test.00.varmode.I "-Db SAVE.RESID T -D LINEAR_FIT_VARIANCE_MODE IMAGE_VAR"
 runphot test.00 test.00.varmode.M "-Db SAVE.RESID T -D LINEAR_FIT_VARIANCE_MODE MODEL_VAR"
 runphot test.00 test.00.varmode.S "-Db SAVE.RESID T -D LINEAR_FIT_VARIANCE_MODE MODEL_SKY"
end

macro go.vars
 dev -n varC; ckchip.mags test.00.in.cmf test.00.varmode.C.cmf test.00.varmode.C 0.0
 dev -n varI; ckchip.mags test.00.in.cmf test.00.varmode.I.cmf test.00.varmode.I 0.0
 dev -n varM; ckchip.mags test.00.in.cmf test.00.varmode.M.cmf test.00.varmode.M 0.0
 dev -n varS; ckchip.mags test.00.in.cmf test.00.varmode.S.cmf test.00.varmode.S 0.0
end

# compare two cmf files with extname Chip.psf 
# things to compare:
# * completeness (which sources in (1) are not detected in (2)
# * positions (X_PSF, Y_PSF) 
# * instrumental psf mags
# * position errors (no input errors; use a model?)
# * measured FWHM?
# * kron mags (fluxes)
# * etc, etc
macro ckchip
  if ($0 != 5)
    echo "USAGE: ckchip (raw) (out) (output) (zpt_off)"
    break
  end

  load.cmf $1 Chip.psf raw
  load.cmf $2 Chip.psf out

  # images generated with convolution will not have the right output positions
  set X_raw = int(X_PSF_raw) + 0.5
  set Y_raw = int(Y_PSF_raw) + 0.5
  set M_raw = PSF_INST_MAG_raw + $4
  set K_out = -2.5*log(KRON_FLUX_out)
  match2d X_PSF_out Y_PSF_out X_PSF_raw Y_PSF_raw 1.5 -index1 index1 -index2 index2

  local i NX NY nx ny N

  device -n compare
  resize 1000 1000

  # plot trends as a function of mag
  $NX = 2
  $NY = 5
  $nx = 0
  $ny = 0
  $N = 0
  clear -s
  for i 0 $pairs:n
    section a$nx\$ny {$nx/$NX} {$ny/$NY} {1/$NX} {1/$NY}
    show.pair $i
    $ny ++
    if ($ny == $NY)
      $ny = 0
      $nx ++
    end
    if ($nx == $NX)
      png -name $3.$N.png
      clear -s
      $nx = 0
      $ny = 0
      $N ++
    end
  end

  # plot (input - output) vs mag
end

macro stchip
  if ($0 != 5)
    echo "USAGE: stchip (raw) (out) (output) (zpt_off)"
    break
  end

  load.cmf $1 Chip.psf raw
  load.cmf $2 Chip.psf out

  # images generated with convolution will not have the right output positions
  set X_raw = int(X_PSF_raw) + 0.5
  set Y_raw = int(Y_PSF_raw) + 0.5
  set M_raw = PSF_INST_MAG_raw + $4
  set K_out = -2.5*log(KRON_FLUX_out)
  match2d X_PSF_out Y_PSF_out X_PSF_raw Y_PSF_raw 1.5 -index1 index1 -index2 index2

  local i

  for i 0 $spairs:n
    stats.pair $i $3
  end
end

# compare chip to warp
macro ckwarp
  if ($0 != 5)
    echo "USAGE: ckwarp (raw) (out) (output) (zpt_off)"
    break
  end

  load.cmf $1 Chip.psf raw
  load.cmf $2 SkyChip.psf out
  set X_raw = int(X_PSF_raw) + 0.5
  set Y_raw = int(Y_PSF_raw) + 0.5
  set M_raw = PSF_INST_MAG_raw + $4
  set K_out = -2.5*log(KRON_FLUX_out)
  match2d X_PSF_out Y_PSF_out X_PSF_raw Y_PSF_raw 1.0 -index1 index1 -index2 index2

  local i nx ny NX NY N

  device -n compare
  resize 1000 1000

  # plot trends as a function of mag
  $NX = 2
  $NY = 5
  $nx = 0
  $ny = 0
  $N = 0
  clear -s

  for i 0 $pairs:n
    section a$nx\$ny {$nx/$NX} {$ny/$NY} {1/$NX} {1/$NY}
    show.pair $i
    $ny ++
    if ($ny == $NY)
      $ny = 0
      $nx ++
    end
    if ($nx == $NX)
      png -name $3.$N.png
      clear -s
      $nx = 0
      $ny = 0
      $N ++
    end
  end
end

macro stwarp
  if ($0 != 5)
    echo "USAGE: stwarp (raw) (out) (output) (zpt_off)"
    break
  end

  load.cmf $1 Chip.psf raw
  load.cmf $2 SkyChip.psf out

  # images generated with convolution will not have the right output positions
  set X_raw = int(X_PSF_raw) + 0.5
  set Y_raw = int(Y_PSF_raw) + 0.5
  set M_raw = PSF_INST_MAG_raw + $4
  set K_out = -2.5*log(KRON_FLUX_out)
  match2d X_PSF_out Y_PSF_out X_PSF_raw Y_PSF_raw 1.5 -index1 index1 -index2 index2

  local i

  for i 0 $spairs:n
    stats.pair $i $3
  end
end

macro stats.pair
  if ($0 != 3)
    echo "USAGE: stats.pair (N) (output)"
    break
  end

  list word -split $spairs:$1
  if ($word:n != 8)
    echo "invalid pair $1"
    break
  end

  $Nr = $word:3

  reindex v1 = $word:0 using index1
  reindex v2 = $word:1 using index2
  reindex rv = $word:2 using index$Nr

  set delta = v1 - v2
  subset d1 = delta if ($word:4 < rv) && (rv < $word:5) && (abs(delta) < $word:7)
  subset d2 = delta if ($word:5 < rv) && (rv < $word:6) && (abs(delta) < $word:7)

  vstats -q d1 -sigma-clip 3.0
  $M1 = $MEAN
  $S1 = $SIGMA
  vstats -q d2 -sigma-clip 3.0
  $M2 = $MEAN
  $S2 = $SIGMA

  output $2
  fprintf "%-18s  %7.4f %7.4f  %7.4f %7.4f" $word:0  $M1 $S1  $M2 $S2
  output stdout
end

macro show.pair
  if ($0 != 2)
    echo "USAGE: show.pair (N)"
    break
  end

  list word -split $pairs:$1
  if ($word:n != 7)
    echo "invalid pair $1"
    break
  end

  $Nr = $word:3

  reindex v1 = $word:0 using index1
  if ("$word:6" == "V") 
    reindex v2 = $word:1 using index2
  end
  if ("$word:6" == "S") 
    set v2 = $word:1 + zero(index1)
  end
  reindex rv = $word:2 using index$Nr

  set delta = v1 - v2
  if (("$word:4" == "def") || ("$word:5" == "def"))
    lim rv delta; box; plot rv delta
  else
    lim rv $word:4 $word:5; box; plot rv delta
  end
  $line = '$word:0' - '$word:1'
  label -y "$line" -x '$word:2'
end

# This list is used to compare a pair of vectors (sans error) or a
# vector and an expected (constant) value.  The last field defines a
# vector or constant for the comparison.  It is assumed that the
# vector sets have been loaded and matched with match2d to generate
# index vectors 'index1' and index2'.  The macro 'show.pair' generates
# a plot of the range vector vs (v1 - v2).  The indices for v1, v2 are
# index1 and 2 respectively.  The index for the range vector is defined
# by the integer following that vector.  the y-limits of the plot are
# given by the last two numbers
list pairs
  X_PSF_out             X_raw                 PSF_INST_MAG_raw 2 -1.01 1.01 V
  Y_PSF_out             Y_raw                 PSF_INST_MAG_raw 2 -1.01 1.01 V
  X_PSF_out             X_PSF_raw             PSF_INST_MAG_raw 2 -1.01 1.01 V
  Y_PSF_out             Y_PSF_raw             PSF_INST_MAG_raw 2 -1.01 1.01 V
  X_PSF_SIG_out         X_PSF_SIG_raw         PSF_INST_MAG_raw 2 -1.01 1.01 V
  Y_PSF_SIG_out         Y_PSF_SIG_raw         PSF_INST_MAG_raw 2 -1.01 1.01 V
  #PSF_INST_MAG_out      PSF_INST_MAG_raw      PSF_INST_MAG_raw 2 -1.01 1.01 V
  PSF_INST_MAG_out      M_raw                 PSF_INST_MAG_raw 2 -1.01 1.01 V
  PSF_INST_MAG_SIG_out  PSF_INST_MAG_SIG_raw  PSF_INST_MAG_raw 2 -1.01 1.01 V
  #PSF_INST_FLUX_out     PSF_INST_FLUX_raw     PSF_INST_MAG_raw 2   def  def V
  #PSF_INST_FLUX_SIG_out PSF_INST_FLUX_SIG_raw PSF_INST_MAG_raw 2   def  def V
  AP_MAG_out            M_raw                 PSF_INST_MAG_raw 2 -1.01 1.01 V
  AP_MAG_RAW_out        M_raw                 PSF_INST_MAG_raw 2 -1.01 1.01 V
  AP_MAG_RADIUS_out     0.0                   PSF_INST_MAG_raw 2 -0.01 20.1 S
  SKY_out               0.0                   PSF_INST_MAG_raw 2   def  def S
  SKY_SIGMA_out         0.0                   PSF_INST_MAG_raw 2   def  def S
  PSF_CHISQ_out         1.0                   PSF_INST_MAG_raw 2   def  def S
  CR_NSIGMA_out         0.0   		      PSF_INST_MAG_raw 2   def  def S
  EXT_NSIGMA_out        0.0   		      PSF_INST_MAG_raw 2 -5.01 5.01 S
  PSF_MAJOR_out         0.0   		      PSF_INST_MAG_raw 2 -0.01 5.01 S
  PSF_MINOR_out         0.0   		      PSF_INST_MAG_raw 2 -0.01 5.01 S
  PSF_THETA_out         0.0   		      PSF_INST_MAG_raw 2 -1.61 1.61 S
  PSF_QF_out            0.0   		      PSF_INST_MAG_raw 2 -0.10 1.10 S
  PSF_QF_PERFECT_out    0.0   		      PSF_INST_MAG_raw 2 -0.10 1.10 S
  PSF_NDOF_out          0.0   		      PSF_INST_MAG_raw 2  def  def  S
  PSF_NPIX_out          0.0   		      PSF_INST_MAG_raw 2  def  def  S
  MOMENTS_XX_out        0.0   		      PSF_INST_MAG_raw 2 -0.01 3.01 S
  MOMENTS_XY_out        0.0   		      PSF_INST_MAG_raw 2 -3.01 3.01 S
  MOMENTS_YY_out        0.0   		      PSF_INST_MAG_raw 2 -0.01 3.01 S
  MOMENTS_M3C_out       0.0   		      PSF_INST_MAG_raw 2 -3.01 3.01 S
  MOMENTS_M3S_out       0.0   		      PSF_INST_MAG_raw 2 -3.01 3.01 S
  MOMENTS_M4C_out       0.0   		      PSF_INST_MAG_raw 2 -2.01 2.01 S
  MOMENTS_M4S_out       0.0   		      PSF_INST_MAG_raw 2 -2.01 2.01 S
  MOMENTS_R1_out        0.0   		      PSF_INST_MAG_raw 2 -5.01 5.01 S
  MOMENTS_RH_out        0.0   		      PSF_INST_MAG_raw 2 -5.01 5.01 S
  K_out                 M_raw  		      PSF_INST_MAG_raw 2  def  def  V
  KRON_FLUX_ERR_out     0.0   		      PSF_INST_MAG_raw 2  def  def  S
  KRON_FLUX_INNER_out   0.0   		      PSF_INST_MAG_raw 2  def  def  S
  KRON_FLUX_OUTER_out   0.0   		      PSF_INST_MAG_raw 2  def  def  S
# CAL_PSF_MAG          CAL_PSF_MAG          none Mraw 2 -1.01 1.01 V
# CAL_PSF_MAG_SIG      CAL_PSF_MAG_SIG      none Mraw 2 -1.01 1.01 V
# RA_PSF               RA_PSF               none Mraw 2 -1.01 1.01 V
# DEC_PSF              DEC_PSF              none Mraw 2 -1.01 1.01 V
# PEAK_FLUX_AS_MAG     PEAK_FLUX_AS_MAG     none Mraw 2 -1.01 1.01 V
# FLAGS                FLAGS                0.0  Mraw 2 -1.01 1.01 S
# FLAGS2               FLAGS2               0.0  Mraw 2 -1.01 1.01 S
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
list fields
  X_PSF              
  Y_PSF              
  X_PSF_SIG          
  Y_PSF_SIG          
  PSF_INST_MAG       
  PSF_INST_MAG_SIG   
  PSF_INST_FLUX      
  PSF_INST_FLUX_SIG  
  AP_MAG             
  AP_MAG_RAW         
  AP_MAG_RADIUS      
  SKY                
  SKY_SIGMA          
  PSF_CHISQ          
  CR_NSIGMA          
  EXT_NSIGMA         
  PSF_MAJOR          
  PSF_MINOR          
  PSF_THETA          
  PSF_QF             
  PSF_QF_PERFECT     
  PSF_NDOF           
  PSF_NPIX           
  MOMENTS_XX         
  MOMENTS_XY         
  MOMENTS_YY         
  MOMENTS_M3C        
  MOMENTS_M3S        
  MOMENTS_M4C        
  MOMENTS_M4S        
  MOMENTS_R1         
  MOMENTS_RH         
  KRON_FLUX          
  KRON_FLUX_ERR      
  KRON_FLUX_INNER    
  KRON_FLUX_OUTER    
#   CAL_PSF_MAG      
#   CAL_PSF_MAG_SIG  
#   RA_PSF           
#   DEC_PSF          
#   PEAK_FLUX_AS_MAG 
#  FLAGS            
#  FLAGS2           
end

# use these cmf entries to measure average stats of the given pairs
list spairs
  X_PSF_out             X_raw                 PSF_INST_MAG_raw 2 -15 -10 -8.5  0.1
  Y_PSF_out             Y_raw                 PSF_INST_MAG_raw 2 -15 -10 -8.5  0.1
  PSF_INST_MAG_out      M_raw                 PSF_INST_MAG_raw 2 -15 -10 -8.5  0.1
  AP_MAG_out            M_raw                 PSF_INST_MAG_raw 2 -15 -10 -8.5  0.1
end

#  # XXX this is a hack: the cmf file created by ppSim is not
#  # compatible with the image for warp (because one is a chip-mosaic
#  # and the other is not)
#  exec rm -f fix.hdr
#  output fix.hdr
#  echo "PSMOSAIC= 'CHIP    '           / Mosaicked level"
#  output stdout 
# fix the header to be compatible with the chip file (to avoid running psphot)
# exec fits_insert $basename.cmf fix.hdr

macro completeness
 if ($0 != 4)
   echo "USAGE: completeness (raw) (out) (output)"
   break
 end

 load.cmf $1 Chip.psf raw
 load.cmf $2 Chip.psf out
 set X_raw = int(X_PSF_raw) + 0.5
 set Y_raw = int(Y_PSF_raw) + 0.5
 match2d X_PSF_raw Y_PSF_raw X_PSF_out Y_PSF_out 1.5 -index1 index1 -index2 index2 -closest

 histogram PSF_INST_MAG_raw nMag -16.0 -3.0 0.25 -range dMag
 set fMag = zero(dMag) 
 for i 0 {dMag[]-1}
  set inrange = (PSF_INST_MAG_raw > dMag[$i]) && (PSF_INST_MAG_raw <= dMag[$i+1])
  subset all = index1 if (inrange)
  subset got = index1 if (inrange) && (index1 >= 0)
  if (all[] == 0)
    fMag[$i] = 0
  else 
   fMag[$i] = got[] / all[]
  end
 end

 device -n complete
 resize 1000 600

 clear -s

 section default 0 0 1 1
 lim dMag fMag; clear; box -ypad 5 +ypad 5 -ticks 1110; plot -x 1 dMag fMag
 label -x mag_inst -y det_frac 

 set found = (index1 >= 0)
 plot PSF_INST_MAG_raw found -c red

 set ldmag = log(PSF_INST_MAG_SIG_raw)
 section overlay 0 0 1 1; lim dMag -5 1.2; box -ypad 5 +ypad 5 -ticks 1011 -labels 1001; plot PSF_INST_MAG_raw ldmag
 label +y log(S/N)

 png -name $3
end

macro show.dpair
  if ($0 != 2)
    echo "USAGE: show.dpair (N)"
    break
  end

  list word -split $dpairs:$1
  if ($word:n != 7)
    echo "invalid dpair $1"
    break
  end

  $Nr = $word:4

  reindex v1 = $word:0 using index1
  reindex dv = $word:1 using index1
  reindex v2 = $word:2 using index2
  reindex rv = $word:3 using index$Nr

  set delta = (v1 - v2) / dv
  if (("$word:5" == "def") || ("$word:6" == "def"))
    lim rv delta; box; plot rv delta
  else
    lim rv $word:5 $word:6; box; plot rv delta
  end
  label -y '$word:0' -x '$word:3'
end

# this list is used to compare a pair of vectors with an error it is
# assumed that the vector sets have been loaded and matched with
# match2d to generate index vectors 'index1' and index2'.  the macro
# show.dpair generates a plot of the range vector vs (v1 - v2) / dv1.
# The indices for v1, dv1, and v2 are index1,1, and 2 respectively.  The
# index for the range vector is defined by the integer following that
# vector.  The y-limits of the plot are given by the last two numbers
# (use 'def') for the full default range of the delta vector
list dpairs
  # v1              dv                v2   range 
  X_PSF             X_PSF_SIG         Xraw Mraw  2 -10.0 10.0
  Y_PSF             Y_PSF_SIG         Yraw Mraw  2 -10.0 10.0
  PSF_INST_MAG      PSF_INST_MAG_SIG  Mraw Mraw  2 -10.0 10.0
  PSF_INST_FLUX     PSF_INST_FLUX_SIG Fraw Mraw  2 -10.0 10.0
  AP_MAG            PSF_INST_MAG_SIG  Mraw Mraw  2 -10.0 10.0
  AP_MAG_RAW        PSF_INST_MAG_SIG  Mraw Mraw  2 -10.0 10.0
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

macro ckradialflux
 data $1
 read -fits Chip.xrad X_APER Y_APER PSF_FWHM APER_FLUX APER_FLUX_ERR APER_FLUX_STDEV APER_FILL
 read -fits Chip.psf X_PSF Y_PSF PSF_INST_MAG
 set mag = PSF_INST_MAG
 # XXX include nradii in header
 for i 0 11
   set mA$i = -2.5*log(APER_FLUX:$i)
   set dM$i = mA$i - PSF_INST_MAG
   vstat dM$i
   $DM$i = $MEDIAN
 end
 for i 1 11
   $j = $i - 1
   echo {$DM$i - $DM$j}
 end

 lim mag -2 2; clear; box
 for i 0 11
   plot mag dM$i -c black
 end
end

macro init
  # config for ppImage to generate chip, mask, weight
  $ppImageConfig = -recipe PPIMAGE PPIMAGE_N
  $ppImageConfig = $ppImageConfig -Db BACKGROUND T
  $ppImageConfig = $ppImageConfig -Db CHIP.FITS T
  $ppImageConfig = $ppImageConfig -Db CHIP.MASK.FITS T
  $ppImageConfig = $ppImageConfig -Db CHIP.VARIANCE.FITS T
  $ppImageConfig = $ppImageConfig -Db BASE.FITS F
  $ppImageConfig = $ppImageConfig -Db VARIANCE.BUILD T
  $ppImageConfig = $ppImageConfig -Db PHOTOM F
  
  # basic options for the these images (filter, location, obstype)
  $BaseOptions = -type OBJECT -filter r -ra 270.70 -dec -23.70 -pa 0.0
  $BaseOptions = $BaseOptions -Df PSASTRO:DVO.GETSTAR.MAX.RHO 50000.0
  
  # PSF.CONVOLVE : if true, we insert delta functions (and optionally
  #                galaxies) and smooth the image with the psf model
  #                (uses a GAUSS regardless of the model). Note that
  #                PSF.CONVOLVE = T is faster than F, but (a) only
  #                allows Gauss models and (b) only yields quantized
  #                locations

  # create an image with fake sources and insert the resulting cmf file into a dvodb
  $RefConfig = -camera SIMTEST 
  $RefConfig = $RefConfig -recipe PPSIM STACKTEST.MAKE 
  $RefConfig = $RefConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref 
  $RefConfig = $RefConfig -Db PSF.CONVOLVE F
  
  # options for the reference image
  $RefOptions = $BaseOptions
  $RefOptions = $RefOptions -exptime 100.0 
  $RefOptions = $RefOptions -seeing 1.0 
  $RefOptions = $RefOptions -skymags 21.0  
  $RefOptions = $RefOptions -D PSF.MODEL PS_MODEL_GAUSS 
  $RefOptions = $RefOptions -Df STARS.DENSITY 10.0 
  $RefOptions = $RefOptions -Df STARS.SIGMA.LIM 0.5

  # basic config for ppSim with randomly distributed stars and NO galaxies
  $RealConfig = -camera SIMTEST 
  $RealConfig = $RealConfig -recipe PPSIM STACKTEST.RUN 
  $RealConfig = $RealConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref
  $RealConfig = $RealConfig -Db STARS.FAKE F
  $RealConfig = $RealConfig -Db STARS.REAL T 
  $RealConfig = $RealConfig -Db MATCH.DENSITY F 
  $RealConfig = $RealConfig -Db PSF.CONVOLVE F
  $RealConfig = $RealConfig -Df STARS.DENSITY 10.0
  $RealConfig = $RealConfig -Df STARS.SIGMA.LIM 1.0
  $RealConfig = $RealConfig -Db GALAXY.FAKE F 
  $RealConfig = $RealConfig -Db GALAXY.GRID F 
  
  # options for the repeated images
  $RealOptions = $BaseOptions -exptime 30.0
    
  # sample alternate options:
  # $ppSimOptions = $FakeOptions -D PSF.MODEL PS_MODEL_PS1_V1
  # $ppSimOptions = $FakeOptions -Df PSF.ARATIO 1.2
  # $ppSimOptions = $FakeOptions -Df PSF.THETA +30.0
  # $ppSimOptions = $FakeOptions -D PSF.MODEL PS_MODEL_GAUSS
  
  list fwhm 
   1.0 
   1.1 
   1.2 
   1.5
  end
end

if ($SCRIPT)
  fulltest 4
  exit 0
end
