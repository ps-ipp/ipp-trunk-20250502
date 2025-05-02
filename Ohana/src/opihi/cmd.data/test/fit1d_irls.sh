
list tests
 test1
 test2
 test3
 test4
 test5
 test6
 test7
 test8
end

## NOTE: this is not a TAP test
macro mean.test
  if ($0 != 2)
    echo "USAGE: mean.test (errorbar)"
    break
  end

  create x 0 100

  delete C0fit dC0fit Cnfit Mfit dMfit
  for i 0 500
    gaussdev y x[] 0 1.0   ; # mean of 0, sigma of 1
    
    set dy = 1.0 + zero(x) ; # set error to be correct
    fit1d_irls -q x y 0 -dy dy -wt wt -mask mask -use-median -Nboot 1000

    concat $C0    C0fit
    concat $dC0  dC0fit

    vstat -q y
    concat $MEAN Mfit
    concat {$SIGMA / sqrt($NPTS)} dMfit
  end

  vstat -q C0fit;  set MeanC  = $MEAN; set StdevC = $SIGMA
  vstat -q dC0fit; set MeanDC = $MEAN
  vstat -q Mfit;   set MeanM  = $MEAN; set StdevM = $SIGMA
  vstat -q dMfit;  set MeanDM = $MEAN

  fprintf "%7.3f %7.3f" $MeanC $MeanM
  fprintf "%7.3f %7.3f" $StdevC $StdevM
end

macro cliptest.plot
  if ($0 != 3)
    echo "USAGE: cliptest.plot (errorbar) (output)"
    break
  end

  set dy = $1 + zero(x) ; # set error to be correct
  fit1d_irls x y 0 -dy dy -wt wt -mask mask -use-median

  subset ygood = y where not(mask)

  vstat y
  vstat ygood

  histogram wt Nwt 0 1 0.005 -range dw
  vstat wt

  subset wts = wt where (wt < 0.3*$MEDIAN)
  echo "0.3 of MEDIAN : wts[]"

  subset wts = wt where (wt < 0.3*$MEAN)
  echo "0.3 of MEAN : wts[]"

  dev -n 0; resize 1200 600
  label -fn courier 24; 
  section a 0.4 0.0 0.6 1.0; lim x y; clear; box -ypad 0.1 -xpad 3.2 -labelpadx 2.5 -labels 1001; label -x sequence +y value +x "red: clipped, blue: unclipped"; 
  plot x y; plot -c blue -pt 7 x y where not(mask); plot -c red -pt 7 -lw 2 x y where mask

  section b 0.0 0.0 0.4 1.0; lim dw Nwt; box +ypad 0.1 -xpad 3.2 -labelpadx 2.5; label -x "weight" -y "Npoints"; plot -x 1 dw Nwt
  png -name $2
end

## NOTE: this is not a TAP test
macro cliptest

  create x 0 100
  gaussdev y x[] 0 1.0   ; # mean of 0, sigma of 1

  if (1)
    y[10] = 5.0
    y[20] = 7.0
    y[30] = 4.0
  end

  cliptest.plot 1.0 irls.sample.v0.png

  cliptest.plot 0.1 irls.sample.v1.png

  cliptest.plot 0.01 irls.sample.v2.png
end

## NOTE: this is not a TAP test
macro outlier.test
  if ($0 != 2)
    echo "USAGE: outlier.test (errorbar)"
    break
  end

  create x 0 100

  delete C0fit dC0fit Cnfit Mfit dMfit
  for i 0 300
    gaussdev y x[] 0 1.0   ; # mean of 0, sigma of 1
    
    if (0)
      y[10] = 5.0
      y[20] = 7.0
      y[30] = 4.0
    end
    
    set dy = $1 + zero(x) ; # set error to be correct
    fit1d_irls -q x y 0 -dy dy -wt wt -mask mask -use-median -Nboot 1000
    concat $C0 C0fit
    concat $dC0 dC0fit
    concat $Cnfit Cnfit

    vstat -q y
    concat $MEAN Mfit
    concat {$SIGMA / sqrt($NPTS)} dMfit
  end

  vstat C0fit
  vstat dC0fit
  vstat Cnfit
  vstat Mfit
  vstat dMfit
