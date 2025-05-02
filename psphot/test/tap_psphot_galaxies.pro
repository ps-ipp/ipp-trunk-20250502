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

# options for the simulated images (using the refimage for the stars)
$FakeOptions = $BaseOptions
$FakeOptions = $FakeOptions -exptime 30.0
# $FakeOptions = $FakeOptions -D PSF.MODEL PS_MODEL_PS1_V1
$FakeOptions = $FakeOptions -D PSF.MODEL PS_MODEL_GAUSS
  
# sample alternate options:
# $FakeOptions = $FakeOptions -D PSF.MODEL PS_MODEL_PS1_V1
# $FakeOptions = $FakeOptions -Df PSF.ARATIO 1.2
# $FakeOptions = $FakeOptions -Df PSF.THETA +30.0

# create an image with fake sources (these are then inserted into the catdir)
$RefConfig = -camera SIMTEST
$RefConfig = $RefConfig -recipe PPSIM STACKTEST.MAKE

# basic config for ppSim with randomly distributed stars and gridded galaxies
if (1)
  $FakeConfig = -camera SIMTEST
  $FakeConfig = $FakeConfig -recipe PPSIM STACKTEST.RUN
  $FakeConfig = $FakeConfig -D PSASTRO:PSASTRO.CATDIR catdir.ref
  $FakeConfig = $FakeConfig -Db STARS.FAKE F                         ; # only use stars from catdir.ref
  $FakeConfig = $FakeConfig -Db MATCH.DENSITY F
  $FakeConfig = $FakeConfig -Db PSF.CONVOLVE T
  $FakeConfig = $FakeConfig -Db GALAXY.FAKE T                        ; # generate a "realistic" distribution of galaxies
  $FakeConfig = $FakeConfig -Df GALAXY.MAG 17.0
  $FakeConfig = $FakeConfig -Db GALAXY.GRID F                        ; # generate a grid of galaxies (constant mag)
  #$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_GAUSS
  #$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_EXP
  #$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_SERSIC
  #$FakeConfig = $FakeConfig -D GALAXY.MODEL PS_MODEL_DEV
  $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MIN 10.0
  $FakeConfig = $FakeConfig -Df GALAXY.RMAJOR.MAX  1.0
  $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MIN 0.25
  $FakeConfig = $FakeConfig -Df GALAXY.ARATIO.MAX 1.00
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MIN 0 
  $FakeConfig = $FakeConfig -Df GALAXY.THETA.MAX 180
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MIN 1.66
  $FakeConfig = $FakeConfig -Df GALAXY.INDEX.MAX 1.66
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DX 120
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DY 120
end

# basic config for ppSim with randomly distributed stars and gridded galaxies
if (0) 
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
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DX 120
  $FakeConfig = $FakeConfig -Di GALAXY.GRID.DY 120
end

list fwhm 
 1.0 
 1.1 
 1.2 
 1.5
end

macro go
  mkexp test.exp 1.0 EXP
  fitexp test.exp test.exp.fit EXP_CONV

  mkexp test.ser 1.0 SERSIC
  fitexp test.ser test.ser.fit SER_CONV

  mkexp test.dev 1.0 DEV
  fitexp test.dev test.dev.fit DEV_CONV

  mkexp test.gau 1.0 GAUSS
  fitexp test.gau test.gau.fit GAU_CONV

  mkexp test.pg 1.0 PGAUSS
  fitexp test.pg test.pg.fit PGA_CONV

  mkexp test.qga 1.0 QGAUSS
  fitexp test.qga test.qga.fit QGA_CONV

  mkexp test.p1 1.0 PS1_V1
  fitexp test.p1 test.p1.fit PS1_CONV
end

macro go.ckgalaxy
  foreach type exp ser dev gau pg qga p1
    ckgalaxy test.$type.dat test.$type.fit.cmf
    wait $type
  end
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
  $psphotConfig = $psphotConfig -D  PSPHOT:PSF_MODEL PS_MODEL_PS1_V1
  # $psphotConfig = $psphotConfig -D  PSPHOT:PSF_MODEL PS_MODEL_GAUSS
  $psphotConfig = $psphotConfig -D  PSPHOT:EXTENDED_SOURCE_MODELS_SELECTION $fitModel
  # $psphotConfig = $psphotConfig -D  PSPHOT:OUTPUT.FORMAT PS1_V5
  $psphotConfig = $psphotConfig -D  PSPHOT:OUTPUT.FORMAT PS1_SV3
  $psphotConfig = $psphotConfig -Db PSPHOT:LENSING_PARAMETERS T

  # ppImage / psphot on the output
  echo ppImage $ppImageConfig $psphotConfig -file $basename.fits $outname
  exec ppImage $ppImageConfig $psphotConfig -file $basename.fits $outname
