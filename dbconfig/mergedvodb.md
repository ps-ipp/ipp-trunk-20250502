mergedvodbRun METADATA
    merge_id        S64    0
    minidvodb_id    S64    0	     
    mergedvodb	    STR    64
    mergedvodb_path STR    255
    state           STR    64
    creation_date   UTC    0001-01-01T00:00:00Z
END 

mergedvodbProcessed METADATA
    merge_id	    S64    0
    merge_order     S64    0
    dtime_verify    F32    0.0
    dtime_merge     F32    0.0
    dtime_script    F32	   0.0
    epoch           UTC    0001-01-01T00:00:00Z
    fault	    S16	   0
END

mergedvodbCopy METADATA
    mergevodbcopy_id	S64 0
    merge_id    S64    0
    mergedvodb_rsync_path   STR    255
    destination_host	   STR	  255
    fault           S16	   0
    state           STR    64
    epoch           UTC    0001-01-01T00:00:00Z
    dtime           F32    0.0
END