end

# fit a line without errors
macro test1
 $PASS = 1
 break -auto off

 delete -q x y

 create x 0 100
 set y = 3 + 5*x
 fit -q x y 1

 if ($Cn != 1)
   $PASS = 0
 end
 if (abs($C0 - 3) > 1e-5)
   $PASS = 0
 end
 if (abs($C1 - 5) > 1e-5)
   $PASS = 0
 end
end

# fit a line with errors
macro test2
 $PASS = 1
 break -auto off

 delete -q x y dy

 create x 0 100
 set dy = 0.1*rnd(x) - 0.05
 set y = 3 + 5*x + dy
 fit -q x y 1

 if ($Cn != 1)
   $PASS = 0
   echo "wrong number of elements : $Cn"
 end
 if (abs($C0 - 3) > 0.06)
   $PASS = 0
   echo "wrong value for C0 : $C0"
 end
 if (abs($C1 - 5) > 0.06)
   $PASS = 0
   echo "wrong value for C1 : $C1"
 end
end

# fit a line with errors and weights
macro test3
 $PASS = 1
 break -auto off

 delete -q x y dy

 create x 0 100
 set dy = 0.1*rnd(x) - 0.05
 set y = 3 + 5*x + dy
 set dy = 0.1 + zero(x)
 fit -q x y 1 -dy dy

 if ($Cn != 1)
   $PASS = 0
 end
 if (abs($C0 - 3) > 0.06)
   $PASS = 0
 end
 if (abs($C1 - 5) > 0.06)
   $PASS = 0
 end
end

# fit a line with errors, weights, and outliers 
macro test4
 $PASS = 1
 break -auto off

 delete -q x y dy

 create x 0 100
 set dy = 0.1*rnd(x) - 0.05
 set y = 3 + 5*x + dy
 set dy = 0.1 + zero(x)
 y[5] = 23
 y[20] = -10
 y[50] = 0.0
 fit -q x y 1 -dy dy -clip 3 3

 if ($Cn != 1)
   $PASS = 0
 end
 if ($Cnv != 97)
   $PASS = 0
 end
 if (abs($C0 - 3) > 0.06)
   $PASS = 0
 end
 if (abs($C1 - 5) > 0.06)
   $PASS = 0
 end
end

# fit a quadratic without errors
macro test5
 $PASS = 1
 break -auto off

 delete -q x y

 create x 0 100
 set y = 3 + 5*x - 4*x^2
 fit -q x y 2

 if ($Cn != 2)
   $PASS = 0
 end
 if (abs($C0 - 3) > 1e-5)
   $PASS = 0
 end
 if (abs($C1 - 5) > 1e-5)
   $PASS = 0
 end
 if (abs($C2 + 4) > 1e-5)
   $PASS = 0
 end
end

# fit a quadratic with errors
macro test6
 $PASS = 1
 break -auto off

 delete -q x y dy

 create x 0 100
 set dy = 0.1*rnd(x) - 0.05
 set y = 3 + 5*x - 4*x^2 + dy
 fit -q x y 2

 if ($Cn != 2)
   $PASS = 0
 end
 if (abs($C0 - 3) > 0.06)
   $PASS = 0
 end
 if (abs($C1 - 5) > 0.06)
   $PASS = 0
 end
 if (abs($C2 + 4) > 0.06)
   $PASS = 0
 end
end

# fit a quadratic with errors and weights
macro test7
 $PASS = 1
 break -auto off

 delete -q x y dy

 create x 0 100
 set dy = 0.1*rnd(x) - 0.05
 set y = 3 + 5*x - 4*x^2 + dy
 set dy = 0.1 + zero(x)
 fit -q x y 2 -dy dy

 if ($Cn != 2)
   $PASS = 0
 end
 if (abs($C0 - 3) > 0.06)
   $PASS = 0
 end
 if (abs($C1 - 5) > 0.06)
   $PASS = 0
 end
 if (abs($C2 + 4) > 0.06)
   $PASS = 0
 end
end

