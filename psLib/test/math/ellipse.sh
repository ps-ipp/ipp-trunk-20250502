
# I want to generate a set of ellipses with Sx,Sy,Sxy values dispersed by their dSx,dSy,dSxy errors
# then convert to shape values (major,minor,theta)

macro test_shape_errors_vs_sn_ratios

  foreach field Major Minor f1 f2 f3
    set $field\_SN_r = $field\_0_SN / $field\_1_SN 
  end
end

macro test_shape_errors_vs_sn

  delete -q Major_0_SN Major_1_SN Minor_0_SN Minor_1_SN f1_0_SN f2_0_SN f3_0_SN f1_1_SN f2_1_SN f3_1_SN lSN_v

  for lsn 1 4 0.05
    $sn = ten($lsn)
    test_shape_errors 20.0 10.0 30.0 {1.0/$sn} {1.0/$sn} {1.0/$sn} 
    concat $f1_err_0 f1_0_SN
    concat $f2_err_0 f2_0_SN
    concat $f3_err_0 f3_0_SN
    concat $f1_err_1 f1_1_SN
    concat $f2_err_1 f2_1_SN
    concat $f3_err_1 f3_1_SN

    concat $dMajor_0 Major_0_SN
    concat $dMinor_0 Minor_0_SN

    concat $dMajor_1 Major_1_SN
    concat $dMinor_1 Minor_1_SN

    concat $lsn lSN_v
  end
end

macro test_shape_errors
  if ($0 != 7)
    echo "USAGE: test_shape_errors (major) (minor) (theta) (fSxxErr) (fSyyErr) (fSxyErr)"
    break
  end

  $major_0 = $1
  $minor_0 = $2
  $theta_0 = $3

  # AxesToShape major minor theta sxx sxy syy
  AxesToShape $major_0 $minor_0 $theta_0 sxx_0 sxy_0 syy_0
  ShapeToAxesWithErrors $sxx_0 $sxy_0 $syy_0 {$4*$sxx_0} {$5*$sxy_0} {$6*$syy_0} Major_0 Minor_0 Theta_0 dMajor_0 dMinor_0 dTheta_0 
  echo ShapeToAxesWithErrors $sxx_0 $sxy_0 $syy_0 {$4*$sxx_0} {$5*$sxy_0} {$6*$syy_0} Major_0 Minor_0 Theta_0 dMajor_0 dMinor_0 dTheta_0 

  delete Major_v Minor_v Theta_v f1_v f2_v f3_v

  gaussdev sxx 1000 $sxx_0 {$4*$sxx_0}
  gaussdev sxy 1000 $sxy_0 {$5*$sxy_0}
  gaussdev syy 1000 $syy_0 {$6*$syy_0}
  for i 0 sxx[]
    ShapeToAxes sxx[$i] sxy[$i] syy[$i] Major Minor Theta
    concat $f1 f1_v
    concat $f2 f2_v
    concat $f3 f3_v
    concat $Major Major_v
    concat $Minor Minor_v
    concat $Theta Theta_v
  end    

  vstat -q Major_v
  $dMajor_1 = $SIGMA
  
  vstat -q Minor_v
  $dMinor_1 = $SIGMA

  #echo $dMajor_0 $dMajor_1 {$dMajor_0 / $dMajor_1}
  #echo $dMinor_0 $dMinor_1 {$dMinor_0 / $dMinor_1}

  vstat -q f1_v
  $f1_err_1 = $SIGMA
  $f1_err_0 = sqrt($f1_err_2)
  #echo $f1_err_0 $f1_err_1 {$f1_err_0 / $f1_err_1}

  vstat -q f2_v
  $f2_err_1 = $SIGMA
  $f2_err_0 = sqrt($f2_err_2)
  #echo $f2_err_0 $f2_err_1 {$f2_err_0 / $f2_err_1}

  vstat -q f3_v
  $f3_err_1 = $SIGMA
  $f3_err_0 = sqrt($f3_err_2)
  #echo $f3_err_0 $f3_err_1 {$f3_err_0 / $f3_err_1}
end

