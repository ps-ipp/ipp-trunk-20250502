
list tests
 test1
 test2
end

# test that date does not return an error
macro test1
 $PASS = 1
 date -var var
 if ($STATUS == 0)
   $PASS = 0
 end
end

# test that date constructs a reasonable sample date
macro test2
 $PASS = 1
 date -var date1
 $date2 = `date`
 list w1 -split $date1
 list w2 -split $date2
 # check the first 3 entries (day, month, date)
 if ($w1:0 != $w2:0)
   $PASS = 0
 end
 if ($w1:1 != $w2:1)
   $PASS = 0
 end
 if ($w1:2 != $w2:2)
   $PASS = 0
 end
end
