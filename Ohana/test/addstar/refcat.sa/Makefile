
REG = -region 0 1 0 1

usno: usno.photcodes usno.formats usno.depths
usno.depths:  usno.depths.raw  usno.depths.mef  usno.depths.split
usno.formats: usno.formats.raw usno.formats.mef usno.formats.split

gsc:
	addstar -v -cat GSC $(REG) -D CATDIR catdir.gsc
	dvocheck catdir.gsc gsc

2mass:
	addstar -cat 2MASS $(REG) -D CATDIR catdir.2mass -photcode 2MASS_J
	addstar -cat 2MASS $(REG) -D CATDIR catdir.2mass -photcode 2MASS_H
	addstar -cat 2MASS $(REG) -D CATDIR catdir.2mass -photcode 2MASS_K
	dvocheck catdir.2mass 2mass

tycho:
	addstar -cat TYCHO $(REG) -D CATDIR catdir.tycho -photcode TYCHO_B
	addstar -cat TYCHO $(REG) -D CATDIR catdir.tycho -photcode TYCHO_V
	dvocheck catdir.tycho tycho

usnob:
	addstar -cat USNOB $(REG) -D CATDIR catdir.usnob -photcode USNO_RED
	addstar -cat USNOB $(REG) -D CATDIR catdir.usnob -photcode USNO_BLUE
	dvocheck catdir.usnob usnob

usno.photcodes:
	addstar -cat USNO $(REG) -D CATDIR catdir.usno -photcode USNO_RED
	addstar -cat USNO $(REG) -D CATDIR catdir.usno -photcode USNO_BLUE
	dvocheck catdir.usno usno

usno.formats.raw:
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.raw.loneos    -photcode USNO_BLUE -D CATFORMAT LONEOS
	dvocheck catdir.usno.raw.loneos usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.raw.elixir    -photcode USNO_BLUE -D CATFORMAT ELIXIR
	dvocheck catdir.usno.raw.elixir usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.raw.pmtest    -photcode USNO_BLUE -D CATFORMAT PMTEST
	dvocheck catdir.usno.raw.pmtest usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.raw.panstarrs -photcode USNO_BLUE -D CATFORMAT PANSTARRS
	dvocheck catdir.usno.raw.panstarrs usno

usno.formats.mef:
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.mef.loneos    -photcode USNO_BLUE -D CATMODE MEF -D CATFORMAT LONEOS 
	dvocheck catdir.usno.mef.loneos usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.mef.elixir    -photcode USNO_BLUE -D CATMODE MEF -D CATFORMAT ELIXIR
	dvocheck catdir.usno.mef.elixir usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.mef.pmtest    -photcode USNO_BLUE -D CATMODE MEF -D CATFORMAT PMTEST
	dvocheck catdir.usno.mef.pmtest usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.mef.panstarrs -photcode USNO_BLUE -D CATMODE MEF -D CATFORMAT PANSTARRS
	dvocheck catdir.usno.mef.panstarrs usno

usno.formats.split:
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.split.loneos    -photcode USNO_BLUE -D CATMODE SPLIT -D CATFORMAT LONEOS
	dvocheck catdir.usno.split.loneos usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.split.elixir    -photcode USNO_BLUE -D CATMODE SPLIT -D CATFORMAT ELIXIR
	dvocheck catdir.usno.split.elixir usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.split.pmtest    -photcode USNO_BLUE -D CATMODE SPLIT -D CATFORMAT PMTEST
	dvocheck catdir.usno.split.pmtest usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.split.panstarrs -photcode USNO_BLUE -D CATMODE SPLIT -D CATFORMAT PANSTARRS
	dvocheck catdir.usno.split.panstarrs usno

usno.depths.raw:
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.raw.L2 -photcode USNO_BLUE -D SKY_DEPTH 2 -D CATMODE RAW
	dvocheck catdir.usno.raw.L2 usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.raw.L3 -photcode USNO_BLUE -D SKY_DEPTH 3 -D CATMODE RAW
	dvocheck catdir.usno.raw.L3 usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.raw.L4 -photcode USNO_BLUE -D SKY_DEPTH 4 -D CATMODE RAW
	dvocheck catdir.usno.raw.L4 usno

usno.depths.mef:
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.mef.L2 -photcode USNO_BLUE -D SKY_DEPTH 2 -D CATMODE MEF
	dvocheck catdir.usno.mef.L2 usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.mef.L3 -photcode USNO_BLUE -D SKY_DEPTH 3 -D CATMODE MEF
	dvocheck catdir.usno.mef.L3 usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.mef.L4 -photcode USNO_BLUE -D SKY_DEPTH 4 -D CATMODE MEF
	dvocheck catdir.usno.mef.L4 usno

usno.depths.split:
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.split.L2 -photcode USNO_BLUE -D SKY_DEPTH 2 -D CATMODE SPLIT
	dvocheck catdir.usno.split.L2 usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.split.L3 -photcode USNO_BLUE -D SKY_DEPTH 3 -D CATMODE SPLIT
	dvocheck catdir.usno.split.L3 usno
	addstar -cat USNO $(REG) -D CATDIR catdir.usno.split.L4 -photcode USNO_BLUE -D SKY_DEPTH 4 -D CATMODE SPLIT
	dvocheck catdir.usno.split.L4 usno


#####

clean:
	rm -rf catdir.*
