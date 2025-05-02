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
$BaseOptions = -type OBJECT -filter r -skymags 20.86 -ra 270.70 -dec -23.70 -pa 0.0
$BaseOptions = $BaseOptions -Df PSASTRO:DVO.GETSTAR.MAX.RHO 50000.0

# options for the reference image
$RefOptions = $BaseOptions -exptime 100.0 -seeing 1.0 -D PSF.MODEL PS_MODEL_GAUSS -Df STARS.DENSITY 10.0 -Df STARS.SIGMA.LIM 0.5

# options for the repeated images
$FakeOptions = $BaseOptions -exptime 30.0
  
# basic config for ppSim with randomly distributed stars and gridded galaxies
$RealConfig = -camera SIMTEST -recipe PPSIM STACKTEST.RUN -D PSASTRO:PSASTRO.CATDIR catdir.ref
$RealConfig = $RealConfig -Db STARS.FAKE F -Db MATCH.DENSITY F -Db PSF.CONVOLVE T
$RealConfig = $RealConfig -Db GALAXY.FAKE T -Df GALAXY.MAG 17.0
$RealConfig = $RealConfig -Db GALAXY.GRID T -D GALAXY.MODEL PS_MODEL_SERSIC
$RealConfig = $RealConfig -Df GALAXY.ARATIO.MIN 0.5
$RealConfig = $RealConfig -Df GALAXY.ARATIO.MAX 0.5
$RealConfig = $RealConfig -Df GALAXY.THETA.MAX 180
$RealConfig = $RealConfig -Df GALAXY.INDEX.MIN 1
$RealConfig = $RealConfig -Df GALAXY.INDEX.MAX 1
$RealConfig = $RealConfig -Di GALAXY.GRID.DX 120
$RealConfig = $RealConfig -Di GALAXY.GRID.DY 120

# sample alternate options:
# $ppSimOptions = $FakeOptions -D PSF.MODEL PS_MODEL_PS1_V1
# $ppSimOptions = $FakeOptions -Df PSF.ARATIO 1.2
# $ppSimOptions = $FakeOptions -Df PSF.THETA +30.0
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

  mkref 

  local i Npass imname stname

  $Npass = $1

  $imname = image.fg
  $stname = stack.fg

  for i 0 $Npass
    sprintf N "%02d" $i
    onestack $N
  end
end

macro summary.stats
  if ($0 != 2)
    echo "USAGE: summary.stats (Npass)"
    break
  end

  local i Npass base stack imname stname

  $Npass = $1

  exec /bin/rm -f psf.image.summary.stats 
  exec /bin/rm -f psf.stack.inputs.summary.stats
  exec /bin/rm -f psf.stack.conv.summary.stats
  exec /bin/rm -f psf.stack.unconv.summary.stats

  $imname = image.fg
  $stname = stack.fg

  for i 0 $Npass
    sprintf base  "$imname.%02d/$imname.stats" $i
    sprintf stack "$stname.%02d/$stname" $i
    echo $i $base $stack
    exec grep PSF_INST_MAG $base                   >> psf.image.summary.stats
    exec grep PSF_INST_MAG $stack.conv.stats       >> psf.stack.inputs.summary.stats
    exec grep PSF_INST_MAG $stack.pht.stats        >> psf.stack.conv.summary.stats
    exec grep PSF_INST_MAG $stack.unconv.pht.stats >> psf.stack.unconv.summary.stats
  end
end

# create a reference database of fake stars to be used by ppSim below
macro mkref
  exec rm -rf catdir.ref
  exec rm -f refimage.fits
  
  # create an image with fake sources and insert the resulting cmf file into a dvodb
  $RefConfig = -camera SIMTEST -recipe PPSIM STACKTEST.MAKE -D PSASTRO:PSASTRO.CATDIR catdir.ref

  exec ppSim $RefOptions $RefConfig refimage
  
  file synth.photcodes found
  if (not($found))
    echo "making photcodes file"
    mkphotcodes synth.photcodes
  end

  exec addstar -D CAMERA simtest -D CATDIR catdir.ref -accept-astrom -photcode SYNTH.r -D PHOTCODE_FILE synth.photcodes refimage.cmf
  exec relphot -averages -D CATDIR catdir.ref -update -region 260 280 -33 -13
end

