SELECT
    magicInputSkyfile.*,
    magicRun.inverse,
    diffSkyfile.path_base
FROM magicRun
JOIN magicInputSkyfile USING(magic_id)
JOIN diffSkyfile
    ON diffSkyfile.diff_id = magicRun.diff_id
    AND diffSkyfile.skycell_id = magicInputSkyfile.node
WHERE
    magicRun.state = 'new'