end

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

macro ck.poserrors
  reindex X_raw_m = X_raw using index2
  reindex X_out_m = X_PSF_out using index1
  reindex dX_out_m = X_PSF_SIG_out using index1
  set Xoff = X_raw_m - X_out_m
  reindex M_raw_m = PSF_INST_MAG_raw using index2

  # lim M_raw_m Xoff; clear; box; plot M_raw_m Xoff
  set dC = Xoff^2 / dX_out_m^2
  lim M_raw_m dX_out_m; clear; box; plot M_raw_m dX_out_m
end

macro ckgalaxy
  if ($0 != 3)
    echo "USAGE: ckgalaxy (dat) (cmf)"
    break
  end

  data $1
  read Xin_all 1 Yin_all 2 Type 4 Min_all 5 RmajIn_all 7 RminIn_all 8 ThetaIn_all 9

  subset Xin = Xin_all if (Type == 1)
  subset Yin = Yin_all if (Type == 1)
  subset Min = Min_all if (Type == 1)
  subset Tin = ThetaIn_all if (Type == 1)

  subset RmajIn = RmajIn_all if (Type == 1)
  subset RminIn = RminIn_all if (Type == 1)

  data $2
  read -fits Chip.xfit X_EXT Y_EXT EXT_INST_MAG EXT_WIDTH_MAJ EXT_WIDTH_MIN EXT_THETA
  set EXT_THETA_ALT = EXT_THETA * (EXT_THETA >= 0.0) + (EXT_THETA + 3.14159265) * (EXT_THETA < 0.0)
  set EXT_THETA = EXT_THETA_ALT
  
  match2d X_EXT Y_EXT Xin Yin 1.0 -index1 index1 -index2 index2

  reindex Xot_m = X_EXT using index1
  reindex Yot_m = Y_EXT using index1

  reindex Xin_m = Xin using index2
  reindex Yin_m = Yin using index2

  set dX = Xin_m - Xot_m  
  set dY = Yin_m - Yot_m  
  # lim -n 0 dX dY; clear; box; plot dX dY

  reindex Mot_m = EXT_INST_MAG using index1
  reindex Tot_m = EXT_THETA using index1

  reindex Min_m = Min using index2
  reindex Tin_m = Tin using index2

  reindex Rot_m = EXT_WIDTH_MAJ using index1
  reindex rot_m = EXT_WIDTH_MIN using index1

  reindex Rin_m = RmajIn using index2
  reindex rin_m = RminIn using index2

  set n = ramp(Xin_m)

  set dM = Min_m - Mot_m  
  set dT = (Tin_m - Tot_m) * 180 / 3.14159265
  set Tin_deg = Tin_m * 180 / 3.14159265

  lim -n 0 Tin_deg -5 5; clear; box; plot Tin_deg dT; label -y "delta theta (deg)"
  lim -n 1 n dM; clear; box; plot n dM; label -y "delta mag"
  lim -n 2 n -5 5; clear; box; plot n dT; label -y "delta theta (deg)"
  lim -n 3 -5 5 dM; clear; box; plot dT dM; label -x "delta theta (deg)" -y "delta mag"

  set dR = Rin_m - Rot_m  
  set dr = rin_m - rot_m
  lim -n 4 n dR; clear; box; plot n dR; label -y "delta Rmaj"
  lim -n 5 n dr; clear; box; plot n dr; label -y "delta Rmin"
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
  label -y '$word:0' -x '$word:2'
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

macro plot.angles
  if ($0 != 2)
    echo "USAGE: plot.angles (file.dat)"
    break
  end

  data $1
  read x 1 y 2 type 4 trad 9 
  set t = trad * 180 / 3.14159265
  subset xs = x if (type == 1)
  subset ys = y if (type == 1)
  subset ts = t if (type == 1)

  set cs = dcos(ts)
  set sn = dsin(ts)

  delete xp yp
  erase red
  for i 0 xs[]
    point red LINE xs[$i] ys[$i] {10*cs[$i]} {10*sn[$i]}
  end
  plot xp yp -pt 100
end

macro check.flux
  if ($0 != 2)
    echo "USAGE: plot.angles (file.dat)"
    break
  end

  data $1
  read f 3 type 4
  subset F = f if (type == 1)
  set M = -2.5*log(F)
  vstat M
end

if ($SCRIPT)
  echo "no default action defined"
  exit 0
end
