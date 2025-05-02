SELECT mergedvodbRun.* 
FROM mergedvodbRun 
LEFT JOIN mergedvodbProcessed
USING(merge_id)
WHERE (mergedvodbRun.state = 'new' AND mergedvodbProcessed.merge_id IS NULL)
