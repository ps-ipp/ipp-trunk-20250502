
$TIMEFORMAT = mjd
$TIMEREF = 2000/01/01,00:00:00
ctimes -abs 2010/01/01,00:00:00 -var MJD_MIN
ctimes -abs 2015/01/01,00:00:00 -var MJD_MAX
ctimes -abs 2012/01/01,00:00:00 -var MJD_REF

$Tref = 12.4148

macro go
  echo "--- pos : noutliers"
  testsuite pos
  testsuite pos-irls
  testsuite pos-boot

  echo "--- pos : noutliers"
  testsuite pm
  testsuite pm-irls
  testsuite pm-boot

  echo "--- pos : noutliers"
  testsuite plx
  testsuite plx-irls
  testsuite plx-boot

  echo "--- nboot"
  testnboot pos-boot
  testnboot pm-boot
  testnboot plx-boot
end

macro teststk
  if ($0 != 3)
    echo "USAGE: testobj2 (Nstars) (foutliers)"
    break
  end

  exec ../bin/fitstk.lin64 plx -Nstars $1 -foutliers $2 > test.stk.dat
  load.stack test.stk.dat
end

macro testobj3
  if ($0 != 3)
    echo "USAGE: testobj3 (Nstars) (foutliers)"
    break
  end

  $Nmin:pos = 99
  $Nmin:pm  = 99
  $Nmin:plx = 99

  foreach mode pos pm plx
    exec ../bin/fitobj3.lin64 $mode -Nstars $1 -foutliers $2 -Nminpoints $Nmin:$mode > test.$mode.dat
    checkstats test.$mode.dat $mode
  end
end

macro testobj2
  if ($0 != 3)
    echo "USAGE: testobj2 (Nstars) (foutliers)"
    break
  end

  $Nmin:pos = 1
  $Nmin:pm  = 5
  $Nmin:plx = 7

  foreach mode pos pm plx
    exec ../bin/fitobj2.lin64 $mode -Nstars $1 -foutliers $2 -Nminpoints $Nmin:$mode > test.$mode.dat
    checkstats test.$mode.dat $mode
  end
end

macro testobj
  if ($0 != 1)
    echo "USAGE: testobj"
    break
  end

  foreach mode pos pm plx
    exec ../bin/fitobj.lin64 $mode > test.$mode.dat
    checkstats.obj test.$mode.dat $mode
    fprintf "%5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %6.2f %5.2f" $value_dRf $value_dDf $value_duR $value_duD $value_dP $value_dRo $value_dDo $value_duRo $value_duDo $value_dPo $sigma_dRo $sigma_dDo $sigma_duRo $sigma_duDo $sigma_dPo $mean_Nfit $mean_chisq
  end
end

macro testnboot
  if ($0 != 2)
    echo "USAGE: testnboot (mode)"
    break
  end

  foreach nboot 20 30 100 300 1000
    exec ../bin/fitpm.lin64 -Nstars 3000 -Noutliers 20 -Npoints 100 -Nbootstrap $nboot $1 > test.$1.dat
    reload test.$1.dat 0

    fprintf "%4d : %5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %6.2f %5.2f" $nboot $value_dRf $value_dDf $value_duR $value_duD $value_dP $value_dRo $value_dDo $value_duRo $value_duDo $value_dPo $sigma_dRo $sigma_dDo $sigma_duRo $sigma_duDo $sigma_dPo $mean_Nfit $mean_chisq
  end
end

macro testsuite
  if ($0 != 2)
    echo "USAGE: testsuite (mode)"
    break
  end

  # foreach outliers 0 3 10 30 100 

  foreach outliers 0
    echo ../bin/fitpm.lin64 -Nstars 3000 -Npoints 100 -Nbootstrap 100 $1 
    exec ../bin/fitpm.lin64 -Nstars 3000 -Npoints 100 -Nbootstrap 100 $1 > test.$1.dat
    reload test.$1.dat 0

    fprintf "%4d : %5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %6.2f %5.2f" $outliers $value_dRf $value_dDf $value_duR $value_duD $value_dP $value_dRo $value_dDo $value_duRo $value_duDo $value_dPo $sigma_dRo $sigma_dDo $sigma_duRo $sigma_duDo $sigma_dPo $mean_Nfit $mean_chisq
  end
end

