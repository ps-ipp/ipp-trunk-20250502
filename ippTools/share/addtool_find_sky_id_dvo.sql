SELECT staticskyRun.* FROM staticskyRun
JOIN staticskyResult USING(sky_id)
join staticskyInput using(sky_id)
JOIN stackRun USING(stack_id)

WHERE staticskyRun.state = 'full' and staticskyResult.quality = 0
    AND stack_id NOT IN (SELECT stack_id
       FROM addRun
       JOIN staticskyResult on staticskyResult.sky_id = addRun.stage_id
       JOIN staticskyRun USING(sky_id)
       JOIN staticskyInput USING(sky_id)
       WHERE addRun.stage = 'staticsky' AND %s
      )
