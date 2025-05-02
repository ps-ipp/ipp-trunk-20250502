
list tests
 test1
 memtest1
end

# Test if imsmooth works
macro test1

 $PASS = 1

 local i j

 mcreate buff 100 100

 for i 0 99
  for j 0 99
   zap buff $i $j 1 1 -v {100*rnd($i)}
  end
 end

 stats -q buff
 $s1 = $SIGMA

 imsmooth buff 10
 stats -q buff
 $s2 = $SIGMA

 if ($s2/$s1 >= 0.1)
  $PASS = 0
  echo "Inadequate noise reduction: {$s2/$s1*100}\% (should be less than 10%)"
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 100
  imsmooth buff 10
 end

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/100}"
 end
end
