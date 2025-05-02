
list tests
 test1
 test2
 testmem1
 testmem2
 testmem3
 testmem4
end

# test subtraction
macro test1

  $PASS = 1
  local i

  exec rm -f tmp.txt
  output tmp.txt
  echo {99 - 98}
  output stdout

  $i = `cat tmp.txt` 
 
 if ($i != 1)
   $PASS = 0
   echo "i : $i"
 end

end

# test addition, division, and multiplication
macro test2

 $PASS = 1
 local a b c

 $a = {2 + 4}
 $b = {12 / 2}
 $c = {2 * 3}

 if (($a != 6) || ($b != 6) || ($c != 6))
   $PASS = 0
   echo "ALL VALUES NOT 6: a=$a\ b=$b\ c=$c\"
 end

end

# check memleaks (set global)
macro testmem1

 $PASS = 1
 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   echo {99 - 98}
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (set global)
macro testmem2

 $PASS = 1
 local i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   $N = 99 - 98
   $N = 1 + 1
   $N = 2 * 2
   $N = 22/7
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (set global)
macro testmem3

 $PASS = 1
 local i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   $N = 99
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (set global)
macro testmem4

 $PASS = 1
 local i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   $N = word
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end
