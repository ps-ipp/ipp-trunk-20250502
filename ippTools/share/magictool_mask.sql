SELECT
    magicMask.*,
    magicRun.exp_id
FROM magicMask
JOIN magicRun
    USING(magic_id)
WHERE
    magicRun.state = 'full'
    AND magicMask.fault = 0
