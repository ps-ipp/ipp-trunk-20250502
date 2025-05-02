SELECT
    stackRun.stack_id,
    stackRun.tess_id,
    stackRun.skycell_id,
    stackRun.workdir,
    stackRun.reduction,
    stackRun.label,
    stackRun.state,
    stackSumSkyfile.path_base,
    IFNULL(Label.priority, 10000) AS priority
FROM stackRun
JOIN stackInputSkyfile USING(stack_id)
JOIN warpRun USING(warp_id)
LEFT JOIN stackSumSkyfile USING(stack_id)
LEFT JOIN Label ON Label.label = stackRun.label
WHERE
    ((stackRun.state = 'new' AND stackSumSkyfile.stack_id IS NULL)
    OR (stackRun.state = 'update' AND stackSumSkyfile.fault = 0 AND stackSumSkyfile.quality = 0))
    AND (Label.active OR Label.active IS NULL)
    -- WHERE hook %s
GROUP BY stack_id
HAVING SUM(IF(warpRun.state = 'full', 1, 0)) = COUNT(stackInputSkyfile.warp_id)
