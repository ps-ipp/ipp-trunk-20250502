
list tests
 test1
 memtest1
end

# Test if interpolate works
macro test1

 $PASS = 1

 create x0 0 10 0.1
 set y0 = 1+2*x0+3*x0^2
 create x1 0 10 0.001
 interpolate x0 y0 x1 y1
 integrate x1 y1 1 5

 if (abs ($sum-152) > 0.08)
  $PASS = 0
  echo "Inaccurate result (should be 152): $sum"
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  interpolate x0 y0 x1 y1
 end
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 128)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end
