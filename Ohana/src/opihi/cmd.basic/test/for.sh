
list tests
 test1
 test2
 test3
 test4
 test5
 memtest1
 memtest2
 memtest3
end

# do we loop up correctly?
macro test1

  $PASS = 0

  local i

  for i 0 100
  end    
  
 if ($i == 99)
   $PASS = 1
 else
   $PASS = 0
 end

end

# do we loop down correctly?
macro test2

  $PASS = 0

  local i

  for i 100 0 -1
  end    
  
 if ($i == 1)
   $PASS = 1
 else
   $PASS = 0
   echo "i : $i"
 end

end

# do we loop up in small steps correctly?
macro test3

  $PASS = 0

  local i N

  $N = 0
  for i 0 100 0.1
   $N = $N + 1
  end    
  
 if (($i == 99.9) && ($N == 1000))
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
   echo "N: $N"
 end

end

# do we loop down in small steps correctly?
macro test4

  $PASS = 0

  local i N

  $N = 0
  for i 100 0 -0.1
   $N = $N + 1
  end    
  
 if (($i == 0.1) && ($N == 1000))
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
   echo "N: $N"
 end

end

# do we break from a loop correctly
macro test5

  $PASS = 0

  break -auto off

  local i N

  $N = 0
  for i 0 100
   $N = $N + 1
   if ($i == 30)
     break
   end
  end    

  $PASS = 1
  
  if (($i != 30) || ($N != 31))
    $PASS = 0
    echo "i: $i"
    echo "N: $N"
  end
end

# check memleaks
macro memtest1

 $PASS = 0

 local i N


 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks with many loop lines
macro memtest2

 $PASS = 0

 local i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
  echo "test line in loop"
  echo "test line in loop"
  echo "test line in loop"
  echo "test line in loop"
  echo "test line in loop"
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 80)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks on break
macro memtest3

 $PASS = 0

 local i N

 break -auto off

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
  for j 0 5
    break
    echo "test line in loop"
    echo "test line in loop"
    echo "test line in loop"
    echo "test line in loop"
    echo "test line in loop"
  end
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($i != 9999)
   $PASS = 0
   echo "break jumped outer loop: i = $i" 
 end

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end
