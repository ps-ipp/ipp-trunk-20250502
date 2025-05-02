DELETE magicNodeResult 
FROM magicNodeResult 
JOIN magicRun USING(magic_id) 
WHERE magicNodeResult.fault != 0
    AND magicRun.state = 'new'
