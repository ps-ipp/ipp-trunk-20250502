UPDATE magicRun
    JOIN magicDSRun USING(magic_id)
    JOIN chipRun ON stage = 'chip' AND stage_id = chip_id
    JOIN chipProcessedImfile USING(chip_id)
SET chipProcessedImfile.magicked = 0, 
    chipRun.magicked = 0, 
    magicDSRun.state = '@NEW_STATE@'
WHERE re_place
