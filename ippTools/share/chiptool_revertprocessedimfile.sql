DELETE FROM chipProcessedImfile
USING chipProcessedImfile, chipRun, rawExp
WHERE
    chipRun.chip_id = chipProcessedImfile.chip_id
    AND rawExp.exp_id = chipProcessedImfile.exp_id
    AND chipRun.state = 'new'
    AND chipProcessedImfile.fault != 0
