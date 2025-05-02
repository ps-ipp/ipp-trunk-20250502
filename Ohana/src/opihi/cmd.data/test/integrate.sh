
list tests
 test1
 memtest1
end

# Test if integrate works
macro test1

 $PASS = 1

 create x 0 10 0.01
 set y = 1+2*x+3*x^2

 integrate x y 1 5

 if (abs ($sum-152) > 0.5)
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
  integrate x y 1 5
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
