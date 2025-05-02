
clean:
	rm -rf catdir.output.copy
	rm -rf catdir.output.clip
	rm -rf catdir.output.clip2

empty: clean
	rm -rf catdir.input
	rm -rf catdir.in2

init:
	rm -rf catdir.input
	addstar -D CATDIR catdir.input -cat usno -p USNO_RED -region 0 1 0 1
	addstar -D CATDIR catdir.in2 -cat usno -p USNO_RED -region 0 1 0 1
	addstar -D CATDIR catdir.in2 -cat usno -p USNO_BLUE -region 0 1 0 1

copy:
	photdbc -D CATDIR catdir.input catdir.output.copy -region 0 1 0 1

clip:
	photdbc -D CATDIR catdir.input catdir.output.clip -region 0 1 0 1 -maxminmag 14.0

clip2:
	photdbc -D CATDIR catdir.in2 catdir.output.clip2 -region 0 1 0 1 -maxminmag 14.0
