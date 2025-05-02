NEB_CFLAGS=`pkg-config nebclient --cflags`
NEB_LIBS=`pkg-config nebclient --libs`

nebexample: nebexample.c
	$(CC) $(NEB_CFLAGS) $(CFLAGS) $(NEB_LIBS) $< -o $@
