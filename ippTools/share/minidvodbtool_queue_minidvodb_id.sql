INSERT INTO minidvodbCopy
    SELECT
        0,              -- minidvodbcopy_id
        minidvodb_id,         -- minidvodb_id
        '%s',           -- minidvodb_rsync_path
        '%s',           -- destination_host
	%d,           -- fault
        '%s',           -- state
        %s,           -- epoch
        %f           -- dtime
        
    FROM minidvodbRun
    WHERE
        minidvodbRun.state = 'merged'
        AND minidvodbRun.minidvodb_id = %lld
