
macro ShapeToAxes
  if ($0 != 7)
    echo "USAGE: ShapeToAxes (sxx) (sxy) (syy) (major) (minor) (theta)"
    echo " theta is returned in degrees"
    break
  end

  # I need the concept of a local vector...
  # I should be able to test if these are vectors or scalars
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
end

macro AxesToShapeAlt
  if ($0 != 7)
    echo "USAGE: ShapeToAxes (major) (minor) (theta) (sxx) (sxy) (syy)"
    echo " theta is supplied in degrees"
    break
  end

  set _major = $1
  set _minor = $2
  set _theta = -1*$3

  set tc2 = dcos(_theta)^2
  set ts2 = dsin(_theta)^2

  set f1 = tc2*_major^-2 + ts2*_minor^-2
  set f2 = ts2*_major^-2 + tc2*_minor^-2
  set f3 = _minor^-2 - _major^-2

  set $4 = +1.0 / sqrt(f1);
  set $6 = +1.0 / sqrt(f2);
  set $5 = 0.5*f3*dsin(2*_theta);
end

macro AxesToShape
  if ($0 != 7)
    echo "USAGE: ShapeToAxes (major) (minor) (theta) (sxx) (sxy) (syy)"
    echo " theta is supplied in degrees"
    break
  end

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
end

macro test1

  # create a series of ellipses with theta spinning and AR = 2.0
  $DX = 201

  create theta 0 360 10
  set major = 20.0 + zero(theta)
  set minor = 10.0 + zero(theta)

  # AxesToShape major minor theta sxx sxy syy
  AxesToShape major minor theta sxx sxy syy

  mcreate mosaic {6*($DX + 2)} {6*($DX + 2)} 
  mcreate base $DX $DX
  set x = xramp(base) - int(0.5*$DX)
  set y = yramp(base) - int(0.5*$DX)
  
  for i 0 theta[]
    set r = 0.5*(x/sxx[$i])^2 + 0.5*(y/syy[$i])^2 + sxy[$i]*x*y
    set object = exp(-r)
    $ix = $i % 6
    $iy = int($i / 6)
    extract object mosaic 0 0 $DX $DX {$ix * ($DX + 1)} {$iy * ($DX + 1)}  {6*($DX + 2)} {6*($DX + 2)} 
  end    

  tv mosaic -0.01 0.5
  lim -image
  clear; box

  for i 0 theta[]
    $ix = $i % 6
    $iy = int($i / 6)
    
    $dX = $ix * ($DX + 1) + int(0.5*$DX)
    $dY = $iy * ($DX + 1) + int(0.5*$DX)

    create T 0 360 0.1
    set Xr = major[$i]*dcos(T)
    set Yr = minor[$i]*dsin(T)
    set Xo = Xr*dcos(theta[$i]) - Yr*dsin(theta[$i]) + $dX
    set Yo = Yr*dcos(theta[$i]) + Xr*dsin(theta[$i]) + $dY
    plot Xo Yo -x 0 -c red
  end    
end
