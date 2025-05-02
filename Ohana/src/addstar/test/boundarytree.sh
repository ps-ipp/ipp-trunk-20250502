#!/usr/bin/env dvo

# make a RINGS tess with the full skycells
exec rm -r RINGS.V3
exec skycells -mode RINGS -scale 0.25 -nx 10 -ny 10 -fix-ns -D CATDIR RINGS.V3 -overlap 60 60 -skyparity

# make a RINGS tess with just projection cell
exec rm -r RINGS.V3.proj
exec skycells -mode RINGS -scale 2.50 -fix-ns -D CATDIR RINGS.V3.proj -overlap 60 60 -skyparity

# build a boundary tree from RINGS.V3.proj
exec findskycell -mktree RINGS.V3.tree.fits RINGS.V3.proj -nx 10 -ny 10 -scale 2.5

delete -q r d
concat 180.0 r; concat 0.1 d
concat 180.1 r; concat 0.2 d
concat 180.0 r; concat 0.2 d
concat 180.5 r; concat 0.5 d
write boundarytree.dat r d

# try some test points
exec findskycell -tree RINGS.V3.tree.fits boundarytree.dat

catdir RINGS.V3.proj
skyregion 0 360 -90 90
for i 0 r[]
  gimages -pix r[$i] d[$i]
end