macro reload
  if ($0 != 3)
    echo "USAGE: reload (file) (isObjPLX)"
    break
  end

  data $1
  if ($2)
    read Npts 1 Nout 2 Ro 3 Do 4 uRo 5 uDo 6 Po 7 Rx 9 Dx 10 uRx 11 uDx 12 Px 13 Tx 15 dRo 17 dDo 18 duRo 19 duDo 20 dPo 21 Nfit 22 chisq 26
    read Ro 1 Do 2 uRo 3 uDo 4 Po 5 Rx 7 Dx 8 uRx 9 uDx 10 Px 11 Tx 13 dRo 15 dDo 16 duRo 17 duDo 18 dPo 19 Nfit 20 chisq 24
  else
    read Ro 1 Do 2 uRo 3 uDo 4 Po 5 Rx 7 Dx 8 uRx 9 uDx 10 Px 11 Tx 13 dRo 15 dDo 16 duRo 17 duDo 18 dPo 19 Nfit 20 chisq 22
  end

  foreach f R D uD uR P
    set d$f = $f\o - $f\x
  end

  set Rf = Rx - uRx*(Tx - $Tref)/3600.0/dcos(Dx)
  set Df = Dx - uDx*(Tx - $Tref)/3600.0

  set dRf = 3600*(Ro - Rf)*dcos(Dx)
  set dDf = 3600*(Do - Df) 

  # echo "fit scatter"
  vstat -q dRf;  $value_dRf =  $SIGMA*1000
  vstat -q dDf;  $value_dDf =  $SIGMA*1000
  vstat -q duR;  $value_duR =  $SIGMA*1000
  vstat -q duD;  $value_duD =  $SIGMA*1000
  vstat -q dP ;  $value_dP  =  $SIGMA*1000

  # echo "fit errors"
  vstat -q dRo;  $value_dRo  = $MEAN*1000; $sigma_dRo  = $SIGMA*1000
  vstat -q dDo;  $value_dDo  = $MEAN*1000; $sigma_dDo  = $SIGMA*1000
  vstat -q duRo; $value_duRo = $MEAN*1000; $sigma_duRo = $SIGMA*1000
  vstat -q duDo; $value_duDo = $MEAN*1000; $sigma_duDo = $SIGMA*1000
  vstat -q dPo ; $value_dPo  = $MEAN*1000; $sigma_dPo  = $SIGMA*1000

  # echo "Nfit"
  vstat -q Nfit; $mean_Nfit = $MEAN

  # echo chisq
  vstat -q chisq; $mean_chisq = $MEAN
end

macro checkstats.obj
  if ($0 != 3)
    echo "USAGE: checkstats.obj (file) (mode)"
    break
  end

  data $1
  if ("$2" == "pos")
    read Ro 1 Do 2 uRo 3 uDo 4 Po 5 Rx 7 Dx 8 uRx 9 uDx 10 Px 11 Tx 13 dRo 15 dDo 16 duRo 17 duDo 18 dPo 19 Nfit 20 chisq 22
  end
  if ("$2" == "pm")
    read Ro 1 Do 2 uRo 3 uDo 4 Po 5 Rx 7 Dx 8 uRx 9 uDx 10 Px 11 Tx 13 dRo 15 dDo 16 duRo 17 duDo 18 dPo 19 Nfit 20 chisq 23
  end
  if ("$2" == "plx")
    read Ro 1 Do 2 uRo 3 uDo 4 Po 5 Rx 7 Dx 8 uRx 9 uDx 10 Px 11 Tx 13 dRo 15 dDo 16 duRo 17 duDo 18 dPo 19 Nfit 20 chisq 24
  end

  foreach f R D uD uR P
    set d$f = $f\o - $f\x
  end

  set Rf = Rx - uRx*(Tx - $Tref)/3600.0/dcos(Dx)
  set Df = Dx - uDx*(Tx - $Tref)/3600.0

  set dRf = 3600*(Ro - Rf)*dcos(Dx)
  set dDf = 3600*(Do - Df) 

  # echo "fit scatter"
  vstat -q dRf;  $value_dRf =  $SIGMA*1000
  vstat -q dDf;  $value_dDf =  $SIGMA*1000
  vstat -q duR;  $value_duR =  $SIGMA*1000
  vstat -q duD;  $value_duD =  $SIGMA*1000
  vstat -q dP ;  $value_dP  =  $SIGMA*1000

  # echo "fit errors"
  vstat -q dRo;  $value_dRo  = $MEAN*1000; $sigma_dRo  = $SIGMA*1000
  vstat -q dDo;  $value_dDo  = $MEAN*1000; $sigma_dDo  = $SIGMA*1000
  vstat -q duRo; $value_duRo = $MEAN*1000; $sigma_duRo = $SIGMA*1000
  vstat -q duDo; $value_duDo = $MEAN*1000; $sigma_duDo = $SIGMA*1000
  vstat -q dPo ; $value_dPo  = $MEAN*1000; $sigma_dPo  = $SIGMA*1000

  # echo "Nfit"
  vstat -q Nfit; $mean_Nfit = $MEAN

  # echo chisq
  vstat -q chisq; $mean_chisq = $MEAN
