
list tests
 test1
 memtest1
end

# Test if dimendown works
macro test1

 $PASS = 1

 create tvec 0 1000

 dimenup tvec timg 10 100

 stats -q timg 6 34 1 1

 if ($MEAN != 346)
  $PASS = 0
  echo "Value mismatch: $MEAN (should be 346)"
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  dimenup tvec timg 10 100
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
