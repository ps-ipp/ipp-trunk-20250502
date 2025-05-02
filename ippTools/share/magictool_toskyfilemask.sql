SELECT
    magicMask.*
FROM magicRun
JOIN magicMask
    USING(magic_id)
LEFT JOIN magicSkyfileMask
    USING(magic_id)
WHERE
    magicRun.state = 'new'
    AND magicSkyfileMask.magic_id IS NULL
