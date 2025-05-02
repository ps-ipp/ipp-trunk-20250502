list tests
 test1
 memtest1
end

# Does local work?
macro test1

 $PASS = 1

 local lvar

 $lvar = 5

 if ($?lvar != 1)
  $PASS = 0
  echo "Local variable failed to be created!"
 end

 if ($lvar != 5)
  $PASS = 0
  echo "Local variable value not assigned!"
 end

end


# Memory test for local
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  local lvar2
  $lvar2 = 9
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
