SELECT stackRun.* FROM stackSumSkyfile
JOIN stackRun USING(stack_id)
WHERE stackRun.state = 'full' and stackSumSkyfile.quality = 0
    AND stack_id NOT IN (SELECT stack_id
       FROM addRun
       JOIN stackInputSkyfile on stackInputSkyfile.stack_id = addRun.stage_id
       JOIN stackRun USING(stack_id)
       WHERE addRun.stage = 'stack' AND %s
      )
