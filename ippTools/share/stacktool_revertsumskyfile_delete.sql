DELETE FROM stackSumSkyfile
USING stackSumSkyfile, stackRun
WHERE stackSumSkyfile.stack_id = stackRun.stack_id
    AND stackRun.state = 'new'
    AND fault != 0
