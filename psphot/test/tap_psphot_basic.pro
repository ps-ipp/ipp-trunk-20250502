#!/usr/bin/env mana 
# -*-sh-*-

# macros to support tap tests in mana/dvo
input tap.pro

# create a fake image (ppSim, all stars are fake)
macro make.fake

  local options

  # basic options for these image (r, 100sec)
  $options = -type OBJECT -filter r -exptime 100.0
  $options = $options -skymags 20.86 -ra 270.70 -dec -23.70 -pa 0.0 -seeing 1.0
  $options = $options -D PSF.MODEL PS_MODEL_GAUSS
  
  $options = $options -Df STARS.DENSITY 20.0

  $options = $options -camera SIMTEST -recipe PPSIM STACKTEST.MAKE
  exec ppSim $options testimage
end

macro run.psphot
  exec psphot -file testimage.fits testimage.phot
end

macro test.init
  data testimage.phot.cmf

  # a bit hackish : cannot append to non-existent scalar
  $fields = Chip.psf
  for i 0 $pairs:n
    list word -split $pairs:$i
    $fields = $fields $word:0
  end

  read -fits $fields

  data testimage.dat
  read Xraw 1 Yraw 2 Fraw 3 Galaxy 4 Mraw 5 dMraw 6 SXXraw 7 SYYraw 8 SXYraw 9 S7raw 10
  set dFraw = Fraw * dMraw
  set dXraw = 4.0 * dMraw / 2.35
  set dYraw = 4.0 * dMraw / 2.35

  match2d X_PSF Y_PSF Xraw Yraw 2.0 -index1 index1 -index2 index2
end

macro test.plots
  if ($0 != 2)
    echo "USAGE: test.plots (outroot)"
    break
  end

  clear -s 

  resize 1200 1175

  $nx = 0
  $ny = 0
  $np = 0
  for i 0 $pairs:n
    section a$i {0.5*$nx} {0.8 - 0.2*$ny} 0.5 0.2
    show.pair $i
    $ny ++
    if ($ny == 5) 
      $nx ++
      $ny = 0
    end
    if ($nx == 2)
      png -name $1.$np.png
      clear -s
      $np ++
      $nx = 0
    end
  end  
  png -name $1.$np.png
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
  label -y $word:0 -x $word:3
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
  label -y $word:0 -x $word:3
end

macro show.pair.old
  set delta = $1 - $2
  lim mag2 delta; clear; box; plot mag2 delta
end

# list the entries that should be compared and the vector to use as a range
list dpairs
  X_PSF      	    X_PSF_SIG 	      Xraw Mraw 2 -10.0 10.0
  Y_PSF 	    Y_PSF_SIG 	      Yraw Mraw 2 -10.0 10.0
  PSF_INST_MAG 	    PSF_INST_MAG_SIG  Mraw Mraw 2 -10.0 10.0
  PSF_INST_FLUX	    PSF_INST_FLUX_SIG Fraw Mraw 2 -10.0 10.0
  AP_MAG  	    PSF_INST_MAG_SIG  Mraw Mraw 2 -10.0 10.0
  AP_MAG_RAW	    PSF_INST_MAG_SIG  Mraw Mraw 2 -10.0 10.0
end

# list the entries that should be compared and the vector to use as a range
list pairs
  X_PSF      	     Xraw Mraw 2 -1.01 1.01 V
  Y_PSF 	     Yraw Mraw 2 -1.01 1.01 V
  X_PSF_SIG 	    dXraw Mraw 2 -1.01 1.01 V
  Y_PSF_SIG 	    dYraw Mraw 2 -1.01 1.01 V
  PSF_INST_MAG 	     Mraw Mraw 2 -1.01 1.01 V
  PSF_INST_MAG_SIG    0.0 Mraw 2 -0.01 1.01 S
  PSF_INST_FLUX	     Fraw Mraw 2   def  def V
  PSF_INST_FLUX_SIG dFraw Mraw 2   def  def V
  AP_MAG  	     Mraw Mraw 2 -1.01 1.01 V
  AP_MAG_RAW	     Mraw Mraw 2 -1.01 1.01 V
  AP_MAG_RADIUS      0.0  Mraw 2 -0.01 20.1 S
  SKY     	     0.0  Mraw 2   def  def S
  SKY_SIGMA	     0.0  Mraw 2   def  def S
  PSF_CHISQ	     1.0  Mraw 2   def  def S
  CR_NSIGMA	     0.0  Mraw 2   def  def S
  EXT_NSIGMA	     0.0  Mraw 2 -5.01 5.01 S
  PSF_MAJOR	     0.0  Mraw 2 -0.01 5.01 S
  PSF_MINOR	     0.0  Mraw 2 -0.01 5.01 S
  PSF_THETA	     0.0  Mraw 2 -1.61 1.61 S
  PSF_QF  	     0.0  Mraw 2 -0.10 1.10 S
  PSF_QF_PERFECT     0.0  Mraw 2 -0.10 1.10 S
  PSF_NDOF	     0.0  Mraw 2  def  def  S
  PSF_NPIX	     0.0  Mraw 2  def  def  S
  MOMENTS_XX	     0.0  Mraw 2 -0.01 3.01 S
  MOMENTS_XY	     0.0  Mraw 2 -3.01 3.01 S
  MOMENTS_YY	     0.0  Mraw 2 -0.01 3.01 S
  MOMENTS_M3C	     0.0  Mraw 2 -3.01 3.01 S
  MOMENTS_M3S	     0.0  Mraw 2 -3.01 3.01 S
  MOMENTS_M4C	     0.0  Mraw 2 -2.01 2.01 S
  MOMENTS_M4S	     0.0  Mraw 2 -2.01 2.01 S
  MOMENTS_R1	     0.0  Mraw 2 -5.01 5.01 S
  MOMENTS_RH	     0.0  Mraw 2 -5.01 5.01 S
  KRON_FLUX	     0.0  Mraw 2  def  def  S
  KRON_FLUX_ERR	     0.0  Mraw 2  def  def  S
  KRON_FLUX_INNER    0.0  Mraw 2  def  def  S
  KRON_FLUX_OUTER    0.0  Mraw 2  def  def  S
#   CAL_PSF_MAG	     none Mraw 2 -1.01 1.01 V
#   CAL_PSF_MAG_SIG  none Mraw 2 -1.01 1.01 V
#   RA_PSF  	     none Mraw 2 -1.01 1.01 V
#   DEC_PSF 	     none Mraw 2 -1.01 1.01 V
#   PEAK_FLUX_AS_MAG none Mraw 2 -1.01 1.01 V
#  FLAGS   	     0.0  Mraw 2 -1.01 1.01 S
#  FLAGS2             0.0  Mraw 2 -1.01 1.01 S
end

if ($SCRIPT) 
  make.fake
  exit 0
end


  # write tmp.dat X_PSF Y_PSF X_PSF_SIG Y_PSF_SIG PSF_INST_MAG PSF_INST_MAG_SIG
  # exec gcompare -m tmp.dat 1 2 testimage.dat 1 2 2.0 > tmp.match.dat
  # 
  # data tmp.match.dat
  # read x1 1 y1 2 dx1 3 dy1 4 psfmag1 5 dpsfmag1 6 x2 7 y2 8 flux 9 galaxy 10 mag2 11 dmag2 12 sxx 13 syy 14 sxy 15 s7 16