macro test_shape_grid

  # create a series of ellipses with theta spinning and AR = 2.0
  $DX = 201

  create theta 0 360 10
  set major = 20.0 + zero(theta)
  set minor = 10.0 + zero(theta)

  # AxesToShape major minor theta sxx sxy syy
  AxesToShape major minor theta sxx sxy syy
  ShapeToAxes sxx sxy syy Major Minor Theta 

  create T 0 360

  lim -40 260 -40 260; clear; box
  for i 0 theta[]
    set xr = major[$i]*dcos(T)
    set yr = minor[$i]*dsin(T)

    set Xr = Major[$i]*dcos(T)
    set Yr = Minor[$i]*dsin(T)

    $ix = $i % 6
    $iy = int($i / 6)

    set X =    dcos(Theta[$i])*xr + dsin(Theta[$i])*yr + $ix*40
    set Y = -1*dsin(Theta[$i])*xr + dcos(Theta[$i])*yr + $iy*40

    set x =    dcos(theta[$i])*xr + dsin(theta[$i])*yr + $ix*40
    set y = -1*dsin(theta[$i])*xr + dcos(theta[$i])*yr + $iy*40

    plot -x 0 x y -c black
    plot -x 0 X Y -c red
  end    
end

macro test_shape_grid_vectors

  # create a series of ellipses with theta spinning and AR = 2.0
  $DX = 201

  # AxesToShape major minor theta sxx sxy syy

  create T 0 360

  $major = 20
  $minor = 10

  lim -40 260 -40 260; clear; box
  for theta 0 360 10

    AxesToShape $major $minor $theta sxx sxy syy
    ShapeToAxes $sxx $sxy $syy Major Minor Theta 

    set xr = $major*dcos(T)
    set yr = $minor*dsin(T)

    set Xr = $Major*dcos(T)
    set Yr = $Minor*dsin(T)

    $ix = int($theta/10) % 6
    $iy = int(int($theta/10) / 6)

    set X =    dcos($Theta)*xr + dsin($Theta)*yr + $ix*40
    set Y = -1*dsin($Theta)*xr + dcos($Theta)*yr + $iy*40

    set x =    dcos($theta)*xr + dsin($theta)*yr + $ix*40
    set y = -1*dsin($theta)*xr + dcos($theta)*yr + $iy*40

    plot -x 0 x y -c black
    plot -x 0 X Y -c red
  end    
end

macro ShapeToAxes
  if ($0 != 7)
    echo "USAGE: ShapeToAxes (sxx) (sxy) (syy) (major) (minor) (theta)"
    echo " theta is returned in degrees"
    break
  end

  #echo $1
  #echo $2
  #echo $3

  # $allVectors = $1?[] && $2?[] && $3?[]
  $allVectors = 0
  if ($allVectors)
    set _sxx = $1
    set _sxy = $2
    set _syy = $3
    
    set f1 = _syy^-2 + _sxx^-2
    set f2 = _syy^-2 - _sxx^-2
    set f3 = sqrt(f2^2 + 4*_sxy^2)
    
    set _minor = sqrt (2.0 / (f1 + f3))
    
    # this returns theta in degrees
    # @ == atan2 -- I should really replace this..
    set _theta = -0.5 * (+2.0*_sxy @ f2)
    set aratio2 = (f1 - f3) / (f1 + f3)
    
    # I can test here if aratio2 is too large/small/nan
    set _major = sqrt (2.0 / (f1 - f3))
    
    set $4 = _major
    set $5 = _minor
    set $6 = _theta
  else
    $_sxx = $1
    $_sxy = $2
    $_syy = $3
    
    $f1 = $_syy^-2 + $_sxx^-2
    $f2 = $_syy^-2 - $_sxx^-2
    $f3 = sqrt($f2^2 + 4*$_sxy^2)
    
    $_minor = sqrt (2.0 / ($f1 + $f3))
    
    # this returns theta in degrees
    # @ == atan2 -- I should really replace this..
    $_theta = -0.5 * (+2.0*$_sxy @ $f2)
    $aratio2 = ($f1 - $f3) / ($f1 + $f3)
    
    # I can test here if aratio2 is too large/small/nan
    $_major = sqrt (2.0 / ($f1 - $f3))
    
    $$4 = $_major
    $$5 = $_minor
    $$6 = $_theta
  end 
end

macro ShapeToAxesWithErrors
  if ($0 != 13)
    echo "USAGE: ShapeToAxes (dSxx) (dSxy) (dSyy) (dSxx) (dSxy) (dSyy) (Major) (Minor) (Theta) (dMajor) (dMinor) (dTheta)"
    echo " theta is returned in degrees"
    break
  end

