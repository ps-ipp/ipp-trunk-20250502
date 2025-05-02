UPDATE minidvodbCopy
 --   SET minidvodbCopy.state = 'new',
    SET minidvodbCopy.fault = 0
WHERE
    minidvodbCopy.state = 'new'
    AND minidvodbCopy.fault >0    

