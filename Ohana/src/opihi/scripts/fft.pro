
macro fft.subraster

  delete B

  $Nx = $1
  $Ny = $2
  $DX = 4096/$Nx
  $DY = 4096/$Nx
  date
  for ix 0 $Nx
    for iy 0 $Ny
      subraster b B {$DX*$ix} {$DY*$iy} $DX $DY 0 0 $DX $DY
      fft2d B 0 to Br Bi
    end
  end
  date
end

macro fft.timing
  fft.subraster 1 1
  fft.subraster 2 2
  fft.subraster 4 4
  fft.subraster 8 8
end
