DELETE FROM relExp USING relExp, ippRelease, rawExp, camRun
WHERE relExp.rel_id = ippRelease.rel_id
    AND relExp.exp_id = rawExp.exp_id
    AND relExp.cam_id = camRun.cam_id
