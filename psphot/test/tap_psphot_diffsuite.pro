#!/usr/bin/env mana
# -*-sh-*-

if (not($?TESTING)) set TESTING = 0

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
$RealConfig = $RealConfig -Db STARS.FAKE F -Db MATCH.DENSITY F -Db PSF.CONVOLVE F

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

# sample alternate options:
# $ppSimOptions = $FakeOptions -D PSF.MODEL PS_MODEL_PS1_V1
# $ppSimOptions = $FakeOptions -Df PSF.ARATIO 1.2
# $ppSimOptions = $FakeOptions -Df PSF.THETA +30.0
$ppSimOptions = $FakeOptions -D PSF.MODEL PS_MODEL_GAUSS

list params
 1.00   1.00   1.0     0.0
 1.10   1.10   1.0     0.0
 1.25   1.25   1.0     0.0
 1.40   1.40   1.0     0.0
 1.00   1.00   1.3    30.0
 1.10   1.10   1.3    30.0
 1.25   1.25   1.3    30.0
 1.40   1.40   1.3    30.0
 1.00   1.00   1.3   -30.0
 1.10   1.10   1.3   -30.0
 1.25   1.25   1.3   -30.0
 1.40   1.40   1.3   -30.0
 1.00   1.25   1.0     0.0
 1.10   1.25   1.0     0.0
 1.25   1.25   1.0     0.0
 1.40   1.25   1.0     0.0
 1.00   1.25   1.3    30.0
 1.10   1.25   1.3    30.0
 1.25   1.25   1.3    30.0
 1.40   1.25   1.3    30.0
 1.00   1.25   1.3   -30.0
 1.10   1.25   1.3   -30.0
 1.25   1.25   1.3   -30.0
 1.40   1.25   1.3   -30.0
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

# create a set of input exposures using the range of parameters given in the lists
macro mkdiffsuite
  if ($0 != 3)
    echo "USAGE: mkdiffsuite (rawbase) (diffbase)"
    break
  end

  local i1 i2 rawbase diffbase
  $rawbase = $1
  $diffbase = $2

  for i1 0 $params:n
    for i2 {$i1 + 1} $params:n
      mkdiff $rawbase $diffbase.$i1.$i2 $i1 $i2
    end
  end
end

# create diff for a single pair of input
macro mkdiff
  if ($0 != 5)
    echo "USAGE: mkdiff (rawbase) (diffbase) (in1) (in2)"
    break
  end

  # XXX check for local reference db and run mkref if not found

  local in1 in2 diff

  $base = $1

  $diff = $2
  dirname $diff -var dir  
  mkdir $dir

  $in1 = $3
  $in2 = $4

  $command = ppSub

  $command = $command -inimage $base.$in1.wrp.fits
  $command = $command -inmask $base.$in1.wrp.mask.fits
  $command = $command -invariance $base.$in1.wrp.wt.fits
  $command = $command -insources $base.$in1.wrp.cmf

  $command = $command -refimage $base.$in2.wrp.fits
  $command = $command -refmask $base.$in2.wrp.mask.fits
  $command = $command -refvariance $base.$in2.wrp.wt.fits
  $command = $command -refsources $base.$in2.wrp.cmf

# no photometry for now
# $command = $command  -recipe PSPHOT DIFF
# $command = $command  -F PSPHOT.PSF.SAVE PSPHOT.PSF.SKY.SAVE
# $command = $command  -F PSPHOT.OUTPUT PSPHOT.OUT.CMF.MEF
# $command = $command  -F PSPHOT.BACKMDL PSPHOT.BACKMDL.MEF -D PSPHOT:OUTPUT.FORMAT PS1_DV2

  $command = $command  -Db DUAL T -Db INVERSE T -convolve 1 -Df PENALTY 1000000 -Di SPATIAL.ORDER 1
  # $command = $command  -Db DUAL T -Db INVERSE T -convolve 1 -Df PENALTY 3000000 -Di SPATIAL.ORDER 1
  # $command = $command  -Db DUAL T -Db INVERSE T -convolve 1 -Df PENALTY 3000000000 -Di SPATIAL.ORDER 1
  $command = $command -Di KERNEL.SIZE 5 -Di STAMP.FOOTPRINT 5

  $command = $command  -save-inconv -save-refconv
  $command = $command  -threads 4
  $command = $command  -image_id 1 -source_id 1

  echo $command $diff
  if ($TESTING)
    exec $command $diff -visual ppsub 10
  else
    exec $command $diff >& $diff.log
  end
