SELECT skycalRun.* FROM skycalRun
JOIN skycalResult USING (skycal_id)
JOIN staticskyInput USING (sky_id,stack_id)
JOIN stackRun USING(stack_id)

WHERE skycalRun.state = 'full' and skycalResult.quality = 0
    AND stack_id NOT IN (SELECT stack_id
       FROM addRun
       JOIN skycalResult on skycalResult.skycal_id = addRun.stage_id
       JOIN skycalRun USING(skycal_id)
       WHERE addRun.stage = 'skycal' AND %s
      )
