list tests
 test1
 test2
 memtest1
 memtest2
end

# Does strpop work with variables?
macro test1

 $PASS = 1

 $tstr = a test string

 local a b c

 strpop tstr a
 strpop tstr b
 strpop tstr c

 if (("$a" != "a") || ("$b" != "test") || ("$c" != "string"))
  $PASS = 0
  echo "Incorrect value returned!"
 end
end


# Does strpop work with lists?
macro test2

 $PASS = 1

 list tlis -split list of strings

 local a b c

 strpop tlis:0 a
 strpop tlis:1 b
 strpop tlis:2 c

 if (("$a" != "list") || ("$b" != "of") || ("$c" != "strings"))
  $PASS = 0
  echo "Incorrect value returned!"
 end
end


# Memory test for variables
macro memtest1

 local i tstr a b c d

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  $tstr = one two three
  strlen tstr a
  strlen tstr b
  strlen tstr c
  strlen tstr d
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


# Memory test for lists
macro memtest2

 local i tstr a b c d

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  list tstr -split one two three
  strlen tstr:0 a
  strlen tstr:1 b
  strlen tstr:2 c
  strlen tstr:0 d
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