end

# create a set of input exposures using the range of parameters given in the lists
macro mkexpsuite
  if ($0 != 2)
    echo "USAGE: mkexpsuite (rawbase)"
    break
  end

  local i base 

  $base = $1

  dirname $base -var dir  
  mkdir $dir

  for i 0 $params:n
    list words -split $params:$i
    mkexp $base.$i $words:0 $words:1 $words:2 $words:3
  end
end

# create a realistic distribution of fake stars, GAUSS PSF
macro mkexp
  if ($0 != 6)
    echo "USAGE: mkexp basename fwhmMin fwhmMax aratio theta"
    break
  end

  local fwhm basename
  $basename = $1
  $fwhmMin = $2
  $fwhmMax = $3
  $aratio = $4
  $theta = $5

  echo ppSim $ppSimOptions $RealConfig $basename -seeing $fwhmMin -seeing-max $fwhmMax -seeing-ramp -Df PSF.ARATIO $aratio -Df PSF.THETA $theta
  exec ppSim $ppSimOptions $RealConfig $basename -seeing $fwhmMin -seeing-max $fwhmMax -seeing-ramp -Df PSF.ARATIO $aratio -Df PSF.THETA $theta
  exec /bin/mv -f $basename.cmf $basename.in.cmf

  # create the chip output
  echo ppImage $ppImageConfig -file $basename.fits $basename
  exec ppImage $ppImageConfig -file $basename.fits $basename

  # XXX pswarp is using all 7k+ sources to measure the PSF : can we reduce this?
  echo pswarp -threads 4 -astrom $basename.cmf -file $basename.ch.fits -mask $basename.ch.mk.fits -variance $basename.ch.wt.fits $basename.wrp refimage.fits
  exec pswarp -threads 4 -astrom $basename.cmf -file $basename.ch.fits -mask $basename.ch.mk.fits -variance $basename.ch.wt.fits $basename.wrp refimage.fits -D PSPHOT:OUTPUT.FORMAT PS1_V2
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

macro ckvar
  if ($0 != 2)
    echo "ckvar (buffer)"
    break
  end

  cursor
  stats $1 {$X1-10} {$Y1-10} 21 21
  $K2sum0 = $TOTAL
  stats $1 {$X2-10} {$Y2-10} 21 21
  $K2sum1 = $TOTAL
  echo {$K2sum0 * 193 + $K2sum1 * 193}
end

macro ckresid

  rd res resid.fits 
  rd con1 conv1.fits 
  rd con2 conv2.fits 
  dev -n im; tvch 1; tv res -50 100; tvch 2; tv con1 -20 100; tvch 3; tv con2 -20 100
end

macro ckchisq
 data chisq2.dat 
 read flux2 3 Mxx 9 Myy 11 chisq 13 npix 15
 set chisqv = chisq / npix
 set flux = sqrt(flux2)

 set lflux = log(flux)
 set lchi = log(chisqv)
 set lMxx = log(Mxx)

 set dev = sqrt(chisq)
 set ndev = sqrt(chisq)/npix
 set lndev = log(ndev)

 lim -n 0 lchi lMxx; clear; box; plot lchi lMxx
 lim -n 1 lflux lMxx; clear; box; plot lflux lMxx
 lim -n 2 lflux lchi; clear; box; plot lflux lchi
 lim -n 3 lflux lndev; clear; box; plot lflux lndev

 # set flux2 = flux
 # set flux = sqrt(flux2)
 # set dev = sqrt(chisq)
 # set ndev = sqrt(chisq)/npix
 # set lndev = log(ndev)
 # set lflux = log(flux)
 # lim -n 0 lflux lndev; clear; box; plot lflux lndev
 # lim -n 1 lndev lMxx; clear; box; plot lndev lMxx

end

