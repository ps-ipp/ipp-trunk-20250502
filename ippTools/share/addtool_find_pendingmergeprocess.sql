SELECT minidvodbRun.* 
FROM minidvodbRun 
LEFT JOIN minidvodbProcessed
USING(minidvodb_id)
WHERE (minidvodbRun.state = 'to_be_merged' AND minidvodbProcessed.minidvodb_id IS NULL)
