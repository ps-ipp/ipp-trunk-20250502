DELETE FROM minidvodbProcessed
USING minidvodbProcessed, minidvodbRun, addRun
WHERE
    minidvodbProcessed.minidvodb_id = minidvodbRun.minidvodb_id
    AND addRun.minidvodb_name = minidvodbRun.minidvodb_name
    AND minidvodbProcessed.fault != 0
    AND minidvodbRun.state = 'merged'
