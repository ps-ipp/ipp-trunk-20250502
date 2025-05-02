
list tests
 test1
 test2
 test3
end

macro test1
 $PASS = 1
 break -auto off

 create x -10 10 0.1
 set y = 0 + 5 / (1 + exp((x - 3)/4))

 $C0 = 1
 $C1 = 2
 $C2 = 10
 $C3 = 1
 set dy = sqrt(y)

 vsigmoid -q x y dy yfit
 lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
 # cursor

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
 set y = 50 + 1000 / (1 + exp((x - 3)/2))
 set dy = (rnd(y) - 0.5)/0.5
 set y = y + dy

 $C0 = 2
 $C1 = 3
 $C2 = 900
 $C3 = 1

 vsigmoid -q x y dy yfit
 lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
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

 $C0o = 3
 $C1o = 2
 $C2o = 20
 $C3o = -2

 create x -10 10 0.1
 set y = $C3o + $C2o / (1 + exp((x - $C0o)/$C1o))

 gaussdev dY y[] 0.0 1.0
 set dy = dY * sqrt(abs(y))
 set y = y + dy

 $C0 = $C0o + 2
 $C1 = $C1o + 2
 $C2 = $C2o + 50
 $C3 = $C3o + 10

 vsigmoid x y dy yfit
 lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
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

macro test1rev
 $PASS = 1
 break -auto off

 create x -10 10 0.1
 set y = 5 / (1 + exp(-(x - 3)/4))

 $C0 = 1
 $C1 = 2
 $C2 = 10
 $C3 = 1
 set dy = sqrt(abs(y))

 vsigmoid -r -q x y dy yfit
 lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
 # cursor

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
macro test2rev
 $PASS = 1
 break -auto off

 create x -10 10 0.1
 set y = 50 + 1000 / (1 + exp(-(x - 3)/2))
 set dy = (rnd(y) - 0.5)/0.5
 set y = y + dy

 $C0 = 2
 $C1 = 3
 $C2 = 900
 $C3 = 1

 vsigmoid -r -q x y dy yfit
 lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
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
macro test3rev
 $PASS = 1
 break -auto off

 $C0o = 3
 $C1o = 2
 $C2o = 20
 $C3o = -2

 create x -10 10 0.1
 set y = $C3o + $C2o / (1 + exp(-(x - $C0o)/$C1o))

 gaussdev dY y[] 0.0 1.0
 set dy = dY * sqrt(abs(y))
 set y = y + dy

 $C0 = $C0o + 2
 $C1 = $C1o + 2
 $C2 = $C2o + 50
 $C3 = $C3o + 10

 vsigmoid -r -q x y dy yfit
 lim x y; clear; box; plot -x 1 -c black x y; plot -c red x yfit
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

macro memtest1

 create x -10 10 0.1
 set y = 0 + 5 / (1 + exp((x - 3)/4))

 $C0 = 1
 $C1 = 2
 $C2 = 10
 $C3 = 1
 set dy = sqrt(y)
 set yfit = zero(y)

 $i = 0

 memory check
 for i 0 1000
   vsigmoid -q x y dy yfit
 end
 memory check
   
end

macro memtest2

 create x -10 10 0.1
 set y = 0 + 5 / (1 + exp((x - 3)/4))

 $C0 = 1
 $C1 = 2
 $C2 = 10
 $C3 = 1
 set dy = sqrt(y)
 set yfit = zero(y)

 $i = 0

 memory check
 for i 0 1000
   vsigmoid -q -r x y dy yfit
 end
 memory check
   
end

