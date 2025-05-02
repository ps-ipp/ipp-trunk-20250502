
macro test1

  for i 0 100
    create x 0 50
    set y = x^2
    spline create t$i x y
    spline rename t$i test
    spline delete test
  end
end
