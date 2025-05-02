DELETE magicMask
FROM magicRun
    JOIN magicNodeResult USING(magic_id)
    JOIN magicMask USING(magic_id)
WHERE
    magicRun.state = 'new'
    AND magicNodeResult.node = 'root'
    AND magicNodeResult.fault != 0
    AND magicRun.state = 'new'
