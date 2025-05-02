SELECT
    magic_id,
    exp_id,
    camera,
    magicRun.workdir,
    uri
FROM magicRun
JOIN rawExp USING(exp_id)
JOIN magicTree USING(magic_id)
LEFT JOIN magicNodeResult USING(magic_id, node)
WHERE
    magicRun.state = 'new'
    AND magicNodeResult.node = 'root'
    AND magicNodeResult.fault = 0
GROUP BY
    magic_id
HAVING
    COUNT(magicTree.node) = COUNT(magicNodeResult.node)