# fit a quadratic with errors, weights, and outliers 
macro test8
 $PASS = 1
 break -auto off

 delete -q x y dy

 create x 0 100
 set dy = 0.1*rnd(x) - 0.05
 set y = 3 + 5*x - 4*x^2 + dy
 set dy = 0.1 + zero(x)
 y[5] = 23
 y[20] = -10
 y[50] = 0.0

 # it takes 4 iterations to successfully reject the outliers above...
 fit -q x y 2 -dy dy -clip 3 4

 if ($Cn != 2)
   $PASS = 0
 end
 if ($Cnv != 97)
   $PASS = 0
 end
 if (abs($C0 - 3) > 0.06)
   $PASS = 0
 end
 if (abs($C1 - 5) > 0.06)
   $PASS = 0
 end
 if (abs($C2 + 4) > 0.06)
   $PASS = 0
 end
end

macro try.priors
 create x 0 100
 set y = 5.2 + 2.3*x
 set ytru = 5.2 + 2.3*x
 gaussdev dyoff ytru[] 0.0 5.0
 set dy = 5.0 + zero(ytru)
 set yobs = ytru + dyoff
 yobs[1] = 210
 $P0v = 5.0
 $dP0 = 0.5

 fit1d_irls x yobs 1 -dy dy -yfit yfit -max-iterations 0 -use-priors
# fit1d_irls x yobs 1 -dy dy -yfit yfit -max-iterations 5
# fit1d_irls  x yobs 1 -dy dy -yfit yfit -max-iterations 0

 lim x yobs; clear; box; 
 plot -c blue -x line -lw 2 x ytru
 plot x yobs -pt cir -op 0.5
 plot -c red -x line -lw 2 x yfit -op 0.5
end
 
macro run.priors
 create x 0 100
 set y = 5.2 + 2.3*x
 set ytru = 5.2 + 2.3*x
 gaussdev dyoff ytru[] 0.0 5.0
 set dy = 5.0 + zero(ytru)
 set yobs = ytru + dyoff

 $P0v = NAN
 $P1v = NAN

 $n = 0

 if ($n == 0)
   $P0v = 4.5
   $dP0 = 1.0
 end
 if ($n == 1)
   $P1v = 2.3
   $dP1 = 0.5
 end

 set yobsS = yobs

 delete -q C0norm C0irls C0prio C0irpi
 delete -q N0norm N0irls N0prio N0irpi
 for i 0 5000

   set yobs = yobsS
   for j 0 10
     $Npt = int(yobs[]*rnd(0))
     yobs[$Npt] = 1000*rnd(0) - 500
   end
   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 0
   concat $C$n C0norm; vstat -q ymsk; concat $TOTAL N0norm
   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 0 -use-priors
   concat $C$n C0prio; vstat -q ymsk; concat $TOTAL N0prio
   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 5
   concat $C$n C0irls; vstat -q ymsk; concat $TOTAL N0irls
   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 5 -use-priors
   concat $C$n C0irpi; vstat -q ymsk; concat $TOTAL N0irpi
 end

  histogram C0norm nCnorm -30 30 0.25 -range dC
  histogram C0prio nCprio -30 30 0.25 -range dC
  histogram C0irls nCirls -30 30 0.25 -range dC
  histogram C0irpi nCirpi -30 30 0.25 -range dC

  lim dC nCirls; clear; box; 

  plot dC nCnorm -x hist -op 0.6 -lw 3 -c black  ; vstat C0norm 
  plot dC nCprio -x hist -op 0.6 -lw 3 -c blue	 ; vstat C0prio
  plot dC nCirls -x hist -op 0.6 -lw 3 -c red    ; vstat C0irls
  plot dC nCirpi -x hist -op 0.6 -lw 3 -c blue60 ; vstat C0irpi

  if ($n == 0)
    line -c red -lt dot $P0v 0 to $P0v 1000
    line -c blue -lt dot 5.2 0 to 5.2 1000
  else
    line -c red -lt dot $P1v 0 to $P1v 1000
    line -c blue -lt dot 2.3 0 to 2.3 1000
  end
end
 
