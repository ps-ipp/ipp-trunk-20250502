
list tests
 test1
 memtest1
end

# Test book commands
macro test1

 $PASS = 1

 local bname01 bname02 pcheck pname01 pname02 tword01 tword02 wcheck

 book create testb01
 book create testb02
 book getbook 0 -var bname01
 book getbook 1 -var bname02

 if (("$bname01" != "testb01") || ("$bname02" != "testb02"))
  $PASS = 0
  echo "Books not created/listed correctly!"
 end

 book newpage testb01 tpage01
 book newpage testb01 tpage02
 book npages testb01 -var pcheck
 
 if ($pcheck != 2)
  $PASS = 0
  echo "Book pages not added/recorded correctly!"
 end

 book getpage testb01 0 -var pname01
 book getpage testb01 1 -var pname02

 if (("$pname01" != "tpage01") || ("$pname02" != "tpage02"))
  $PASS = 0
  echo "Pages not created/listed correctly!"
 end

 book setword testb01 tpage01 w01 tword01
 book setword testb01 tpage01 w02 tword02
 book getword testb01 tpage01 w01 -var wcheck

 if ("$wcheck" != "tword01")
  $PASS = 0
  echo "Words not created/listed correctly!"
 end

 book delpage testb01 tpage02
 book getpage testb01 1 -var pcheck

 if ("$pcheck" != "NULL")
  $PASS = 0
  echo "Book pages not deleted correctly (by delpage)!"
 end

 book init testb01
 book getpage testb01 0 -var pcheck

 if ("$pcheck" != "NULL")
  $PASS = 0
  echo "Book pages not deleted correctly (by init)!"
 end

 book delete testb01
 book delete testb02

# delete bname01 bname02 pcheck pname01 pname02 tword01 tword02 wcheck

end


# Memory test
macro memtest1

 local i bcheck pcheck wcheck

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  book create testb
  book getbook 0 -var bcheck
  book newpage testb testp1
  book newpage testb testp2
  book npages testb -var pcheck
  book getpage testb 0 -var pcheck
  book setword testb testp1 testw tada
  book getword testb testp1 testw -var wcheck
  book delpage testb testp2
  book init testb
  book delete testb
 end
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end

#delete i bcheck pcheck wcheck

end
