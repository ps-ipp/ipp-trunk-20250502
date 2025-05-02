list tests
 test1
 memtest1
end

# Does substr work?
macro test1

 $PASS = 1

 local tstr ss

 $tstr = "riddle me this"

 substr $tstr 4 8 ss

 if ("$ss" != "le me th")
  $PASS = 0
  echo "Incorrect substring returned!"
 end
end


# Memory test
macro memtest1

 local i tstr ss

 $tstr = "riddle me this"

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
  substr $tstr 4 8 ss
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end

end
