SELECT  userName, 
        domainName, 
        pstampUser.accessLevel,
        IFNULL(pstampUserDomain.accessLevel, 0) AS domainAccessLevel,
        pstampUser.defaultProduct as userProduct,
        pstampUser.defaultLabel as userLabel,
        pstampUserDomain.defaultProduct as domainProduct,
        pstampUserDomain.defaultLabel as domainLabel
FROM    pstampUser 
        LEFT JOIN pstampUserDomain USING(domainName)
