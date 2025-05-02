relphot -tgroup-fit-airmass -tgroups tgroups.dat -images g -v -region 9 11 19 21 -D CATDIR t0.catdir.v2 -D STAR_TOOFEW 1 -statmode WT_MEAN -cloud-limit 0.5 -update -nloop 12
-images g -v -region 9 11 19 21 -D CATDIR t1.catdir.v4 -D STAR_TOOFEW 1 -statmode MEAN -cloud-limit 0.5 -update -nloop 12

relphot -tgroup-fit-airmass -tgroups tgroups.dat -mosaic -imfreeze -images g -v -region 9 11 19 21 -D CATDIR t0.catdir.v2 -D STAR_TOOFEW 1 -statmode WT_MEAN -cloud-limit 0.5 -update -nloop 12
relphot -tgroup-fit-airmass -tgroups tgroups.dat -mosaic -images g -v -region 9 11 19 21 -D CATDIR t2.catdir.v0 -D STAR_TOOFEW 1 -statmode MEAN -cloud-limit 0.5 -update -nloop 12
relphot -ref-weight -mosaic -images g -v -region 9 11 19 21 -D CATDIR t2.catdir.v0 -D STAR_TOOFEW 1 -statmode MEAN -cloud-limit 0.5 -nloop 12
