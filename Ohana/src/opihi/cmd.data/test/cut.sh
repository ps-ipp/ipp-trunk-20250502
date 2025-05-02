
list tests
 test1
 memtest1
end

macro test2

  mgaussdev z 50 100 0.0 1.0

  cut z x y_sum    x 0 0 z[][0] z[0][]
  cut z x y_median x 0 0 z[][0] z[0][] -median
  cut z x y_mean   x 0 0 z[][0] z[0][] -mean
  cut z x y_inner  x 0 0 z[][0] z[0][] -inner
  
end

macro checkrange
  if ($0 != 2)
    echo "USAGE: checkrange (Npts)"
    break
  end

  $Npts = $1
  if ($Npts % 2)
    $Ncenter = int($Npts / 2)
    $Nquarter = int(0.25 * $Npts)
    $Ns = $Ncenter - $Nquarter
    $Ne = $Ncenter + $Nquarter
  else
    $Ncenter = int($Npts / 2) - 1
    $Nquarter = int(0.25 * $Npts)
    $Ns = $Ncenter - $Nquarter
    $Ne = $Ncenter + $Nquarter + 1
  end
  if ($Ns < 0)
    echo "error: Ns < 0: $Ns $Ncenter $Nquarter"
  end
  if ($Ne >= $Npts)
    echo "error: Ne >= Npts: $Ne $Ncenter $Nquarter"
  end
  echo "$Npts : $Ncenter : $Nquarter : $Ns $Ne"
  for i 0 $Npts
    echo $i {($i >= $Ns) && ($i <= $Ne)}
  end
end


# Test if cut works
macro test1

 $PASS = 1

 mcreate tim 100 10
 zap tim 0 0 100 10 -v 10

 cut tim xdir imx X 40 4 60 6
 cut tim ydir imy Y 40 4 60 6

 if (xdir[] != 60)
  $PASS = 0
 end
 if (imx[] != 60)
  $PASS = 0
 end

 if (ydir[] != 6)
  $PASS = 0
 end
 if (imy[] != 6)
  $PASS = 0
 end

 if (imx[0] != 10*6)
  $PASS = 0
 end
 if (imy[0] != 10*60)
  $PASS = 0
 end
end

macro memtest1

 $i = 0
 mcreate tim 100 100
 cut tim xdir imx X 40 4 60 6
 # do one to set up memory that should stay used

 memory check
 for i 0 100
   cut tim xdir imx X 40 4 60 6
 end
 memory check
   
 for i 0 100
   cut tim xdir imx y 40 4 60 6
 end
 memory check
   
 for i 0 100
   cut -median tim xdir imx X 40 4 60 6
 end
 memory check
   
 for i 0 100
   cut -median tim xdir imx y 40 4 60 6
 end
 memory check
   
 for i 0 100
   cut -mean tim xdir imx X 40 4 60 6
 end
 memory check
   
 for i 0 100
   cut -mean tim xdir imx y 40 4 60 6
 end
 memory check
   
end
