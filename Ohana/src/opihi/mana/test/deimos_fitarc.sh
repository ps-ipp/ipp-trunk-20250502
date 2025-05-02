
macro test_01
 if ($0 != 2)
   echo "USAGE: test_01 (offset)"
   break
 end

  # make a gaussian profile
  create x 0 31
  set yo = 105.1*exp(-0.5*(x - 12.2)^2/2.2^2)
  gaussdev dg yo[] 0 1
  # set dg = zero(yo)
  set y = yo + dg*sqrt(yo)

  # fitarc version
  dimenup y im 1 31
  deimos fitarc im 0 0 1 31 -resid imf -max-iterations 0
  dimendown imf yf1

  echo $C0 $C2 $C1 $C3

  $C0 = $C0 + $1
  vgauss -apply x yf2

  # dy1 : model from fitarc
  # dy2 : rebuilt model
  # NOTE: vgauss and fitarc output results match exactly
  # asymmetry is a result of the fitted values
  
  set dy1 = y - yf1
  set dy2 = y - yf2

  lim -n 1 x dy2; clear; box;
  
  plot x dy1 -c red -x hist
  plot x dy2 -c blue -x hist

  lim -n 2 x y; clear; box;

  plot x y  -c black -x hist -lw 4

  plot x yf1 -c red -x hist -lw 3
  plot x yf2 -c blue -x hist -lw 2
end

macro test_02
 if ($0 != 2)
   echo "USAGE: test_02 (offset)"
   break
 end

  # make a gaussian profile
  create x 0 31
  set yo = 105.1*exp(-0.5*(x - 12.2)^2/2.2^2)
  gaussdev dg yo[] 0 1
  set dg = zero(yo)
  set y = yo + dg*sqrt(yo)

  # vgauss version
  $C0 = 15
  $C1 = 2
  $C2 = 100
  $C3 = 0
  vgauss x y con yf1 -q
  echo $C0 $C2 $C1 $C3
  $C0 = $C0 + $1
  vgauss -apply x yf3

  set dy1 = y - yf1
  set dy3 = y - yf3

  # fitarc version
  dimenup y im 1 31
  deimos fitarc im 0 0 1 31 -resid dim -max-iterations 0
  dimendown dim yf2
  set dy2 = y - yf2

  lim -n 1 x dy2; clear; box;
  
  plot x dy1 -c red -x hist
  plot x dy2 -c blue -x hist
  plot x dy3 -c black -x hist

  lim -n 2 x y; clear; box;

  plot x y  -c black -x hist -lw 3

  plot x yf2 -c blue -x hist -lw 3
  plot x yf1 -c red -x hist -lw 2
  plot x yf3 -c blue60 -x hist
end
