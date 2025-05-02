#!/bin/csh -f

set region = "-region 248 251 -2 1.2"
set cat2M = catdir.2mass
set catPS = catdir.PS1
set raw2M = /data/ipp022.0/alala.0/ipp/ippRefs/catdir.2mass

# create the database
foreach file (smallset/*.smf)
  addstar -D PHOTCODE_FILE dvo.photcodes.3pi -D CAMERA gpc1 -D CATDIR $catPS $file -update
end
addstar -D CAMERA gpc1 -D CATDIR $catPS -resort

# NOTE: if the photcode table does not match the desire, do this:
# dvosecfilt 8 catdir.small.merge.v3
# photcode-table -import dvo.photcodes.3pi -D CATDIR catdir.small.merge.v3

# make a local 2MASS database chunk
dvosplit $raw2M 4 -outdir $cat2M -set-mode split -set-format PS1_REF $region
addstar -resort -D CATDIR $cat2M $region
# cp $raw2M/Images.dat catdir.2mass.d4/

# dvomerge A and B to C is broken, so copy A (serial and parallel)
rsync -auv $catPS/ $catPS.ser/
rsync -auv $catPS/ $catPS.par/

# merge 2MASS and PS1 (serial and parallel)
dvomerge $cat2M into $catPS.ser $region
dvomerge $cat2M into $catPS.par $region

# ensure g,r,i,z,y,J,H,K ave values are set
relphot -averages -update -D CATDIR $catPS.ser
relphot -averages -update -D CATDIR $catPS.par

# now parallelize the db
sed s/CATDIR/$catPS.par/ < HostTable.dat > $catPS.par/HostTable.dat
dvodist -out $catPS.par

relastro -parallel -D CATDIR $catPS.par -high-speed r J 20.0 catdir.highspeed.par $region -D WHERE_A "(r:nphot > 3) && (r:err > 0.05)" -D WHERE_B "(J:err > 0.05)"
relastro           -D CATDIR $catPS.ser -high-speed r J 20.0 catdir.highspeed.ser $region -D WHERE_A "(r:nphot > 3) && (r:err > 0.05)" -D WHERE_B "(J:err > 0.05)"

relphot -reset -averages -update -D CATDIR catdir.highspeed.ser
relphot -reset -averages -update -D CATDIR catdir.highspeed.par

# test relastro runs
if (0) then
  relastro           -D CATDIR $catPS.ser $region -update-chips   -update
  relastro           -D CATDIR $catPS.ser $region -update-objects -update
  relastro           -D CATDIR $catPS.ser $region -update-objects -update -pm

  relastro -parallel -D CATDIR $catPS.par $region -update-chips   -update
  relastro -parallel -D CATDIR $catPS.par $region -update-objects -update
  relastro -parallel -D CATDIR $catPS.par $region -update-objects -update -pm
endif

