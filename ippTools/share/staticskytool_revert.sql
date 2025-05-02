DELETE FROM staticskyResult
USING staticskyResult, staticskyRun, staticskyInput, stackRun, skycell
WHERE staticskyRun.sky_id = staticskyResult.sky_id
    AND staticskyRun.sky_id = staticskyInput.sky_id
    AND staticskyInput.stack_id = stackRun.stack_id
    AND stackRun.tess_id = skycell.tess_id
    AND stackRun.skycell_id = skycell.skycell_id
    AND staticskyRun.state = 'new'
    AND staticskyResult.fault != 0
