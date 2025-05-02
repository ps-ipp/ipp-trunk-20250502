SELECT
    minidvodbProcessed.*,
    minidvodbRun.minidvodb_name,	
    minidvodbRun.minidvodb_group
FROM minidvodbProcessed
JOIN minidvodbRun
    USING(minidvodb_id)

