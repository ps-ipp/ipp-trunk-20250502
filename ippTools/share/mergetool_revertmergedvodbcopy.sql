UPDATE mergedvodbCopy
 --   SET minidvodbCopy.state = 'new',
    SET mergedvodbCopy.fault = 0
WHERE
    mergedvodbCopy.state = 'new'
    AND mergedvodbCopy.fault >0    

