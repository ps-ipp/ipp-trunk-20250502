SELECT warp_id
FROM warpRun join warpSkyfile using(warp_id, tess_id)
    JOIN fakeRun using(fake_id)
    JOIN camRun USING(cam_id)
    JOIN chipRun USING(chip_id)
    JOIN rawExp USING(exp_id)
    JOIN skycell ON skycell.tess_id = warpSkyfile.tess_id AND skycell.skycell_id = warpSkyfile.skycell_id
WHERE warpRun.tess_id = '%s' AND warpSkyfile.skycell_id = '%s' AND rawExp.filter = '%s'
    AND warpRun.state = 'full' AND warpSkyfile.quality = 0 and warpSkyfile.fault = 0
