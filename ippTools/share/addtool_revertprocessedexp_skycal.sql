DELETE FROM addProcessedExp
USING addProcessedExp, addRun
WHERE
    addRun.add_id = addProcessedExp.add_id
    AND addProcessedExp.fault != 0
    AND addRun.state = 'new'
    AND addRun.stage = 'skycal'