macro ckscore
  if ($0 != 3)
    echo "USAGE: ckscore (diffbase) (output)"
    break
  end

  local i j diffbase
  $diffbase = $1

  output $2
  for i 0 $params:n
    for j {$i+1} $params:n
      $line = `grep applying $diffbase.$i.$j.log`
      list word -split $line
      $order = $word:2
      $mode = $word:4
      $val = `grep chisq: $diffbase.$i.$j.log | head -n 1`
      sprintf chisq1 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
      $val = `grep chisq: $diffbase.$i.$j.log | head -n 2 | tail -n 1`
      sprintf chisq2 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
      $val = `grep chisq: $diffbase.$i.$j.log | head -n 3 | tail -n 1`
      sprintf chisq3 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
      echo "$order $mode   $chisq1   $chisq2   $chisq3  $params:$i  $params:$j"
    end
  end
  output stdout
end

macro ckslope
  if ($0 != 3)
    echo "USAGE: ckslope (diffbase) (output)"
    break
  end

  local i j diffbase
  $diffbase = $1

  output $2
  for i 0 $params:n
    for j {$i+1} $params:n
      $line = `grep applying $diffbase.$i.$j.log`
      list word -split $line
      $order = $word:2
      $mode = $word:4
      if ($mode == 1)
        $val = `grep chisq: $diffbase.$i.$j.log | head -n 1`
        sprintf chisq1 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
        $val = `grep chisq: $diffbase.$i.$j.log | head -n 4 | tail -n 1`
        sprintf chisq2 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
      end
      if ($mode == 2)
        $val = `grep chisq: $diffbase.$i.$j.log | head -n 2 | tail -n 1`
        sprintf chisq1 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
        $val = `grep chisq: $diffbase.$i.$j.log | head -n 5 | tail -n 1`
        sprintf chisq2 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
      end
      if ($mode == 4)
        $val = `grep chisq: $diffbase.$i.$j.log | head -n 3 | tail -n 1`
        sprintf chisq1 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
        $val = `grep chisq: $diffbase.$i.$j.log | head -n 6 | tail -n 1`
        sprintf chisq2 "%s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f  %s %6.3f" $val
      end
      echo "$order $mode   $chisq1   $chisq2   $params:$i  $params:$j"
    end
  end
  output stdout
end

macro plotscore
 if ($0 != 2)
   echo "USAGE: plotscore (input)"
   break
 end

 data $1 
 read x1 4 f1 10 x2 14 f2 20 xd 24 fd1 30 fd2 32
 set X1 = 2 * x1 / (1 + f1)
 set X2 = 2 * x2 / (1 + f2)
 set Xd = 2 * xd / (fd1 + fd2)

 subset X1_1 = X1 if (X1 < X2) && (X1 < Xd)
 subset X2_1 = X2 if (X1 < X2) && (X1 < Xd)
 subset Xd_1 = Xd if (X1 < X2) && (X1 < Xd)

 subset X1_2 = X1 if (X1 > X2) && (X2 < Xd)
 subset X2_2 = X2 if (X1 > X2) && (X2 < Xd)
 subset Xd_2 = Xd if (X1 > X2) && (X2 < Xd)

 subset X1_d = X1 if (Xd < X2) && (X1 > Xd)
 subset X2_d = X2 if (Xd < X2) && (X1 > Xd)
 subset Xd_d = Xd if (Xd < X2) && (X1 > Xd)

 lim -n 0 0.3 1.1 0.3 1.1; clear; box
 line 0 0 to 2 2

 plot X1_1 X2_1 -pt 0 -c blue -sz 0.5
 plot X1_2 X2_2 -pt 2 -c red
 plot X1_d X2_d -pt 7 -c black

 lim -n 1 0.3 1.1 0.3 1.1; clear; box
 line 0 0 to 2 2

 plot X1_1 Xd_1 -pt 0 -c blue -sz 0.5
 plot X1_2 Xd_2 -pt 2 -c red
 plot X1_d Xd_d -pt 7 -c black

 lim -n 2 0.3 1.1 0.3 1.1; clear; box
 line 0 0 to 2 2

 plot X2_1 Xd_1 -pt 0 -c blue -sz 0.5
 plot X2_2 Xd_2 -pt 2 -c red
 plot X2_d Xd_d -pt 7 -c black

end

if ($SCRIPT)
  # mkexpsuite image/image
  mkdiffsuite image/image diff.wt4/diff
  exit 0
end

