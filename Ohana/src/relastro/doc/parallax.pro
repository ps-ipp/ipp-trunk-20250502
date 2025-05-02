
macro sample
  PMsim 15 30 20 10 $ANGLE
  set dX = zero(X) + 10
  set dY = zero(Y) + 10

  style -c black
  plot -x 2 -pt 2 -sz 2.0 X Y -dx dX -dy dY

  # set x = (1000.0/$dS)*cos(t*$TP) + $vX*t
  # set y = (1000.0/$dS)*sin(t*$TP)*dsin($ANGLE) + $vY*t
  # plot -c grey60 -x 2 -sz 2.0 -pt 7 x y

  create t 0 1.5 0.01
  set x = (1000.0/$dS)*cos(t*$TP) + $vX*t
  set y = (1000.0/$dS)*sin(t*$TP)*dsin($ANGLE) + $vY*t
  plot -c grey60 -x 0 x y

  # label -x "offset east (milliarcsec)"
  # label -y "offset north (milliarcsec)"
end

macro PMsim
  if ($0 != 6)
    echo "USAGE: PMsim Do vX vY dP dT"
    echo "Do : distance in parsec"
    echo "vX, vY : velocity (milliarcsec/year)"
    echo "dP : astrometric scatter (arcsec)"
    echo "dT : ecliptic lattitude (degrees)"
    break
  end

  $TP = 2*3.1416

  $dS = $1
  $vX = $2
  $vY = $3
  $dP = $4
  $dT = $5

  mkbase

  set x = (1000.0/$dS)*cos(t*$TP) + $vX*t
  set y = (1000.0/$dS)*sin(t*$TP)*dsin($dT) + $vY*t

  gaussdev dx x[] 0.0 $dP
  gaussdev dy y[] 0.0 $dP 

  set X = x + dx
  set Y = y + dy

  if ($PLOT)
    lim -200 200 -200 200; clear; box;
    style -c black
    plot -x 2 -pt 2 -sz 2.0 X Y
    plot -x 0 X Y
  end

  PMfit X Y t $dT

  if ($PLOT) 
    label -x "offset east (milliarcsec)"
    label -y "offset north (milliarcsec)"
    section a 0 0 1 1
    lim 0 1 0 1
    sprintf line "input: D: %5.1f (pc), V: %5.1f, %5.1f (mas/yr), dS: %4.1f" $1 $2 $3 $4
    textline -fn courier 14 0.15 0.95 "$line"
    sprintf line "fit:    D: %5.1f (pc), V: %5.1f, %5.1f (mas/yr)" $dS $vX $vY
    textline -fn courier 14 0.15 0.90 "$line"
    section default
  end
end

macro PMfit
  if ($0 != 5)
    echo "USAGE: PMfit X Y t (theta)"
    echo "X,Y are offsets in milliarcsec"
    echo "t is time in years"
    break
  end

  $TP = 2*3.1416
  set x = $1
  set y = $2
  set t = $3
  set cs = cos(t*$TP)
  set snx = sin(t*$TP)*dsin($4)

  mcreate A 3 3
  create B 0 3

  # define A values:
  set tmp = cs^2 + snx^2
  vstat -q tmp
  zap A 0 0 1 1 -v $TOTAL

  set tmp = t*cs
  vstat -q tmp
  zap A 1 0 1 1 -v $TOTAL
  zap A 0 1 1 1 -v $TOTAL

  set tmp = t*snx
  vstat -q tmp
  zap A 2 0 1 1 -v $TOTAL
  zap A 0 2 1 1 -v $TOTAL

  set tmp = t^2
  vstat -q tmp
  zap A 1 1 1 1 -v $TOTAL
  zap A 2 2 1 1 -v $TOTAL

  zap A 2 1 1 1 -v 0
  zap A 1 2 1 1 -v 0

  # define B values:
  set tmp = x*cs + y*snx
  vstat -q tmp
  B[0] = $TOTAL

  set tmp = x*t
  vstat -q tmp
  B[1] = $TOTAL

  set tmp = y*t
  vstat -q tmp
  B[2] = $TOTAL

  gaussj A B

  $dS = 1000/B[0]
  $vX = B[1]
  $vY = B[2]

  if ($PLOT)
    echo "Do: $dS"
    echo "vX: $vX"
    echo "vY: $vY"

    set x = (1000.0/$dS)*cos(t*$TP) + $vX*t
    set y = (1000.0/$dS)*sin(t*$TP)*dsin($4) + $vY*t
    plot -c red -x 2 -sz 2.0 -pt 7 x y
  end
end