macro run.2dpriors.fixmeas
 $C0tru = +3
 $C1tru = -4
 $Sigma =  5

 create x -25 25
 set ytru = $C0tru + $C1tru*x
 gaussdev yoff ytru[] 0.0 $Sigma

 set dy = $Sigma + zero(ytru)
 set yobs = ytru + yoff

 $P0v = NAN
 $P1v = NAN

 $P0v = $C0tru + 0.5
 $dP0 = 1.0

 $P1v = $C1tru + 0.2
 $dP1 = 0.5

 set yobsSave = yobs

 delete -q N0norm N0irls N0prio N0irpi
 delete -q C0norm C0irls C0prio C0irpi
 delete -q C1norm C1irls C1prio C1irpi

 for i 0 5000

   set yobs = yobsSave

   # inject outliers
   for j 0 10
     $Npt = int(yobs[]*rnd(0))
     yobs[$Npt] = 1000*rnd(0) - 500
   end

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 0
   concat $C0 C0norm; concat $C1 C1norm; vstat -q ymsk; concat $TOTAL N0norm

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 0 -use-priors
   concat $C0 C0prio; concat $C1 C1prio; vstat -q ymsk; concat $TOTAL N0prio

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 5
   concat $C0 C0irls; concat $C1 C1irls; vstat -q ymsk; concat $TOTAL N0irls

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 5 -use-priors
   concat $C0 C0irpi; concat $C1 C1irpi; vstat -q ymsk; concat $TOTAL N0irpi
 end

 histogram C0norm nC0norm -30 30 0.25 -range dC
 histogram C0prio nC0prio -30 30 0.25 -range dC
 histogram C0irls nC0irls -30 30 0.25 -range dC
 histogram C0irpi nC0irpi -30 30 0.25 -range dC

 histogram C1norm nC1norm -30 30 0.25 -range dC
 histogram C1prio nC1prio -30 30 0.25 -range dC
 histogram C1irls nC1irls -30 30 0.25 -range dC
 histogram C1irpi nC1irpi -30 30 0.25 -range dC

 dev -n 0
 lim dC nC0irpi; clear; box; 
 line -c red  -lw 2 -lt dot $P0v   0 to $P0v   1000
 line -c blue -lw 2 -lt dot $C0tru 0 to $C0tru 1000
 plot dC nC0norm -x hist -op 0.6 -lw 3 -c black  ; vstat C0norm 
 plot dC nC0prio -x hist -op 0.6 -lw 3 -c blue	 ; vstat C0prio
 plot dC nC0irls -x hist -op 0.6 -lw 3 -c red    ; vstat C0irls
 plot dC nC0irpi -x hist -op 0.6 -lw 3 -c blue60 ; vstat C0irpi

 dev -n 1
 lim dC nC1irpi; clear; box; 
 line -c red  -lw 2 -lt dot $P1v   0 to $P1v   1000
 line -c blue -lw 2 -lt dot $C1tru 0 to $C1tru 1000
 plot dC nC1norm -x hist -op 0.6 -lw 3 -c black  ; vstat C1norm 
 plot dC nC1prio -x hist -op 0.6 -lw 3 -c blue	 ; vstat C1prio
 plot dC nC1irls -x hist -op 0.6 -lw 3 -c red    ; vstat C1irls
 plot dC nC1irpi -x hist -op 0.6 -lw 3 -c blue60 ; vstat C1irpi

end
 
macro run.2dpriors
 $C0tru = +3
 $C1tru = -4
 $Sigma =  5

 create x -25 25
 set ytru = $C0tru + $C1tru*x

 set dy = $Sigma + zero(ytru)

 $P0v = NAN
 $P1v = NAN

 $P0v = $C0tru + 0.5
 $dP0 = 1.0

 $P1v = $C1tru + 0.2
 $dP1 = 0.5


 delete -q N0norm N0irls N0prio N0irpi
 delete -q C0norm C0irls C0prio C0irpi
 delete -q C1norm C1irls C1prio C1irpi

 for i 0 5000

   gaussdev yoff ytru[] 0.0 $Sigma
   set yobs = ytru + yoff

   # inject outliers
   for j 0 10
     $Npt = int(yobs[]*rnd(0))
     yobs[$Npt] = 1000*rnd(0) - 500
   end

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 0
   concat $C0 C0norm; concat $C1 C1norm; vstat -q ymsk; concat $TOTAL N0norm

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 0 -use-priors
   concat $C0 C0prio; concat $C1 C1prio; vstat -q ymsk; concat $TOTAL N0prio

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 5
   concat $C0 C0irls; concat $C1 C1irls; vstat -q ymsk; concat $TOTAL N0irls

   fit1d_irls -q x yobs 1 -dy dy -yfit yfit -mask ymsk -max-iterations 5 -use-priors
   concat $C0 C0irpi; concat $C1 C1irpi; vstat -q ymsk; concat $TOTAL N0irpi
 end
