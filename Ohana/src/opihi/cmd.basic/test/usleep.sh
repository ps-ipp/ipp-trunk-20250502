list tests
 test1
end

# Does usleep work?
macro test1
 $PASS = 1

 list tstart -x "date +%S"

 usleep 3000000

 list tend -x "date +%S"

 if ($tstart:0 >= 57)
  $tend:0 = $tend:0 + 60
 end

 $diff = abs (3 - abs($tstart:0 - $tend:0))

 if ($diff > 1.1)
  $PASS = 0
 end

end
