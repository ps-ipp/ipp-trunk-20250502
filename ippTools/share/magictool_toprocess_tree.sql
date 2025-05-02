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
FROM magicTree
JOIN magicRun USING(magic_id)
JOIN rawExp USING(exp_id)
LEFT JOIN magicNodeResult USING(magic_id, node)
LEFT JOIN Label ON magicRun.label = Label.label
WHERE
    magicRun.state = 'new'
    AND (Label.active OR Label.active IS NULL)
-- WHERE hook %s
ORDER BY
    priority, magicRun.magic_id
