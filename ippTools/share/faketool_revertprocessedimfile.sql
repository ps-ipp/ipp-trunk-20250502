DELETE FROM fakeProcessedImfile
USING fakeProcessedImfile, fakeRun, camRun, chipRun, rawExp
WHERE
    fakeProcessedImfile.fake_id = fakeRun.fake_id
    AND fakeRun.cam_id = camRun.cam_id
    AND camRun.chip_id = chipRun.chip_id
    AND chipRun.exp_id = rawExp.exp_id
    AND fakeProcessedImfile.fault != 0
