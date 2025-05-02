
list tests
 test1
 memtest1
end

# Test if applyfit works
macro test1

 $PASS = 1

 $Cn = 2
 $C0 = 4
 $C1 = -2
 $C2 = 1

 create x 0 10

 applyfit x y

 if (y[5] != 19)
  $PASS = 0
  echo "Value mismatch: y[5]"
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  applyfit x y
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
