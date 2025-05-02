# Tables for receiving files

receiveSource	METADATA
	source_id	S64	0	# Primary Key AUTO_INCREMENT
	source		STR	127	# Key
	product		STR	64	# Key
	workdir		STR	255
        state           STR     64
	comment		STR	255	# Key
	fileset_last	STR	128
        status_product  STR     64
        ds_dbname       STR     64
        ds_dbhost       STR     64
END

receiveFileset	METADATA
	fileset_id	S64	0	# Primary Key AUTO_INCREMENT
	source_id	S64	0	# Key fkey (source_id) ref receiveSource(source_id)
	fileset		STR	128
        state           STR     64
        dirinfo         STR     255
        dbinfo          STR     255
	fault		S16	0	# Key
END

receiveFile	METADATA
	file_id		S64	0	# Primary Key AUTO_INCREMENT
	fileset_id	S64	0	# Key fkey (fileset_id) ref receiveFileset(fileset_id)
	file		STR	128
        bytes           S64     0
        md5sum          STR     255
        file_type       STR     64
        component       STR     64
END

receiveResult	METADATA
	file_id		S64	0	# Primary Key fkey (file_id) ref receiveFile(file_id)
	dtime_copy	F32	0.0
	dtime_extract	F32	0.0
	fault		S16	0	# Key
END
