
macro makegauss
  if ($0 != 6)
    echo "USAGE: makegauss (Io) (mean) (sigma) (dy) (color)"
    break
  end

  set f = $1*exp(-0.5*($4-$2)^2 / $3^2)
  plot -c $5 $4 f -x 0
end

macro plot1
  data data/yraw_01.dat
  read y 1
  histogram y ny 0 100 1 -range dy
  lim dy ny; clear; box; plot dy ny -x 1
  makegauss 100 16.334488 3.818669 dy red
  makegauss 100 16.773050 3.790289 dy blue
end  

macro plot2
  data data/yraw_02.dat
  read y 1
  histogram y ny 0 10000 10 -range dy
  lim dy ny; clear; box; plot dy ny -x 1
  makegauss 10 746.773743 621.665955 dy red
  makegauss 10 899.454041 568.609497 dy blue
end  

macro plot3
  data data/yraw_03.dat
  read y 1
  histogram y ny -1 1000 1 -range dy
  lim 100 300 -1 60; clear; box; plot dy ny -x 1
  makegauss 47 175.329529 14.232742 dy red ; # robust
  makegauss 47 148.204117 66.969803 dy blue ; # clipped
  makegauss 47 178.721268 10.870105 dy green ; # fitted v2
end  

macro plot4
  data data/yraw_04.dat
  read y 1
  histogram y ny -1.1 0.1 0.01 -range dy
  lim dy ny; clear; box; plot dy ny -x 1
  makegauss 100 -0.984124 0.029106 dy red
  makegauss 100 -1.000000 0.029106 dy blue
end  
