INSERT INTO fakeRun
    SElECT
        0,              -- fake_id
        cam_id,         -- cam_id
        '%s',           -- state
        '%s',           -- workdir
        '%s',           -- label
        '%s',           -- data_group
        '%s',           -- dist_group
        '%s',           -- reduction
        '%s',           -- expgroup
        '%s',           -- dvodb 
        '%s',           -- tess_id
        '%s',           -- end_stage
        NULL,           -- epoch
        '%s'            -- note
    FROM camRun
    WHERE
        camRun.state = 'full'
        AND camRun.cam_id = %lld
