
list tests
 test1
 test2
 test3
end

macro test1
 $PASS = 1
 break -auto off

 create x -10 10 0.1

 $xo = 0.0
 $go = 2.0
 $Io = 10.0
 $so = 0.0

 set ytru = $Io * $go / ((x - $xo)^2 + $go^2 / 4.0) / 2 / $M_PI + $so
 gaussdev ystd ytru[] 0.0 1.0
 # set yobs = ytru + ystd * sqrt(abs(ytru))
 set yobs = ytru + ystd * 0.05

 $C0 = 0.5
 $C1 = 3
 $C2 = 20
 $C3 = 0
 # set dy = sqrt(abs(yobs))
 set dy = 0.05 + zero(yobs)

 vlorentz x yobs dy yfit
 lim x yobs; clear; box; plot -x 1 -c black x yobs; plot -c red x yfit

 if (abs($C0 - 0.0) > 0.01)
   $PASS = 0
 end
 if (abs($C1 - 3.0) > 0.01)
   $PASS = 0
 end
 if (abs($C2 - 5.0) > 0.01)
   $PASS = 0
 end
 if (abs($C3 - 0.0) > 0.01)
   $PASS = 0
 end
end

macro test1a
 $PASS = 1
 break -auto off

 create x -10 10 0.1

 $xo = 0.0
 $go = 2.0
 $Io = 10.0
 $so = 0.0

 set ytru = $Io * $go / ((x - $xo)^2 + $go^2 / 4.0) / 2 / $M_PI + $so
 gaussdev ystd ytru[] 0.0 1.0
 # set yobs = ytru + ystd * sqrt(abs(ytru))
 set yobs = ytru + ystd * 0.05
 set dy = 0.05 + zero(yobs)

 delete rC0 rC1 rC2 rC3 vC0 vC1 vC2 vC3

 for i 0 500

   $C0 = -1 + 2*rnd(0)
   $C1 = 0.5 + 5*rnd(0)
   $C2 = 2.0 + 30*rnd(0)
   $C3 = -10 + 20*rnd(0)
   
   concat $C0 rC0
   concat $C1 rC1
   concat $C2 rC2
   concat $C3 rC3

   vlorentz -q x yobs dy yfit
   # lim x yobs; clear; box; plot -x 1 -c black x yobs; plot -c red x yfit

   concat $C0 vC0
   concat $C1 vC1
   concat $C2 vC2
   concat $C3 vC3
 end

 if (abs($C0 - 0.0) > 0.01)
   $PASS = 0
 end
 if (abs($C1 - 3.0) > 0.01)
   $PASS = 0
 end
 if (abs($C2 - 5.0) > 0.01)
   $PASS = 0
 end
 if (abs($C3 - 0.0) > 0.01)
   $PASS = 0
 end
end

# noise of 0.1
macro test2
 $PASS = 1
 break -auto off

 create x -10 10 0.1
 set y = 1000 * exp(-0.5*x^2/3^2)
 set dy = (rnd(y) - 0.5)/0.5
 set y = y + dy

 $C0 = 0.5
 $C1 = 2
 $C2 = 900
 $C3 = 1

 vgauss -q x y dy yfit
 # lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
 # cursor

 if (abs($C0 - 0.0) > 0.01)
   $PASS = 0
 end
 if (abs($C1 - 3.0) > 0.01)
   $PASS = 0
 end
 if (abs($C2 - 1000.0) > 0.1)
   $PASS = 0
 end
 if (abs($C3 - 0.0) > 0.1)
   $PASS = 0
 end
end

# poisson-distributed noise
macro test3
 $PASS = 1
 break -auto off

 $C0o = 0.5
 $C1o = 2
 $C2o = 900
 $C3o = 1

 create x -10 10 0.1
 set y = $C3o + $C2o * exp(-0.5*(x - $C0o)^2/$C1o^2)

 gaussdev dY y[] 0.0 1.0
 set dy = dY * sqrt(y)
 set y = y + dy

 $C0 = $C0o + 2
 $C1 = $C1o + 2
 $C2 = $C2o + 50
 $C3 = $C3o + 10

 vgauss -q x y dy yfit
 # lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
 # cursor

 $dS = 3.0 / sqrt(y[])

 if (abs($C0 - $C0o) > $dS)
   $PASS = 0
 end
 if (abs($C1 - $C1o)/$C1o > $dS)
   $PASS = 0
 end
 if (abs($C2 - $C2o)/$C2o > $dS)
   $PASS = 0
 end
 if (abs($C3 - $C3o) > $dS)
   $PASS = 0
 end
end
