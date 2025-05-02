DELETE FROM mergedvodbProcessed
USING mergedvodbProcessed, mergedvodbRun
WHERE
    mergedvodbProcessed.merge_id = mergedvodbRun.merge_id
    AND mergedvodbProcessed.fault != 0
    AND mergedvodbRun.state = 'new'
