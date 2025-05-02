SELECT
    mergedvodbRun.*,
    minidvodbRun.minidvodb_path
FROM minidvodbRun 
JOIN mergedvodbRun 
    USING(minidvodb_id)
LEFT JOIN mergedvodbProcessed
    USING(merge_id)
WHERE
  (mergedvodbRun.state = 'new' AND mergedvodbProcessed.merge_id IS NULL)
    AND mergedvodbRun.mergedvodb IS NOT NULL
    AND mergedvodbRun.mergedvodb_path IS NOT NULL
  

