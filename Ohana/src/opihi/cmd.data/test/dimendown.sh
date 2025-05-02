
list tests
 test1
 memtest1
end

# Test if dimendown works
macro test1

 $PASS = 1

 mcreate timg 100 10
 zap timg 0 0 100 10 -v 10
 dimendown timg val
 dimendown timg xc -x
 dimendown timg yc -y

 if (val[777] != 10)
  $PASS = 0
  echo "Value mismatch: val[777] (should be 10)"
 end

 if (xc[777] != 77)
  $PASS = 0
  echo "X Coord mismatch: xc[777] (should be 77)"
 end

 if (yc[777] != 7)
  $PASS = 0
  echo "Y Coord mismatch: yc[77] (should be 7)"
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
  dimendown timg val
  dimendown timg xc -x
  dimendown timg yc -y
 end
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 128)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end
end
