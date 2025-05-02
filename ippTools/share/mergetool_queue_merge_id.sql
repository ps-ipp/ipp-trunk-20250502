INSERT INTO mergedvodbRun
    SELECT
        0,              -- merge_id
	minidvodb_id,   -- minidvodb_id
        '%s',           -- mergedvodb
        '%s',           -- mergedvodb_path
	'%s',           -- state
    FROM minidvodbRun join minidvodbProcessed using (minidvodb_id)
    WHERE
        minidvodbRun.state = 'merged' and minidvodbProcessed.fault = 0
        AND minidvodbRun.minidvodb_id = %lld
