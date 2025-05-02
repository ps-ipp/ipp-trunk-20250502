
macro find.fwhm.pgauss
  if ($0 != 1)
    echo "USAGE: find.fwhm"
    break
  end
  
  $dz = 0.01
  create z 0 50 $dz

  # Gaussian taylor expansion
  # f = 1 / (1 + z + z^2/2 + z^3/6)
  # 1 + z + z^2/2 + z^3/6 = 2
  # f = z + z^2/2 + z^3/6 - 2, find where f == 0.0
  
  set f = z + 0.5*z^2 + (1/6.0)*z^3 - 2.0
  set dfdz = 1 + z + 0.5*z^2

  lim -n 0 z f; clear; box; plot z f
  lim -n 1 z dfdz; clear; box; plot z dfdz  

  $nZ0 = 0
  $nZ1 = 5 / $dz

  $Zg = 0.5*(z[$nZ0] + z[$nZ1])  
  $nZg = int($Zg / $dz)
  $dZ = 1.0

  for i 0 10
    $Fg = f[$nZg]
    $dFdz_g = dfdz[$nZg]

    $dZ = $Fg / $dFdz_g

    echo $Zg $Fg $dZ $dFdz_g

    $Zg -= $dZ
    $Zg = max ($Zg , 0)
    $nZg = int($Zg / $dz)
  end 
  echo $Zg $Fg $dZ $dFdz_g
  $Zhm = $Zg
  $FWHM = 2*sqrt(2*$Zg)

  echo $Zhm : $FWHM
end

macro find.fwhm.rgauss
  if ($0 != 2)
    echo "USAGE: find.fwhm (K)"
    break
  end
  
  $K = $1

  if ($K == 0.0)
    $Zhm = (sqrt(5) - 1.0) / 2.0
    $FWHM = 2*sqrt(2*$Zhm)
    echo $K : $Zhm : $FWHM
    return
  end

  $dz = 0.01
  create z 0 50 $dz

  # set f = 1.0 / (1.0 + $K*z + z^1.667)
  # f = $K*z + z^1.667 - 1.0, find where f == 0.0
  
  set f = z + z^$K - 1.0
  set dfdz = ln(z) * z^$K + 1.0

  lim -n 0 z f; clear; box; plot z f
  lim -n 1 z dfdz; clear; box; plot z dfdz  

  $nZ0 = 0
  $nZ1 = 5 / $dz

  $Zg = 0.5*(z[$nZ0] + z[$nZ1])  
  $nZg = int($Zg / $dz)
  $dZ = 1.0

  for i 0 10
    $Fg = f[$nZg]
    $dFdz_g = dfdz[$nZg]

    $dZ = $Fg / $dFdz_g

    echo $nZg $Zg $Fg $dZ $dFdz_g

    $Zg -= $dZ
    $Zg = max ($Zg , 0)
    $nZg = int($Zg / $dz)
  end 
  # echo $Zg $Fg $dZ $dFdz_g
  $Zhm = $Zg
  $FWHM = 2*sqrt(2*$Zg)

  echo $K : $Zhm : $FWHM
end

macro find.fwhm.ps1v1
  if ($0 != 2)
    echo "USAGE: find.fwhm (K)"
    break
  end
  
  $K = $1

  if ($K == 0.0)
    $Zhm = 1.0
    $FWHM = 2*sqrt(2)
    fprintf "%4.2f, // %4.1f, %4.2f" $FWHM $K $Zhm 
    return
  end

  $dz = 0.01
  create z 0 50 $dz

  # set f = 1.0 / (1.0 + $K*z + z^1.667)
  # f = $K*z + z^1.667 - 1.0, find where f == 0.0
  
  set f = $K*z + z^1.667 - 1.0
  set dfdz = $K + 1.677*z^0.667

  #lim -n 0 z f; clear; box; plot z f
  #lim -n 1 z dfdz; clear; box; plot z dfdz  

  # these just need to bound the solution
  $nZ0 = 0.5 / $dz
  $nZ1 = 5.0 / $dz

  $Zg = 0.5*(z[$nZ0] + z[$nZ1])  
  $nZg = int($Zg / $dz)
  $dZ = 1.0

  for i 0 10
    # interpolate between $nZg and $nZg + 1
    $Fg = f[$nZg] + ($Zg - $nZg*$dz)*(f[$nZg+1] - f[$nZg])/$dz
    $dFdz_g = dfdz[$nZg] + ($Zg - $nZg*$dz)*(dfdz[$nZg+1] - dfdz[$nZg])/$dz

    # $Fg = f[$nZg]
    # $dFdz_g = dfdz[$nZg]

    $dZ = $Fg / $dFdz_g

    # echo -no-return $Zg $Fg $dZ $dFdz_g

    $Zg -= $dZ

    $Zg = max ($Zg , 0)
    $Zg = min ($Zg , z[-2])
    $nZg = int($Zg / $dz)
    
    # echo ": $Zg $nZg"
  end 
  # echo $Zg $Fg $dZ $dFdz_g
  $Zhm = $Zg
  $FWHM = 2*sqrt(2*$Zg)

  fprintf "%4.2f, // %4.1f, %4.2f" $FWHM $K $Zhm 
end

