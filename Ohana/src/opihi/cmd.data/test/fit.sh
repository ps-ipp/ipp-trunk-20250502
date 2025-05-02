
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
