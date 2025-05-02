
list tests
 test1
 memtest1
end

# Does concat work?
macro test1

 $PASS = 1

 create a 0 10
 set b = a

 concat a b

 if ((b[] != 20) || (b[10] != 0))
  $PASS = 0
  echo "Concat failed!: nelements: b[] b(10)= b[10]"
 end
end


# Memory Test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  create a 0 10
  set b = a
  concat a b
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
