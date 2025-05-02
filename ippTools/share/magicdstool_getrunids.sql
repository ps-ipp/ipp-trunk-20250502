-- find the inputs for a new magicDSRun from an existing magicRun
SELECT DISTINCT
    magic_id,
    magicRun.exp_id,
    chip_id,
    cam_id,
    warp_id,
    diffSkyfile.diff_id
FROM magicRun 
JOIN magicInputSkyfile USING(magic_id)
JOIN diffSkyfile 
    ON magicRun.diff_id = diffSkyfile.diff_id
    AND magicInputSkyfile.node = diffSkyfile.skycell_id
JOIN diffInputSkyfile
    ON diffInputSkyfile.diff_id = diffSkyfile.diff_id
    AND diffInputSkyfile.skycell_id = diffSkyfile.skycell_id
JOIN warpRun
    ON (diffInputSkyfile.warp1 = warp_id AND !magicRun.inverse)
    OR (diffInputSkyfile.warp2 = warp_id AND magicRun.inverse)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
WHERE magic_id = %ld
