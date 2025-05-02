
macro checker
  mcreate a 8 8
  for ix 0 8
    for iy 0 8
      if (($ix + $iy) % 2) continue
      a[$ix][$iy] = $ix + $iy
    end
  end
end
