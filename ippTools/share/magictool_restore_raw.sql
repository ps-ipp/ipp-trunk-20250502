UPDATE magicRun
    JOIN magicDSRun USING(magic_id)
    JOIN rawExp ON stage = 'raw' AND stage_id = rawExp.exp_id
    JOIN rawImfile ON rawExp.exp_id = rawImfile.exp_id
SET rawImfile.magicked = 0, 
    rawExp.magicked = 0, 
    magicDSRun.state = '@NEW_STATE@'
WHERE re_place