end

macro plot.2dpriors 

 if (1)
   histogram C0norm nC0norm -30 30 0.25 -range dC
   histogram C0prio nC0prio -30 30 0.25 -range dC
   histogram C0irls nC0irls -30 30 0.25 -range dC
   histogram C0irpi nC0irpi -30 30 0.25 -range dC
   
   histogram C1norm nC1norm -30 30 0.25 -range dC
   histogram C1prio nC1prio -30 30 0.25 -range dC
   histogram C1irls nC1irls -30 30 0.25 -range dC
   histogram C1irpi nC1irpi -30 30 0.25 -range dC
   
   dev -n 0
   lim dC nC0irpi; clear; box; 
   line -c red  -lw 2 -lt dot $P0v   0 to $P0v   1000
   line -c blue -lw 2 -lt dot $C0tru 0 to $C0tru 1000
   plot dC nC0norm -x hist -op 0.6 -lw 3 -c black  ; vstat C0norm 
   plot dC nC0prio -x hist -op 0.6 -lw 3 -c blue	 ; vstat C0prio
   plot dC nC0irls -x hist -op 0.6 -lw 3 -c red    ; vstat C0irls
   plot dC nC0irpi -x hist -op 0.6 -lw 3 -c blue60 ; vstat C0irpi
   
   dev -n 1
   lim dC nC1irpi; clear; box; 
   line -c red  -lw 2 -lt dot $P1v   0 to $P1v   1000
   line -c blue -lw 2 -lt dot $C1tru 0 to $C1tru 1000
   plot dC nC1norm -x hist -op 0.6 -lw 3 -c black  ; vstat C1norm 
   plot dC nC1prio -x hist -op 0.6 -lw 3 -c blue	 ; vstat C1prio
   plot dC nC1irls -x hist -op 0.6 -lw 3 -c red    ; vstat C1irls
   plot dC nC1irpi -x hist -op 0.6 -lw 3 -c blue60 ; vstat C1irpi
 end

 dev -n 2
 $C0range = 50; $C1range = 5
 resize 1800 1800
 clear -s
 section a0 0.0 0.0 0.5 0.5
 section a1 0.5 0.0 0.5 0.5
 section a2 0.0 0.5 0.5 0.5
 section a3 0.5 0.5 0.5 0.5

 section a0; subplot.2dprior C0norm C1norm
 section a1; subplot.2dprior C0prio C1prio
 section a2; subplot.2dprior C0irls C1irls
 section a3; subplot.2dprior C0irpi C1irpi

 dev -n 3
 $C0range = 8; $C1range = 2
 resize 1800 1800
 clear -s
 section a0 0.0 0.0 0.5 0.5
 section a1 0.5 0.0 0.5 0.5
 section a2 0.0 0.5 0.5 0.5
 section a3 0.5 0.5 0.5 0.5

 section a0; subplot.2dprior C0norm C1norm
 section a1; subplot.2dprior C0prio C1prio
 section a2; subplot.2dprior C0irls C1irls
 section a3; subplot.2dprior C0irpi C1irpi
end

macro subplot.2dprior 
 if ($0 != 3)
   echo "USAGE: subplot.2dprior (vec) (vec)"
   break
 end
    
 lim {$C0tru - $C0range} {$C0tru + $C0range} {$C1tru - $C1range} {$C1tru + $C1range}
 box; label -x C0 -y C1 

 plot -op 0.1 -pt cir -sz 1 -c blue $1 $2

 line -c blue -lw 2 -lt dot $C0tru -1000 to $C0tru 1000
 line -c red  -lw 2 -lt dot $P0v   -1000 to $P0v   1000
 line -c blue -lw 2 -lt dot -1000 $C1tru to 1000 $C1tru
 line -c red  -lw 2 -lt dot -1000   $P1v to 1000 $P1v
end