end

macro teststats
  if ($0 != 3)
    echo "USAGE: teststats (file) (mode)"
    break
  end

  data $1

  if ("$2" == "pos")
    read Npts 1 Nout 2 Ro 3 Do 4 uRo 5 uDo 6 Po 7 Rx 9 Dx 10 uRx 11 uDx 12 Px 13 Tx 15 dRo 17 dDo 18 duRo 19 duDo 20 dPo 21 Nfit 22 chisq 24
  end
  if ("$2" == "pm")
    read Npts 1 Nout 2 Ro 3 Do 4 uRo 5 uDo 6 Po 7 Rx 9 Dx 10 uRx 11 uDx 12 Px 13 Tx 15 dRo 17 dDo 18 duRo 19 duDo 20 dPo 21 Nfit 22 chisq 25
  end
  if ("$2" == "plx")
    read Npts 1 Nout 2 Ro 3 Do 4 uRo 5 uDo 6 Po 7 Rx 9 Dx 10 uRx 11 uDx 12 Px 13 Tx 15 dRo 17 dDo 18 duRo 19 duDo 20 dPo 21 Nfit 22 chisq 26
  end

  foreach f R D uD uR P
    set d$f = $f\o - $f\x
  end

  set Rf = Rx - uRx*(Tx - $Tref)/3600.0/dcos(Dx)
  set Df = Dx - uDx*(Tx - $Tref)/3600.0

  set dRf = 3600*(Ro - Rf)*dcos(Dx)
  set dDf = 3600*(Do - Df) 

  # echo "fit scatter"
  vstat -q dRf;  $value_dRf =  $SIGMA*1000
  vstat -q dDf;  $value_dDf =  $SIGMA*1000
  vstat -q duR;  $value_duR =  $SIGMA*1000
  vstat -q duD;  $value_duD =  $SIGMA*1000
  vstat -q dP ;  $value_dP  =  $SIGMA*1000

  # echo "fit errors"
  vstat -q dRo;  $value_dRo  = $MEAN*1000; $sigma_dRo  = $SIGMA*1000
  vstat -q dDo;  $value_dDo  = $MEAN*1000; $sigma_dDo  = $SIGMA*1000
  vstat -q duRo; $value_duRo = $MEAN*1000; $sigma_duRo = $SIGMA*1000
  vstat -q duDo; $value_duDo = $MEAN*1000; $sigma_duDo = $SIGMA*1000
  vstat -q dPo ; $value_dPo  = $MEAN*1000; $sigma_dPo  = $SIGMA*1000

  # echo "Nfit"
  vstat -q Nfit; $mean_Nfit = $MEAN

  # echo chisq
  vstat -q chisq; $mean_chisq = $MEAN

  fprintf "%5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %6.2f %5.2f" $value_dRf $value_dDf $value_duR $value_duD $value_dP $value_dRo $value_dDo $value_duRo $value_duDo $value_dPo $sigma_dRo $sigma_dDo $sigma_duRo $sigma_duDo $sigma_dPo $mean_Nfit $mean_chisq

end

macro checkstats
  if ($0 != 3) 
    echo "USAGE: checkstats (file) (mode)"
    break
  end

  teststats $1 $2
  
  delete -q dRbin oRbin
  for i 0 100
    subset tmp = dRf if (Npts == $i)
    vstat tmp -q
    concat $SIGMA dRbin
    concat $MEAN oRbin
  end

  create n 0 dRbin[]
  set dRmod = 0.010 / sqrt (n)
  lim n -0.001 0.015; clear; box; plot n dRbin
  plot -c red -pt 7 n dRmod
end