macro onestack
  if ($0 != 2)
    echo "USAGE: onestack (N)"
    break
  end

  mkstack image.$1/image stack.$1/stack
  ckstack image.$1/image stack.$1/stack
  rmstack image.$1/image stack.$1/stack
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkstack
  if ($0 != 3)
    echo "USAGE: mkstack (rawbase) (stackbase)"
    break
  end

  local i base stack

  $base = $1
  $stack = $2

  dirname $base -var dir  
  mkdir $dir

  dirname $stack -var dir  
  mkdir $dir

  exec echo "INPUT MULTI" > $stack.mdc
  exec echo "" >> $stack.mdc

  for i 0 $fwhm:n
    mkexp $base.$i $fwhm:$i

    exec echo "INPUT METADATA" >> $stack.mdc
    exec echo "IMAGE STR $base.$i.wrp.fits" >> $stack.mdc
    exec echo "MASK STR $base.$i.wrp.mask.fits" >> $stack.mdc
    exec echo "VARIANCE STR $base.$i.wrp.wt.fits" >> $stack.mdc
    exec echo "PSF STR $base.$i.wrp.psf" >> $stack.mdc
    exec echo "SOURCES STR $base.$i.wrp.cmf" >> $stack.mdc
    exec echo "END" >> $stack.mdc
    exec echo "" >> $stack.mdc
  end

  exec ppStack -threads 4 -input $stack.mdc $stack -Db TEMP.DELETE F

  basename $stack -var stackbase
  for i 0 $fwhm:n
    exec /bin/cp -f /tmp/$stackbase.$i.conv.im.fits $stack.$i.conv.im.fits
    exec /bin/cp -f /tmp/$stackbase.$i.conv.var.fits $stack.$i.conv.var.fits
    exec /bin/cp -f /tmp/$stackbase.$i.conv.mk.fits $stack.$i.conv.mk.fits
  end  

  # XXX note that the output convolved variance is missing the covariance
  for i 0 $fwhm:n
    exec psphot -file $stack.$i.conv.im.fits -mask $stack.$i.conv.mk.fits $stack.$i.conv
  end  

  # basic photometry for the stack and unconvolved stack
  exec psphot -file $stack.fits -mask $stack.mask.fits -variance $stack.weight.fits $stack.pht
  exec psphot -file $stack.unconv.fits -mask $stack.unconv.mask.fits -variance $stack.unconv.wt.fits $stack.unconv.pht
end

# run this on the files created with mkstack
macro ckstack
  if ($0 != 3)
    echo "USAGE: mkstack (rawbase) (stackbase)"
    break
  end

  local i ZPT base stack

  $base = $1
  $stack = $2

  # generate plots and stats for the per-image chip analysis
  # and the psf-matched convolved images
  $ZPT = -2.5*log($fwhm:n)
  for i 0 $fwhm:n
    ckchip $base.$i.in.cmf $base.$i.cmf       $base.$i       0.0
    ckwarp $base.$i.in.cmf $stack.$i.conv.cmf $stack.$i.conv $ZPT

    stchip $base.$i.in.cmf $base.$i.cmf       $base.stats       0.0
    stwarp $base.$i.in.cmf $stack.$i.conv.cmf $stack.conv.stats $ZPT

    completeness $base.$i.in.cmf $base.$i.cmf $base.$i.complete.png
  end

  ckwarp $base.$i.in.cmf $stack.pht.cmf        $stack.pht        $ZPT
  ckwarp $base.$i.in.cmf $stack.unconv.pht.cmf $stack.unconv.pht $ZPT

  stwarp $base.$i.in.cmf $stack.pht.cmf        $stack.pht.stats  $ZPT
  stwarp $base.$i.in.cmf $stack.unconv.pht.cmf $stack.unconv.pht.stats $ZPT
end

macro rmstack
  if ($0 != 3)
    echo "USAGE: mkstack (rawbase) (stackbase)"
    break
  end

  local base stack
  $base = $1
  $stack = $2

  for i 0 $fwhm:n
    exec rm -f $base.$i.fits
    exec rm -f $base.$i.ch.fits
    exec rm -f $base.$i.ch.mk.fits
    exec rm -f $base.$i.ch.wt.fits

    exec rm -f $base.$i.wrp.fits
    exec rm -f $base.$i.wrp.mask.fits
    exec rm -f $base.$i.wrp.wt.fits

    exec rm -f $stack.$i.conv.im.fits
    exec rm -f $stack.$i.conv.mk.fits
    exec rm -f $stack.$i.conv.wt.fits
  end

  exec rm -f $stack.fits
  exec rm -f $stack.mask.fits
  exec rm -f $stack.weight.fits

  exec rm -f $stack.exp.fits
  exec rm -f $stack.num.fits
  exec rm -f $stack.expwt.fits

  exec rm -f $stack.unconv.fits
  exec rm -f $stack.unconv.mask.fits
  exec rm -f $stack.unconv.wt.fits

  exec rm -f $stack.unconv.exp.fits
  exec rm -f $stack.unconv.num.fits
  exec rm -f $stack.unconv.expwt.fits
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkexp
  if ($0 != 3)
    echo "USAGE: mkexp basename fwhm"
    break
  end

  local fwhm basename
  $basename = $1
  $fwhm = $2

  # create the raw image
  echo ppSim $ppSimOptions $RealConfig $basename -seeing $fwhm
  exec ppSim $ppSimOptions $RealConfig $basename -seeing $fwhm
  exec /bin/mv -f $basename.cmf $basename.in.cmf

  # create the chip output
  echo ppImage $ppImageConfig -file $basename.fits $basename
  exec ppImage $ppImageConfig -file $basename.fits $basename

  # XXX pswarp is using all 7k+ sources to measure the PSF : can we reduce this?
  echo pswarp -threads 4 -astrom $basename.cmf -file $basename.ch.fits -mask $basename.ch.mk.fits -variance $basename.ch.wt.fits $basename.wrp refimage.fits
  exec pswarp -threads 4 -astrom $basename.cmf -file $basename.ch.fits -mask $basename.ch.mk.fits -variance $basename.ch.wt.fits $basename.wrp refimage.fits

  echo psphot -file $basename.wrp.fits -mask $basename.wrp.mask.fits -variance $basename.wrp.wt.fits  $basename.wrp.pht
  exec psphot -file $basename.wrp.fits -mask $basename.wrp.mask.fits -variance $basename.wrp.wt.fits  $basename.wrp.pht
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

if ($SCRIPT)
  fulltest 4
  exit 0
end
