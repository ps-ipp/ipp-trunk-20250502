
macro go
  dot 0.2 0.2 -sz 4 -pt ocir -c red; textline 0.2 0.2 "test" -justify 0
  dot 0.5 0.2 -sz 4 -pt ocir -c red; textline 0.5 0.2 "test" -justify 1
  dot 0.8 0.2 -sz 4 -pt ocir -c red; textline 0.8 0.2 "test" -justify 2
  dot 0.2 0.5 -sz 4 -pt ocir -c red; textline 0.2 0.5 "test" -justify 3
  dot 0.5 0.5 -sz 4 -pt ocir -c red; textline 0.5 0.5 "test" -justify 4
  dot 0.8 0.5 -sz 4 -pt ocir -c red; textline 0.8 0.5 "test" -justify 5
  dot 0.2 0.8 -sz 4 -pt ocir -c red; textline 0.2 0.8 "test" -justify 6
  dot 0.5 0.8 -sz 4 -pt ocir -c red; textline 0.5 0.8 "test" -justify 7
  dot 0.8 0.8 -sz 4 -pt ocir -c red; textline 0.8 0.8 "test" -justify 8

  png -name test.png
  ps -name test.ps
end
