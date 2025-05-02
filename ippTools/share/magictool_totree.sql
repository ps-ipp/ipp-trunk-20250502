SELECT
    magic_id,
    exp_id,
    camera,
    magicRun.workdir,
    ra,
    decl,
    diffRun.tess_id,
    IFNULL(priority, 10000) AS priority
FROM magicRun
JOIN diffRun USING(diff_id)
JOIN rawExp USING(exp_id)
LEFT JOIN magicTree
    USING(magic_id)
LEFT JOIN Label ON magicRun.label = Label.label
WHERE
    magicRun.state = 'new'
    AND magicTree.node IS NULL
    AND magicRun.fault = 0
    AND (Label.active OR Label.active IS NULL)