macro load.stack
  if ($0 != 2) 
    echo "USAGE: load.stack (file)"
    break
  end

  data $1
  read Npts 1 Nout 2 Nstk 3 Ro 4 Do 5 uRo 6 uDo 7 Po 8 Rx 10 Dx 11 uRx 12 uDx 3 Px 14 Tx 16 dRo 18 dDo 19 duRo 20 duDo 21 dPo 22 Nfit 23 Rstk 25 Dstk 26 dRstk 27 dDstk 28 chisq 32
  # read Ro 1 Do 2 uRo 3 uDo 4 Po 5 Rx 7 Dx 8 uRx 9 uDx 10 Px 11 Tx 13 dRo 15 dDo 16 duRo 17 duDo 18 dPo 19 Nfit 20 chisq 24
  # read Ro 1 Do 2 uRo 3 uDo 4 Po 5 Rx 7 Dx 8 uRx 9 uDx 10 Px 11 Tx 13 dRo 15 dDo 16 duRo 17 duDo 18 dPo 19 Nfit 20 chisq 22

  foreach f uD uR P
    set d$f = $f\o - $f\x
  end
  set sdR = 3600*(Ro - Rstk)*dcos(Do)
  set sdD = 3600*(Do - Dstk)

  set Rf = Rx - uRx*(Tx - $Tref)/3600.0/dcos(Dx)
  set Df = Dx - uDx*(Tx - $Tref)/3600.0

  set dRf = 3600*(Ro - Rf)*dcos(Dx)
  set dDf = 3600*(Do - Df) 

  # echo "fit scatter"
  vstat -q dRf;  $value_dRf =  $SIGMA*1000
  vstat -q dDf;  $value_dDf =  $SIGMA*1000
  vstat -q duR;  $value_duR =  $SIGMA*1000
  vstat -q duD;  $value_duD =  $SIGMA*1000
  vstat -q dP ;  $value_dP  =  $SIGMA*1000

  vstat -q sdR;  $value_sdR =  $SIGMA*1000
  vstat -q sdD;  $value_sdD =  $SIGMA*1000

  # echo "fit errors"
  vstat -q dRo;  $value_dRo  = $MEAN*1000; $sigma_dRo  = $SIGMA*1000
  vstat -q dDo;  $value_dDo  = $MEAN*1000; $sigma_dDo  = $SIGMA*1000
  vstat -q duRo; $value_duRo = $MEAN*1000; $sigma_duRo = $SIGMA*1000
  vstat -q duDo; $value_duDo = $MEAN*1000; $sigma_duDo = $SIGMA*1000
  vstat -q dPo ; $value_dPo  = $MEAN*1000; $sigma_dPo  = $SIGMA*1000

  # echo "Nfit"
  vstat -q Nfit; $mean_Nfit = $MEAN

  # echo chisq
  vstat -q chisq; $mean_chisq = $MEAN

  fprintf "%5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %5.2f %5.2f %5.2f %5.2f %5.2f : %6.2f %5.2f" $value_dRf $value_dDf $value_duR $value_duD $value_dP $value_dRo $value_dDo $value_duRo $value_duDo $value_dPo $sigma_dRo $sigma_dDo $sigma_duRo $sigma_duDo $sigma_dPo $mean_Nfit $mean_chisq
  
  delete -q dRbin oRbin
  for i 0 100
    subset tmp = dRf if (Npts == $i)
    vstat tmp -q
    concat $SIGMA dRbin
    concat $MEAN oRbin
  end

  create n 0 dRbin[]
  set dRmod = 0.010 / sqrt (n)
  lim n -0.001 0.015; clear; box; plot n dRbin
  plot -c red -pt 7 n dRmod
end

# determine the parallax factor (time) (ra) (dec) (parfR) (parfD) are vectors, 
macro getParFactor_vectors
  if ($0 != 6)
    echo "USAGE: getParFactor (time) (ra) (dec) (parfR) (parfD) -- time in MJD"
    echo "RETURN: parfR & parfD (vectors of length ra[],dec[])"
    break
  end

  set dJ2000 = $1 - $J2000
  ## first, determine the sun postion
  set L_v = 280.460 + 0.9856474 * dJ2000
  set g_v = 357.528 + 0.9856003 * dJ2000
  set epsilon_v = 23.439 - 0.0000004 * dJ2000 ; # obliquity of ecliptic in degrees
  
  set lambda_v = L_v + 1.915 * dsin(g_v) + 0.020 * dsin(2. * g_v); # longitude in degrees
  set sol_dist_v = 1.00014 - 0.01671*dcos(g_v) - 0.00014*dcos(2*g_v); # earth-to-sun dist in AU

  set tmp_x = sol_dist_v*dcos(lambda_v)
  set tmp_y = sol_dist_v*dcos(epsilon_v)*dsin(lambda_v)
  set tmp_z = sol_dist_v*dsin(epsilon_v)*dsin(lambda_v)

  set tmp_ra  = $2
  set tmp_dec = $3

  set $4 = +(tmp_y*dcos(tmp_ra) - tmp_x*dsin(tmp_ra))
  set $5 = -(tmp_y*dsin(tmp_ra) + tmp_x*dcos(tmp_ra))*dsin(tmp_dec) + tmp_z*dcos(tmp_dec)
end

