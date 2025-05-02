UPDATE magicRun 
    JOIN magicDSRun USING(magic_id)
    JOIN diffRun ON stage = 'diff' AND stage_id = diffRun.diff_id
    JOIN diffSkyfile ON diffRun.diff_id = diffSkyfile.diff_id
SET diffRun.magicked = 0,
    diffSkyfile.magicked = 0,
    magicDSRun.state = '@NEW_STATE@'
WHERE re_place
