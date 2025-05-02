SELECT
    fakeProcessedImfile.*,
    fakeRun.state,
    fakeRun.workdir,
    fakeRun.label,
    fakeRun.reduction,
    fakeRun.expgroup,
    fakeRun.dvodb,
    fakeRun.tess_id,
    fakeRun.end_stage
FROM fakeRun
JOIN fakeProcessedImfile
    USING(fake_id)
WHERE
    ((fakeRun.state = 'goto_cleaned' AND fakeProcessedImfile.data_state = 'full')
OR 
    (fakeRun.state = 'goto_scrubbed' AND fakeProcessedImfile.data_state = 'full')
OR 
    (fakeRun.state = 'goto_purged' AND fakeProcessedImfile.data_state != 'purged'))
