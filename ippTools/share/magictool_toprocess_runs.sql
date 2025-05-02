SELECT DISTINCT
    magic_id,
    IFNULL(priority, 10000) AS priority
FROM magicTree
JOIN magicRun USING(magic_id)
LEFT JOIN magicNodeResult USING(magic_id, node)
LEFT JOIN Label ON magicRun.label = Label.label
WHERE
    magicRun.state = 'new'
AND magicNodeResult.magic_id IS NULL
-- WHERE hook %s
ORDER BY
    priority DESC, magicRun.magic_id
