
list tests
 test1
 memtest1
end

# Test if applyfit2d works
macro test1

 $PASS = 1

 $Cnn = 2
 $CX0Y0 = 1
 $CX1Y0 = -4
 $CX2Y0 = 2
 $CX1Y1 = -3
 $CX0Y1 = 1.5
 $CX0Y2 = -2.5

 create x 0 5 0.01
 set y = 3*cos(2*3.14159*x/2.25)

 applyfit2d x y z

 if (abs(z[300]-12.625) > 0.001)
  $PASS = 0
  echo "Value mismatch: z[300]"
 end

end


# Memory test
# NOTE: requires test1 to be run first
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  applyfit2d x y z
 end
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if (($endmem - $startmem)/1000 > 1.0)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end