macro PMstats
  
  if ($0 != 7)
    echo "USAGE: PMsim Do vX vY dP dT (Niter)"
    echo "Do : distance in parsec"
    echo "vX, vY : velocity (milliarcsec/year)"
    echo "dP : astrometric scatter (arcsec)"
    echo "dT : ecliptic lattitude (degrees)"
    break
  end

  delete -q vx vy ds
  $PLOT = 0

  for i 0 $6
    PMsim $1 $2 $3 $4 $5
    concat $dS ds
    concat $vX vx
    concat $vY vy
  end

  vstat ds
  vstat vx
  vstat vy

  set gm = 1000/ds
  vstat gm

  clear -s -n 1
  label -fn courier 14
  # section a 0.0 0.0 0.5 1.0
  lim {$2-30} {$2+30} {$3-30} {$3+30}; box
  plot -x 2 -pt 2 -sz 0.5 vx vy
  label -x "p.m. (east, mas/yr)"
  label -y "p.m. (east, mas/yr)"

  clear -s -n 2
  # section b 0.5 0.0 0.5 1.0
  histogram gm Ng {1000/$1 - 30.0} {1000/$1 + 30.0} 1.0
  create dg {1000/$1 - 30.0} {1000/$1 + 30.0} 1.0
  lim dg Ng; box
  plot -x 1 dg Ng

  label -x "parallax (mas)"
  label -y "# of tests"

end

macro PMtrend

  PMstats 10 5.0 5.0 10 90 100
  echo 10
  PMstats 10 5.0 5.0 {750/2.38/25} 90 100
  echo {750/2.38/25} 
  PMstats 10 5.0 5.0 {750/2.38/10} 90 100
  echo {750/2.38/10} 
  PMstats 10 5.0 5.0 {750/2.38/5} 90 100
  echo {750/2.38/5} 
end

list dist
  5
  10
  15
  20
  30
  40
  50
  60
  70
  80
end

macro ParDetect

  local i
  delete -q xp yp dyp dym
  
  $ANGLE = 45

  for i 0 $dist:n
    PMstats $dist:$i 5.0 5.0 10 $ANGLE 100
    concat $dist:$i xp
    concat {1000/$MEAN} yp
    concat {1000/($MEAN - $SIGMA) - 1000/$MEAN} dyp
    concat {1000/$MEAN - 1000/($MEAN + $SIGMA)} dym
  end

  section a 0.0 0.0 0.5 1.0

  lim xp -1.0 101; clear; box; 
  plot -c black -x 2 xp yp -dy dym +dy dyp
  
  delete -q xp yp 
  concat 0 xp
  concat 100 xp
  set yp = xp
  plot -x 0 -c blue xp yp

  set yp = zero(xp) + 62.5
  plot -x 0 -c red -lt 1 xp yp
  style -lt 0  
  label -x "input dist (pc)" -y "output dist" -fn courier 14

  $PLOT = 0
  section b 0.5 0.0 0.5 1.0
  lim -120 120 -120 120; box -labels 1001;

  for i 0 5
   sample
  end

  label -x "offset east (milliarcsec)"
  label +y "offset north"
  ps -name ParallaxDist.ps
end

macro fields

  $Dx = 0.25*4800*8/3600
  $Fo = 1.09

  $dD = 2.65/1.09

  $Nf = 0
  for i {-30+$dD/2} 90 $dD
    $Np = int(dcos($i)*1.09*360/2.65) + 1
    $Nf = $Nf + $Np
    fprintf "%5.1f %5.1f %4d %4d" $i {dcos($i)*1.09*360/2.65} $Np $Nf 
  end
end

macro SNsim
  $w  = 0.76
  $m1 = 24.8
  $mu = 20.4

  lim 13.5 26.5 0.0 4.0; clear; box

  $t = 1
  SN

  $t = 5
  SN

  $t = 30
  SN

  $t = 100
  SN

  set sn = 5 + zero (m)
  set lsn = log(sn)
  plot -lt 2 m lsn

  set sn = 25 + zero (m)
  set lsn = log(sn)
  plot -lt 2 m lsn

  set sn = 100 + zero (m)
  set lsn = log(sn)
  plot -lt 2 m lsn
  style -lt 0
end

macro SN
  create m 14 26 0.1
  set sn = ten (-0.2*(2*m - $mu - $m1)) * sqrt($t/$w^2/3.1416)
  set lsn = log(sn)
  plot m lsn
end

# r: 30s @ 5sig = 22.65  (0.6")
# i: 30s @ 5sig = 22.42  (0.6")
# i: 30s @ 5sig = 22.27  (for 0.3 mag lower zp)

macro mkbase
  delete -q t
  concat 0.0 t
  concat {10.0/365.0} t
  concat {20.0/365.0} t
  concat { 6.0/52.0} t

  concat { 3.0/12.0} t
  concat { 6.0/12.0} t
  concat {12.0/12.0} t
  concat {18.0/12.0} t
end


macro parfactor

  $RA = $1
  $Dec = $2
  $e = 23 + 27/60
  
  create s 0 360
  set pR = dcos($e)*d

  set pR =  +(dcos($e)*dsin(s)*dcos($RA) - dcos(s)*dsin($RA))
  set pD =  -(dcos($e)*dsin(s)*dsin($RA) + dcos(s)*dcos($RA))*dsin($Dec) + dsin($e)*dsin(s)*dcos($Dec)

  lim -1.1 1.1 -1.1 1.1; clear; box; plot pR pD
end
