#!/bin/csh

rm -rf catdir.t0

rsync -auv catdir.example/ catdir.t0/
chmod +w -R catdir.t0

dvosplitsky -v catdir.t0   -region 235 255 -35 -25
echo dvosplit    -v catdir.t0 5 -region 235 255 -35 -25
