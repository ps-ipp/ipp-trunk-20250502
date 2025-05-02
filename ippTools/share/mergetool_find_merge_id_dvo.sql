SELECT mergedvodbRun.* FROM mergedvodbRun
JOIN mergedvodbProcessed USING(merge_id)
WHERE ( mergedvodbRun.state = 'merged' OR mergedvodbRun.state = 'full')
      AND mergedvodbProcessed.fault = 0
    AND merge_id NOT IN (SELECT merge_id
       FROM mergedvodbCopy
       JOIN mergedvodbRun USING(mergedvodb_id)
       WHERE %s
      )