#   $allVectors = $1?[] && $2?[] && $3?[] && $4?[] && $5?[] && $6?[]
  $allVectors = 0
  if ($allVectors)
    set _sxx = $1
    set _sxy = $2
    set _syy = $3
    
    set _sxx_err = $4
    set _sxy_err = $5
    set _syy_err = $6

    set f1 = _syy^-2 + _sxx^-2
    set f2 = _syy^-2 - _sxx^-2
    set f3 = sqrt(f2^2 + 4*_sxy^2)
    
    set _minor = sqrt (2.0 / (f1 + f3))
    
    # this returns theta in degrees
    # @ == atan2 -- I should really replace this..
    set _theta = -0.5 * (+2.0*_sxy @ f2)
    set aratio2 = (f1 - f3) / (f1 + f3)
    
    # I can test here if aratio2 is too large/small/nan
    set _major = sqrt (2.0 / (f1 - f3))
    
    set $4 = _major
    set $5 = _minor
    set $6 = _theta
  else
    $_sxx = $1
    $_sxy = $2
    $_syy = $3
    
    $_sxx_err = $4
    $_sxy_err = $5
    $_syy_err = $6
    
    $f1 = $_syy^-2 + $_sxx^-2
    $f2 = $_syy^-2 - $_sxx^-2
    $f3 = sqrt($f2^2 + 4*$_sxy^2)
    
    $df1_dsxx_2 = 4*$_sxx^-6
    $df1_dsyy_2 = 4*$_syy^-6
    $df2_dsxx_2 = 4*$_sxx^-6
    $df2_dsyy_2 = 4*$_syy^-6

    if ($_sxy == 0.0) 
     $df3_df2_2  = 1.0
     $df3_dsxy_2 = 0.0
    else
     $df3_df2_2  = $f2^2 / $f3^2
     $df3_dsxy_2 = 16*$_sxy^2 / $f3^2
    end

    # this returns theta in degrees
    # @ == atan2 -- I should really replace this..
    $_theta = -0.5 * (+2.0*$_sxy @ $f2)
    $aratio2 = ($f1 - $f3) / ($f1 + $f3)

    # I can test here if aratio2 is too large/small/nan

    $_minor = sqrt (2.0 / ($f1 + $f3))
    $_major = sqrt (2.0 / ($f1 - $f3))
    
    $dmaj_df1_2 = 0.25 * $_major^2 / ($f1 - $f3)^2
    $dmaj_df3_2 = $dmaj_df1_2

    $dmin_df1_2 = 0.25 * $_minor^2 / ($f1 + $f3)^2
    $dmin_df3_2 = $dmin_df1_2

    $f1_err_2 = $_sxx_err^2*$df1_dsxx_2 + $_syy_err^2*$df1_dsyy_2
    $f2_err_2 = $_sxx_err^2*$df2_dsxx_2 + $_syy_err^2*$df2_dsyy_2
    $f3_err_2 = $f2_err_2*$df3_df2_2 + $_sxy_err^2*$df3_dsxy_2

    $_major_err_2 = $f1_err_2*$dmaj_df1_2 + $f3_err_2*$dmaj_df3_2
    $_minor_err_2 = $f1_err_2*$dmin_df1_2 + $f3_err_2*$dmin_df3_2

    $$7 = $_major
    $$8 = $_minor
    $$9 = $_theta

    $$10 = sqrt($_major_err_2)
    $$11 = sqrt($_minor_err_2)
    $$12 = NAN
  end 
end

macro AxesToShape
  if ($0 != 7)
    echo "USAGE: ShapeToAxes (major) (minor) (theta) (sxx) (sxy) (syy)"
    echo " theta is supplied in degrees"
    break
  end

  $allVectors = $1?[] && $2?[] && $3?[]
  $allVectors = 0

  if ($allVectors) 
    set _major = $1
    set _minor = $2
    set _theta = $3
    
    set f1 = _minor^-2 + _major^-2
    set f2 = _minor^-2 - _major^-2
    
    set sxr = 0.5*f1 - 0.5*f2*dcos(2*_theta);
    set syr = 0.5*f1 + 0.5*f2*dcos(2*_theta);
    
    set $4 = +1.0 / sqrt(sxr);
    set $6 = +1.0 / sqrt(syr);
    set $5 = -0.5*f2*dsin(2*_theta);
  else
    $_major = $1
    $_minor = $2
    $_theta = $3

    $f1 = $_minor^-2 + $_major^-2
    $f2 = $_minor^-2 - $_major^-2

    $sxr = 0.5*$f1 - 0.5*$f2*dcos(2*$_theta);
    $syr = 0.5*$f1 + 0.5*$f2*dcos(2*$_theta);

    $$4 = +1.0 / sqrt($sxr);
    $$6 = +1.0 / sqrt($syr);
    $$5 = -0.5*$f2*dsin(2*$_theta);
  end
end
