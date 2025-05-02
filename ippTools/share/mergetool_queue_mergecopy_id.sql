INSERT INTO mergedvodbCopy
    SELECT
        0,              -- mergedvodbcopy_id
        merge_id,         -- merge_id
        '%s',           -- mergedvodb_rsync_path
        '%s',           -- destination_host
	%d,           -- fault
        '%s',           -- state
        %s,           -- epoch
        %f           -- dtime
        
    FROM mergedvodbRun
    WHERE
        (mergedvodbRun.state = 'merged'
 	OR mergedvodbRun.state = 'full')
        AND mergedvodbRun.merge_id = %lld
