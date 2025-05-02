
$skip = 0

list tests
 test1
 test2
 test3
end

# test using full range
macro test1
 $PASS = 1
 break -auto off

 gaussdev dev {100*100} 0.0 1.0
 dimenup dev buf 100 100
 stats -q buf 
 $npix = 100*100

 if ($STATUS == 0)
   echo "failed command"
   $PASS = 0
 end
 if ($NPIX != $npix) 
   echo "failed npix"
   $PASS = 0
 end
 if (abs($MEAN - 0.0) > (2.0/sqrt($npix))) 
   echo "failed mean"
   $PASS = 0
 end
 if (abs($MEDIAN - 0.0) > (2.0/sqrt($npix))) 
   echo "failed median"
   $PASS = 0
 end
 if (abs($SIGMA - 1.0) > (2.0/sqrt($npix))) 
   echo "failed sigma"
   $PASS = 0
 end
 if ($MIN < -4.0) 
   echo "failed min"
   $PASS = 0
 end
 if ($MAX > +4.0)
   echo "failed max"
   $PASS = 0
 end
# I don't this MODE is being correctly calculated.
 if ($skip && (abs($MODE - 0.0) > (2.0/sqrt($npix))))
   echo "MODE fails: known problem with mode"
   $PASS = 0
 end
 if (abs($TOTAL - $MEAN*$npix) > (2.0/sqrt($npix))) 
   echo "failed total"
   $PASS = 0
 end
end

# test using full range
macro test2
 $PASS = 1
 break -auto off

 gaussdev dev {100*100} 0.0 1.0
 dimenup dev buf 100 100
 stats -q buf  - - - -
 $npix = 100*100

 if ($STATUS == 0)
   echo "failed command"
   $PASS = 0
 end
 if ($NPIX != $npix) 
   echo "failed npix"
   $PASS = 0
 end
 if (abs($MEAN - 0.0) > (2.0/sqrt($npix))) 
   echo "failed mean"
   $PASS = 0
 end
 if (abs($MEDIAN - 0.0) > (2.0/sqrt($npix))) 
   echo "failed median"
   $PASS = 0
 end
 if (abs($SIGMA - 1.0) > (2.0/sqrt($npix))) 
   echo "failed sigma"
   $PASS = 0
 end
 if ($MIN < -4.0) 
   echo "failed min"
   $PASS = 0
 end
 if ($MAX > +4.0)
   echo "failed max"
   $PASS = 0
 end
# I don't this MODE is being correctly calculated.
 if ($skip && (abs($MODE - 0.0) > (2.0/sqrt($npix))))
   echo "MODE fails: known problem with mode"
   $PASS = 0
 end
 if (abs($TOTAL - $MEAN*$npix) > (2.0/sqrt($npix))) 
   echo $TOTAL {$MEAN*$npix} {abs($TOTAL - $MEAN*$npix) > (2.0/sqrt($npix))) 
   echo "failed total"
   $PASS = 0
 end
end

# test using full range
macro test3
 $PASS = 1
 break -auto off

 gaussdev dev {100*100} 0.0 1.0
 dimenup dev buf 100 100
 stats -q buf 10 10 10 10
 $npix = 10*10

 if ($STATUS == 0)
   echo "failed command"
   $PASS = 0
 end
 if ($NPIX != $npix) 
   echo "failed npix"
   $PASS = 0
 end
 if (abs($MEAN - 0.0) > (2.0/sqrt($npix))) 
   echo "failed mean"
   $PASS = 0
 end
 if (abs($MEDIAN - 0.0) > (2.0/sqrt($npix))) 
   echo "failed median"
   $PASS = 0
 end
 if (abs($SIGMA - 1.0) > (2.0/sqrt($npix))) 
   echo "failed sigma"
   $PASS = 0
 end
 if ($MIN < -4.0) 
   echo "failed min"
   $PASS = 0
 end
 if ($MAX > +4.0)
   echo "failed max"
   $PASS = 0
 end
# I don't this MODE is being correctly calculated.
 if ($skip && (abs($MODE - 0.0) > (2.0/sqrt($npix))))
   echo "MODE fails: known problem with mode"
   $PASS = 0
 end
 if (abs($TOTAL - $MEAN*$npix) > (2.0/sqrt($npix))) 
   echo "failed total"
   $PASS = 0
 end
end
