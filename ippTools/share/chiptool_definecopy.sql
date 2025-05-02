SELECT
    rawExp.*,
    newExp.label,
    chipOld.workdir AS old_workdir,
    chipOld.data_group AS old_data_group,
    chipOld.state AS old_state
FROM chipRun AS chipOld
JOIN rawExp USING(exp_id)
JOIN newExp using (exp_id)
LEFT JOIN chipRun AS chipNew
    ON chipNew.exp_id = chipOld.exp_id
    AND chipNew.label = '%s'
WHERE chipNew.chip_id IS NULL
    AND rawExp.fault = 0
