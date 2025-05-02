

list tests
 test1
 memtest1
end

# Test if uniqpair works
macro test1

 $PASS = 1

 local i

 delete ID1 ID2

 vlist -int ID1 1   1   2   3   3   4   5   6
 vlist -int ID2 2   3   4   5   5   6   7   8

 uniqpair ID1 ID2 IDu -c IDn -d index

 # if ((xvec[1024] != 17) || (yvec[1024] != 100))
 #  $PASS = 0
 #  echo "Value mismatch: xvec[1024] yvec[1024] (should be 17,100)"
 # end
 # 
 # imhist -q buff xvec yvec -region 40 0 25 10 -range 0 10
 # 
 # if ((xvec[1024] != 10) || (yvec[1024] != 100))
 #  $PASS = 0
 #  echo "Value mismatch: xvec[1024] yvec[1024] (should be 10,100)"
 # end

end

# Test if uniqpair works
macro test2

 $PASS = 1

 delete ID1 ID2

 vlist -int ID1 {2^16 + 2} {2^17}     {2^18 + 3} {2^16 + 2} {2^17 + 2} {2^18 + 3} 
 vlist -int ID2 {2^16 + 0} {2^17 + 2} {2^18 + 5} {2^16 + 0} {2^17 + 2} {2^18 + 5} 

 uniqpair ID1 ID2 IDu -c IDn -d index

 reindex ID2s = ID2 using index
 reindex ID1s = ID1 using index

 vectors

 echo "unique"
 print_v IDu IDn

 echo "dups"
 print_v ID1s ID2s index
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
