
list tests
 test1
 memtest1
end

# Test if imhist works
macro test1

 $PASS = 1

 local i

 mcreate buff 100 10

 zap buff 40 0 10 10 -v 17
 zap buff 60 5 5 2 -v 6

 imhist buff xvec yvec

 if ((xvec[1024] != 17) || (yvec[1024] != 100))
  $PASS = 0
  echo "Value mismatch: xvec[1024] yvec[1024] (should be 17,100)"
 end

 imhist -q buff xvec yvec -region 40 0 25 10 -range 0 10

 if ((xvec[1024] != 10) || (yvec[1024] != 100))
  $PASS = 0
  echo "Value mismatch: xvec[1024] yvec[1024] (should be 10,100)"
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  imhist -q buff xvec yvec -region 40 0 25 10 -range 0 10
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
