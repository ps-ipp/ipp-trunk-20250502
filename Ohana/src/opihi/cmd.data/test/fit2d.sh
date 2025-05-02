
list tests
 test1
 test2
 memtest1
end

# fit a function without errors
macro test1
 $PASS = 1
 break -auto off

 create x 0 5 0.01
 set y = sin((2*3.14159*x)/2)
 set z = 5-4*x+x^2-3*y+6*x*y-2*y^2

 fit2d -q x y z 2

 if ($Cnn != 2)
   $PASS = 0
   echo "Function Order Incorrect!"
 end
 if (abs($CX0Y0 - 5) > 1e-5)
   $PASS = 0
   echo "Term CX0Y0 Incorrect!"
 end
 if (abs($CX1Y0 + 4) > 1e-5)
   $PASS = 0
   echo "Term CX1Y0 Incorrect!"
 end
 if (abs($CX2Y0 - 1) > 1e-5)
   $PASS = 0
   echo "Term CX2Y0 Incorrect!"
 end
 if (abs($CX0Y1 + 3) > 1e-5)
   $PASS = 0
   echo "Term CX0Y1 Incorrect!"
 end
 if (abs($CX1Y1 - 6) > 1e-5)
   $PASS = 0
   echo "Term CX1Y1 Incorrect!"
 end
 if (abs($CX0Y2 + 2) > 1e-5)
   $PASS = 0
   echo "Term CX0Y2 Incorrect!"
 end
end

# fit a function with errors
macro test2
 $PASS = 1
 break -auto off

 create x 0 5 0.01
 set y = sin((2*3.14159*x)/2)
 set dz = 0.1*rnd(x) - 0.05
 set z = 5-4*x+x^2-3*y+6*x*y-2*y^2+dz

 fit2d -q x y z 2

 if ($Cnn != 2)
   $PASS = 0
   echo "Function Order Incorrect!"
 end
 if (abs($CX0Y0 - 5) > 0.01)
   $PASS = 0
   echo "Term CX0Y0 Incorrect!"
 end
 if (abs($CX1Y0 + 4) > 0.01)
   $PASS = 0
   echo "Term CX1Y0 Incorrect!"
 end
 if (abs($CX2Y0 - 1) > 0.01)
   $PASS = 0
   echo "Term CX2Y0 Incorrect!"
 end
 if (abs($CX0Y1 + 3) > 0.01)
   $PASS = 0
   echo "Term CX0Y1 Incorrect!"
 end
 if (abs($CX1Y1 - 6) > 0.01)
   $PASS = 0
   echo "Term CX1Y1 Incorrect!"
 end
 if (abs($CX0Y2 + 2) > 0.01)
   $PASS = 0
   echo "Term CX0Y2 Incorrect!"
 end
end

# Memory Test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 create x 0 5 0.01
 set y = sin((2*3.14159*x)/2)
 set dz = 0.1*rnd(x) - 0.05
 set z = 5-4*x+x^2-3*y+6*x*y-2*y^2+dz

 for i 0 1000
  fit2d -q x y z 2
 end
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end
