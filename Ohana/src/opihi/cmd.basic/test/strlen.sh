list tests
 test1
 memtest1
end

# Does strlen work?
macro test1
 
 $PASS = 1

 $tstr = "Test string"

 strlen $tstr len

 if ($len != 11)
  $PASS = 0
  echo "Incorrect length: $len"
 end

end

# Memory Test
macro memtest1

 local i

 $tstr = "Test string"

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  strlen $tstr len
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
