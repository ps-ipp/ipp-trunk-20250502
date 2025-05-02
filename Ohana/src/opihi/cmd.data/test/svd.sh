
list tests
 test1
 test2
end

# a very simple diagonal matrix equation
macro test1
 $PASS = 1
 break -auto off

 delete -q A U w Vt W A1

 mcreate A 2 2

 A[0][0] = 1.0
 A[1][0] = 0.5

 A[0][1] = 0.5
 A[1][1] = 2.0

 # svd give A = U w Vt, but this function returns V, not Vt:
 svd A = U w V
 if (not($STATUS))
   $PASS = 0
 end

 # can we recreate A?
 mcreate W w[] w[]
 for i 0 w[]
   W[$i][$i] = w[$i]
 end

 matrix transpose V to Vt
 matrix set t1 = W * Vt
 matrix set A1 = U * t1
 set dA = A1 - A

 stats -q dA
 if ($MIN < -0.0001)
   $PASS = 0
 end
 if ($MAX >  0.0001)
   $PASS = 0
 end
end

# a very simple diagonal matrix equation
macro test2
 $PASS = 1
 break -auto off

 delete -q A U w Vt W A1

 mcreate A 2 3

 A[0][0] = 1.0
 A[1][0] = 0.0

 A[0][1] = 0.0
 A[1][1] = 2.0

 A[0][2] = 0.0
 A[1][2] = 1.0

 # svd give A = U w Vt, but this function returns V, not Vt:
 svd A = U w V
 if (not($STATUS))
   $PASS = 0
 end

 # can we recreate A?
 mcreate W w[] w[]
 for i 0 w[]
   W[$i][$i] = w[$i]
 end

 matrix transpose V to Vt
 matrix set t1 = W * Vt
 matrix set A1 = U * t1
 set dA = A1 - A

 stats -q dA
 if ($MIN < -0.0001)
   $PASS = 0
 end
 if ($MAX >  0.0001)
   $PASS = 0
 end
end

