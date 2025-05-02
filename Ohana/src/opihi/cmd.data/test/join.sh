
list tests
 test1
 memtest1
end

# Test if imhist works
macro test1

 $PASS = 1

 local i

 delete ID1 ID2

 vlist -int ID1 1   2   3   4   5   6
 vlist val1     0.1 4.3 2.1 5.5 2.1 4.2

 vlist -int ID2 5   3   1   6
 vlist val      0.3 2.3 1.1 2.5

 join ID1 ID2 

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  imhist -q buff xvec yvec -region 40 0 25 10 -range 0 10
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
