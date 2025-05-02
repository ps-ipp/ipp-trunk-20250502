
list tests
 test1
 test2
 test3
 test4
 test5
end

# a very simple diagonal matrix equation
macro test1
 $PASS = 1
 break -auto off

 mcreate A 3 3
 create B 0 3

 A[0][0] = 2
 A[1][1] = 2
 A[2][2] = 2

 A[0][1] = -1
 A[1][2] = -1
 A[1][0] = -1
 A[2][1] = -1

 gaussj A B
 if (not($STATUS))
   $PASS = 0
 end

 if (abs(A[0][0] - 0.75) > 0.01)
  $PASS = 0
 end
 if (abs(A[0][1] - 0.50) > 0.01)
  $PASS = 0
 end
 if (abs(A[0][2] - 0.25) > 0.01)
  $PASS = 0
 end

 if (abs(A[1][0] - 0.50) > 0.01)
  $PASS = 0
 end
 if (abs(A[1][1] - 1.00) > 0.01)
  $PASS = 0
 end
 if (abs(A[1][2] - 0.50) > 0.01)
  $PASS = 0
 end

 if (abs(A[2][0] - 0.25) > 0.01)
  $PASS = 0
 end
 if (abs(A[2][1] - 0.50) > 0.01)
  $PASS = 0
 end
 if (abs(A[2][2] - 0.75) > 0.01)
  $PASS = 0
 end

 if (abs(B[0] - 1.00) > 0.01)
  $PASS = 0
 end
 if (abs(B[1] - 2.00) > 0.01)
  $PASS = 0
 end
 if (abs(B[2] - 2.00) > 0.01)
  $PASS = 0
 end
end

# a very simple off-diagonal matrix equation
macro test2
 $PASS = 1
 break -auto off

 mcreate A 3 3
 create B 0 3

 A[0][1] = 2
 A[1][0] = 2
 A[2][2] = 2

 A[0][0] = -1
 A[1][2] = -1
 A[1][1] = -1
 A[2][0] = -1

 gaussj A B
 if (not($STATUS))
   $PASS = 0
 end

 # echo A[0][0] A[0][1] A[0][2]
 # echo A[1][0] A[1][1] A[1][2]
 # echo A[2][0] A[2][1] A[2][2]

 if (abs(A[1][0] - 0.75) > 0.01)
  $PASS = 0
 end
 if (abs(A[1][1] - 0.50) > 0.01)
  $PASS = 0
 end
 if (abs(A[1][2] - 0.25) > 0.01)
  $PASS = 0
 end

 if (abs(A[0][0] - 0.50) > 0.01)
  $PASS = 0
 end
 if (abs(A[0][1] - 1.00) > 0.01)
  $PASS = 0
 end
 if (abs(A[0][2] - 0.50) > 0.01)
  $PASS = 0
 end

 if (abs(A[2][0] - 0.25) > 0.01)
  $PASS = 0
 end
 if (abs(A[2][1] - 0.50) > 0.01)
  $PASS = 0
 end
 if (abs(A[2][2] - 0.75) > 0.01)
  $PASS = 0
 end

 if (abs(B[1] - 1.00) > 0.01)
  $PASS = 0
 end
 if (abs(B[0] - 2.00) > 0.01)
  $PASS = 0
 end
 if (abs(B[2] - 2.00) > 0.01)
  $PASS = 0
 end
end

# a singular matrix equation
macro test3
 $PASS = 1
 break -auto off

 mcreate A 3 3
 create B 0 3

 A[0][0] = 2
 A[1][0] = 2
 A[2][2] = 2

 A[0][1] = -1
 A[1][1] = -1
 A[2][1] = -1

 gaussj -q A B
 if ($STATUS)
   $PASS = 0
 end
end

# a very large matrix equation
macro test4
 $PASS = 1
 break -auto off

 $Ndim = 50
 mcreate A $Ndim $Ndim
 create B 0 $Ndim

 # generate the diagonal + off-diagonal elements
 for i 0 $Ndim
   A[$i][$i] = 2.0
   if ($i > 0)
     A[$i][$i-1] = -1.0
   end
   if ($i < $Ndim - 1)
     A[$i][$i+1] = -1.0
   end
 end

 set inB = B
 set inA = A

 gaussj A B
 if (not($STATUS))
   $PASS = 0
 end

 set meas = zero(inB)
 for i 0 B[]
  for j 0 B[]
   meas[$i] = meas[$i] + inA[$i][$j] * B[$j]
  end
 end

 for i 0 inB[]
  if (abs(inB[$i]-meas[$i]) > 1e-3)
    $PASS = 0
    echo inB[$i] meas[$i] {inB[$i]-meas[$i]}
  end
 end
end

# a nearly singular matrix equation
macro test5
 $PASS = 1
 break -auto off

 delete A B inA inB meas

 mcreate A 3 3
 create B 0 3

 A[0][0] = 2
 A[1][0] = 2.00001
 A[2][2] = 2

 A[0][1] = -1
 A[1][1] = -1
 A[2][1] = -1

 set inB = B
 set inA = A

 gaussj A B
 if (not($STATUS))
   $PASS = 0
 end

 set meas = zero(inB)
 for i 0 B[]
  for j 0 B[]
   # echo $i $j meas[$i] inA[$i][$j] B[$j] {inA[$i][$j] * B[$j]} {meas[$i] + inA[$i][$j] * B[$j]}
   meas[$i] = meas[$i] + inA[$i][$j] * B[$j]
   # echo "-> meas[$i]"
  end
 end

 for i 0 inB[]
  if (abs(inB[$i]-meas[$i]) > 1e-5)
    echo inB[$i] meas[$i] {abs(inB[$i]-meas[$i])}
    $PASS = 0
  end
 end
end

# a very large matrix equation
# timing on my laptop: 100 : ~1sec; 300 : ~7sec; 1000 : ~88sec
# note: at Ndim = 1000, it failed (Ax - B was in the range -2..+2)
macro test6
 if ($0 != 2)
  echo "USAGE: test6 (Ndim)"
  break
 end

 $PASS = 1
 break -auto off

 $Ndim = $1
 mcreate A $Ndim $Ndim
 create B 0 $Ndim

 # generate the diagonal + off-diagonal elements
 for i 0 $Ndim
   A[$i][$i] = 2.0
   if ($i > 0)
     A[$i][$i-1] = -1.0
   end
   if ($i < $Ndim - 1)
     A[$i][$i+1] = -1.0
   end
 end

 set inB = B
 set inA = A

 gaussj A B
 if (not($STATUS))
   $PASS = 0
 end

 set meas = zero(inB)
 for i 0 B[]
  for j 0 B[]
   meas[$i] = meas[$i] + inA[$i][$j] * B[$j]
  end
 end

 for i 0 inB[]
  if (abs(inB[$i]-meas[$i]) > 1e-3)
    $PASS = 0
    echo inB[$i] meas[$i] {inB[$i]-meas[$i]}
  end
 end
end

