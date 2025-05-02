
# create and fit a quadratic:
macro fiterrors

  if ($0 != 2)
    echo "USAGE: fiterrors (sigma)"
    break
  end

  create x 0 30 
  set y = 1000 - 5*(x-15)^2

  gaussdev sigma x[] 0.0 1.0
  set Y = $1*(y + sigma)
  set dY = zero(x) + $1

  lim x Y; clear; box; plot x Y -dy dY

  fit x Y 2 -dy dY
  applyfit x Yf
  plot -x 0 x Yf -c red
  
  $xo = -0.5*$C1 / $C2
  $dxo = $xo * sqrt (($dC1/$C1)^2 + ($dC2/$C2)^2)
  echo "$xo +/- $dxo"

end
