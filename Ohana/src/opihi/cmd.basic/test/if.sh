
list tests
 test1
 test2
 test3
 test4
 test5
 test6
 test7
 test8
 test9
 test10
 test11
 test12
 test13a
 test13
 test14
end

# basic logical test
macro test1

 local a

 $a = 1

 if ($a == 1)
   $PASS = 1
 else
   $PASS = 0
 end
end


# basic logical test
macro test2

 local a b

 $a = 1
 $b = 2

 if (($a == 1) && ($b == 2))
   $PASS = 1
 else
   $PASS = 0
 end
end


# basic logical test
macro test3

 local a b

 $a = 1
 $b = 5

 if (($a == 1) && ($b < 10))
   $PASS = 1
 else
   $PASS = 0
 end
end


# basic logical test
macro test4

 local a

 $a = 0

 if ($a == 1)
   $PASS = 0
 else
   $PASS = 1
 end
end


# basic logical test
macro test5

 local a

 $a = 1

 if ($a > 0)
   $PASS = 1
 else
   $PASS = 0
 end
end


# basic logical test
macro test6

 local a

 $a = 1

 if ($a < 2)
   $PASS = 1
 else
   $PASS = 0
 end
end


# basic logical test
macro test7

 local a

 $a = test

 if ("$a" == "test")
   $PASS = 1
 else
   $PASS = 0
 end
end


# basic logical test
macro test8

 local a

 $a = foobar

 if ("$a" == "test")
   $PASS = 0
 else
   $PASS = 1
 end
end


# basic logical test
macro test9

 local a b

 $a = 1
 $b = 2

 if (($a == 0) || ($b == 2))
   $PASS = 1
 else
   $PASS = 0
 end
end


# basic logical test
macro test10

 local a b

 $a = 1
 $b = 2

 if (($a == 0) || ($b == 1))
   $PASS = 0
 else
   $PASS = 1
 end
end


# basic logical test
macro test11

 local a b

 $a = 1
 $b = 2

 if (($a <= 0) || ($b >= 3))
   $PASS = 0
 else
   $PASS = 1
 end
end


# basic logical test
macro test12

 local a

 $a = 1

 if ($a != 1)
   $PASS = 0
 else
   $PASS = 1
 end
end


# check memleaks
macro test13a

 local a b i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 $a = 1
 for i 0 1000
   if ($a == 1)
    echo "run"    
   end
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end

# check memleaks
macro test13

 local a b i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 $a = 1
 $b = 2
 for i 0 1000
   if (($a == 1) && ($b == 2))
    echo "run"    
   end
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end

# memory test using continue
macro test14

 local a b i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 $a = 1
 $b = 2
 for i 0 1000
   if (($a == 1) && ($b == 2))
    continue
    echo "run"    
   end
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end

# memory test using continue
macro test15

 local a b i N bool

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 $a = 1
 $b = 2
 for i 0 1000
   $bool = (($a == 1) && ($b == 2))
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end

# check memleaks
macro memtest1

 local a b i N

 $i = 0
 $a = 1

 memory check
 output /dev/null 
 for i 0 1000
   if ($a == 1)
    echo "run"    
   end
 end    
 output stdout
 memory check
end

# check memleaks
macro memtest2

 local a b i N

 $i = 0
 $a = 1
 $b = 2

 memory check
 output /dev/null
 for i 0 1000
   if (($a == 1) && ($b == 2))
    echo "run"    
   end
 end    
 output stdout
 memory check
end

# memory test using continue
macro memtest3

 local a b i N

 $i = 0
 $a = 1
 $b = 2

 memory check
 output /dev/null
 for i 0 1000
   if (($a == 1) && ($b == 2))
    continue
    echo "run"    
   end
 end    
 output stdout
 memory check
end

# memory test using continue
macro memtest4

 local a b i N bool


 $i = 0
 $a = 1
 $b = 2
 $bool = 0

 memory check
 output /dev/null
 for i 0 1000
   $bool = (($a == 1) && ($b == 2))
 end    
 output stdout
 memory check
end
