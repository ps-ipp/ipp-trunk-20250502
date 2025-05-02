
macro test_00

  mcreate t0 30 50 -nz 70

  set t1 = xramp(t0)
  set t2 = yramp(t0)
  set t3 = zramp(t0)

  set r0 = sqrt((t1 - 15)^2 + (t2 - 25)^2 + (t3 - 35)^2)

  mslice r0 rz 35 -z
  mslice r0 ry 25 -y
  mslice r0 rx 15 -x

  mcreate Rz 30 50
  set Rz = sqrt((xramp(Rz) - 15)^2 + (yramp(Rz) - 25)^2 + (35 - 35)^2)
  set dRz = rz - Rz

  mcreate Ry 30 70
  set Ry = sqrt((xramp(Ry) - 15)^2 + (25 - 25)^2 + (yramp(Ry) - 35)^2)
  set dRy = ry - Ry

  mcreate Rx 50 70
  set Rx = sqrt((15 - 15)^2 + (xramp(Rx) - 25)^2 + (yramp(Rx) - 35)^2)
  set dRx = rx - Rx

  tv -ch 1 dRx -5 10
  tv -ch 2 dRy -5 10
  tv -ch 3 dRz -5 10

  mslice r0 rz 13 -z
  mslice r0 ry 43 -y
  mslice r0 rx 8 -x

  set Rz = sqrt((xramp(Rz) - 15)^2 + (yramp(Rz) - 25)^2 + (13 - 35)^2)
  set dRz = rz - Rz

  set Ry = sqrt((xramp(Ry) - 15)^2 + (43 - 25)^2 + (yramp(Ry) - 35)^2)
  set dRy = ry - Ry

  set Rx = sqrt((8 - 15)^2 + (xramp(Rx) - 25)^2 + (yramp(Rx) - 35)^2)
  set dRx = rx - Rx

  stat dRx
  stat dRy
  stat dRz
end

