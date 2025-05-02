SELECT minidvodbRun.* FROM minidvodbRun
JOIN minidvodbProcessed USING(minidvodb_id)
WHERE minidvodbRun.state = 'merged' and minidvodbProcessed.fault = 0
    AND minidvodb_id NOT IN (SELECT minidvodb_id
       FROM mergedvodbRun
       JOIN minidvodbRun USING(minidvodb_id)
       WHERE %s
      )
