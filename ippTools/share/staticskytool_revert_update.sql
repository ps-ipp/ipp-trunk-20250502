UPDATE staticskyRun
    JOIN staticskyResult USING(sky_id) JOIN staticskyInput USING(sky_id)
    JOIN stackRun  USING(stack_id) JOIN skycell USING(tess_id, skycell_id)
SET staticskyResult.fault = 0
WHERE staticskyRun.sky_id = staticskyResult.sky_id
    AND staticskyRun.sky_id = staticskyInput.sky_id
    AND staticskyInput.stack_id = stackRun.stack_id
    AND stackRun.tess_id = skycell.tess_id
    AND stackRun.skycell_id = skycell.skycell_id
    AND staticskyRun.state = 'update'
    AND staticskyResult.fault != 0
