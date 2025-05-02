SELECT
    magicTree.*,
    magicRun.workdir,
    rawExp.exp_id,
    rawExp.exp_name,
    rawExp.workdir AS raw_workdir,
    rawExp.camera,
    -- convert magic_id into a boolean value (1 or 0)
    -- note that the type stays a 64 bit int
    magicNodeResult.magic_id IS TRUE AS done,
    magicNodeResult.fault IS TRUE AS bad,
    IFNULL(Label.priority, 10000) AS priority
FROM magicRun
JOIN magicTree USING(magic_id)
JOIN magicInputSkyfile USING(magic_id, node)
JOIN rawExp USING(exp_id)
JOIN diffSkyfile -- only get nodes that match a skycell
    ON diffSkyfile.diff_id = magicRun.diff_id
    AND diffSkyfile.skycell_id = magicInputSkyfile.node
JOIN diffRun
    ON diffRun.diff_id =  diffSkyfile.diff_id
LEFT JOIN magicNodeResult USING(magic_id, node)
LEFT JOIN Label ON magicRun.label = Label.label
WHERE
    magicRun.state = 'new'
    AND magicNodeResult.magic_id IS NULL
    AND magicNodeResult.node IS NULL
    AND diffRun.state = 'full'
    AND (Label.active OR Label.active IS NULL)
-- WHERE hook %s