# hsc
macro find.fwhm.hscv1
  if ($0 != 2)
    echo "USAGE: find.fwhm (K)"
    break
  end
  
  $K = $1

  if ($K == 0.0)
    $Zhm = 1.0
    $FWHM = 2*sqrt(2)
    fprintf "%4.2f, // %4.1f, %4.2f" $FWHM $K $Zhm 
    return
  end

  $dz = 0.01
  create z 0 50 $dz

  # set f = 1.0 / (1.0 + $K*z + z^1.667)
  # f = $K*z + z^1.667 - 1.0, find where f == 0.0
  
  set f = $K*z + z^1.8 - 1.0
  set dfdz = $K + 1.8*z^0.8

  #lim -n 0 z f; clear; box; plot z f
  #lim -n 1 z dfdz; clear; box; plot z dfdz  

  # these just need to bound the solution
  $nZ0 = 0.5 / $dz
  $nZ1 = 5.0 / $dz

  $Zg = 0.5*(z[$nZ0] + z[$nZ1])  
  $nZg = int($Zg / $dz)
  $dZ = 1.0

  for i 0 10
    # interpolate between $nZg and $nZg + 1
    $Fg = f[$nZg] + ($Zg - $nZg*$dz)*(f[$nZg+1] - f[$nZg])/$dz
    $dFdz_g = dfdz[$nZg] + ($Zg - $nZg*$dz)*(dfdz[$nZg+1] - dfdz[$nZg])/$dz

    # $Fg = f[$nZg]
    # $dFdz_g = dfdz[$nZg]

    $dZ = $Fg / $dFdz_g

    # echo -no-return $Zg $Fg $dZ $dFdz_g

    $Zg -= $dZ

    $Zg = max ($Zg , 0)
    $Zg = min ($Zg , z[-2])
    $nZg = int($Zg / $dz)
    
    # echo ": $Zg $nZg"
  end 
  # echo $Zg $Fg $dZ $dFdz_g
  $Zhm = $Zg
  $FWHM = 2*sqrt(2*$Zg)

  fprintf "%4.2f, // %4.1f, %4.2f" $FWHM $K $Zhm 
end

# qgauss is like ps1_v1 with z^2.25
macro find.fwhm.qgauss
  if ($0 != 2)
    echo "USAGE: find.qgauss (K)"
    break
  end
  
  $K = $1

  if ($K == 0.0)
    $Zhm = 1.0
    $FWHM = 2*sqrt(2)
    fprintf "%4.2f, // %4.1f, %4.2f" $FWHM $K $Zhm 
    return
  end

  $dz = 0.01
  create z 0 50 $dz

  # set f = 1.0 / (1.0 + $K*z + z^2.25)
  # f = $K*z + z^2.25 - 1.0, find where f == 0.0
  
  set f = $K*z + z^2.25 - 1.0
  set dfdz = $K + 2.25*z^1.25

  #lim -n 0 z f; clear; box; plot z f
  #lim -n 1 z dfdz; clear; box; plot z dfdz  

  $nZ0 = 0
  $nZ1 = 5 / $dz

  $Zg = 0.5*(z[$nZ0] + z[$nZ1])  
  $nZg = int($Zg / $dz)
  $dZ = 1.0

  for i 0 10
    # interpolate between $nZg and $nZg + 1
    $Fg = f[$nZg] + ($Zg - $nZg*$dz)*(f[$nZg+1] - f[$nZg])/$dz
    $dFdz_g = dfdz[$nZg] + ($Zg - $nZg*$dz)*(dfdz[$nZg+1] - dfdz[$nZg])/$dz

    # $Fg = f[$nZg]
    # $dFdz_g = dfdz[$nZg]

    $dZ = $Fg / $dFdz_g

    # echo $Zg $Fg $dZ $dFdz_g

    $Zg -= $dZ
    $Zg = max ($Zg , 0)
    $Zg = min ($Zg , z[-2])
    $nZg = int($Zg / $dz)
  end 
  # echo $Zg $Fg $dZ $dFdz_g
  $Zhm = $Zg
  $FWHM = 2*sqrt(2*$Zg)

  fprintf "%4.2f, // %4.1f, %4.2f" $FWHM $K $Zhm 
end

macro fwhm.trend
  if ($0 != 4)
    echo "USAGE: fwhm.trend (model) (struct) (output)"
    echo "  model: pgauss, rgauss, ps1v1, qgauss, hscv1"
    break
  end

  delete fwhm_v k_v

  $SAVE = 1
  if ($SAVE) exec rm -f $3
  if ($SAVE) output $3

  $minK = -1
  $maxK = 20
  $delK = 0.2

  if ($SAVE) echo "# define FWHM_BIN $delK"
  if ($SAVE) echo "# define MIN_FWHM_BIN $minK"
  if ($SAVE) echo "# define N_FWHM_BIN {int(($maxK - $minK) / $delK) + 1}"
#  if ($SAVE) echo "static float $2[] = \{"

  for k $minK $maxK $delK -incl
    find.fwhm.$1 $k
    concat $k k_v
    concat $FWHM fwhm_v
  end
#  if ($SAVE) echo "\}@"
  if ($SAVE) output stdout

#  lim k_v fwhm_v; clear; box; plot k_v fwhm_v -pt 10 -sz 1.0
end

