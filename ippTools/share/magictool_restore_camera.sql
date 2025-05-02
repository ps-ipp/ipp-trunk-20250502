UPDATE magicRun 
    JOIN magicDSRun USING(magic_id)
    JOIN camRun ON stage = 'camera' AND stage_id = camRun.cam_id
SET camRun.magicked = 0,
    magicDSRun.state = '@NEW_STATE@'
WHERE re_place
