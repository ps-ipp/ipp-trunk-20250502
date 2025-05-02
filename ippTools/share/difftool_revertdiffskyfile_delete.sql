DELETE FROM diffSkyfile
USING diffSkyfile, diffRun
WHERE
    diffRun.diff_id = diffSkyfile.diff_id
    AND diffRun.state = 'new'
    AND fault != 0
