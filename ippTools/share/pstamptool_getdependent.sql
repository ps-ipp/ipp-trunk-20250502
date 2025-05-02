SELECT DISTINCT pstampDependent.*
FROM pstampDependent
WHERE (pstampDependent.state = 'new' OR pstampDependent.state = 'hold')
