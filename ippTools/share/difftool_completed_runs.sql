SELECT
    COUNT(diffInputSkyfile.skycell_id),
    COUNT(diffSkyfile.skycell_id),
    diffSkyfile.*,
    SUM(!diffSkyfile.magicked) = 0 as all_magicked
FROM diffRun
JOIN diffInputSkyfile USING(diff_id)
LEFT JOIN diffSkyfile USING(diff_id, skycell_id)
WHERE
    diffRun.state = 'new'
-- WHERE hook %s
GROUP BY
    diffInputSkyfile.diff_id
HAVING
    COUNT(diffInputSkyfile.skycell_id) = COUNT(diffSkyfile.skycell_id)
    AND SUM(diffSkyfile.fault) = 0
