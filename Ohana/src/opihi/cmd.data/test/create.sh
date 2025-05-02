
list tests
 test1
 memtest1
end

# Test create function
macro test1
 
 $PASS = 1

 create x 0 10 0.5

 if ((x[1] != 0.5) || (x[9] != 4.5))
  $PASS = 0
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  create y 0 10 0.1
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
