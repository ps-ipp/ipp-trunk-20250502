list tests
 test1
 memtest1
end

# Does sprintf work?
macro test1
 $PASS = 1

 local test_var

 sprint test_var "%7s %5.2f %9.3e" float 34.5 12630000

 if ("$test_var" != "  float 34.50 1.263e+07")
  $PASS = 0
 end

end

# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
  sprint test_var "%7s %5.2f %9.3e" float 34.5 12630000
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if (($endmem - $startmem)/10000 > 1.0)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end

end
