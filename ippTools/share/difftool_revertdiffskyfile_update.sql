UPDATE diffRun
JOIN diffSkyfile USING(diff_id)
SET diffRun.state = 'new'
WHERE
    diffSkyfile.fault != 0
