UPDATE diffSkyfile
    JOIN diffRun USING(diff_id)
SET diffSkyfile.fault = 0
WHERE diffRun.state = 'update'
    AND diffSkyfile.data_state = 'update'
    AND diffSkyfile.fault != 0
