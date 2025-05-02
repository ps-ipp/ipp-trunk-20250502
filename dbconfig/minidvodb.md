minidvodbRun METADATA
    minidvodb_id    S64    0	     
    minidvodb_name  STR    64
    minidvodb_group STR    64
    minidvodb_path  STR    255
    minidvodb_host  STR    64
    state           STR    64
    note            STR    255
    creation_date   UTC    0001-01-01T00:00:00Z
END 

minidvodbProcessed METADATA
    minidvodb_id    S64    0
    dtime_resort    F32    0.0
    dtime_relphot   F32    0.0
    dtime_script    F32	   0.0
    epoch           UTC    0001-01-01T00:00:00Z
    fault	    S16	   0
END

minidvodbCopy METADATA
    minidvodbcopy_id	S64 0
    minidvodb_id    S64    0
    minidvodb_rsync_path   STR    255
    destination_host	   STR	  255
    fault           S16	   0
    state           STR    64
    epoch           UTC    0001-01-01T00:00:00Z
    dtime           F32    0.0
END
