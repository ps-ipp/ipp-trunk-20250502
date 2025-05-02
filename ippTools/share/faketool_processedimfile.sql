SELECT DISTINCT
    fakeProcessedImfile.*,
    fakeRun.workdir,
    rawExp.exp_tag,
    rawExp.exp_name,
    rawExp.camera,
    rawExp.telescope,
    rawExp.filelevel
FROM fakeRun
JOIN fakeProcessedImfile
    USING(fake_id)
JOIN rawExp
    ON fakeProcessedImfile.exp_id = rawExp.exp_id
WHERE
-- bogus test; just here so there there is a 'WHERE' stmt to append
-- conditionals too
    fakeProcessedImfile.exp_id is NOT NULL

