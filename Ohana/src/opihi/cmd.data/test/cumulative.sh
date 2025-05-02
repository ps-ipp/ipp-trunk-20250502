
list tests
 test1
 memtest1
end

# Does cumulative work?
macro test1

 $PASS = 1

 create a 5 15

 cumulative a acum

 if (acum[5] != 45)
  $PASS = 0
  echo "Cumulative failed!: nelements: acum[] acum(5)= acum[5]"
 end
end


# Memory Test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  create a 5 15
  cumulative a acum
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
