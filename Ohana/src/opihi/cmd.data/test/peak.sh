
list tests
 test1
 test2
 test3
 testmem1
end

# test using full range
macro test1
 $PASS = 1
 break -auto off

 create x 0 100
 set y = zero (x)
 y[50] = 1

 peak -q x y 0 100

 if ($peakpos != 50)
   $PASS = 0
 end
 if ($peaknum != 50)
   $PASS = 0
 end
 if ($peakval != 1)
   $PASS = 0
 end
end

# test using auto range
macro test2
 $PASS = 1
 break -auto off

 create x 0 100
 set y = zero (x)
 y[50] = 1

 peak -q x y 

 if ($peakpos != 50)
   $PASS = 0
 end
 if ($peaknum != 50)
   $PASS = 0
 end
 if ($peakval != 1)
   $PASS = 0
 end
end

# test using constrained range
macro test3
 $PASS = 1
 break -auto off

 create x 0 100
 set y = zero (x)
 y[60] = 2
 y[50] = 1
 y[40] = 2

 peak -q x y 45 55

 if ($peakpos != 50)
   $PASS = 0
 end
 if ($peaknum != 50)
   $PASS = 0
 end
 if ($peakval != 1)
   $PASS = 0
 end
end

# test memory usage
macro testmem1
 break -auto off

 create x 0 1000
 set y = zero (x)
 y[500] = 100

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
   peak -q x y 400 600
 end
 
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end

end
