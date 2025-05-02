SELECT
    receiveFile.file_id,
    source,
    product,
    workdir,
    dirinfo,
    fileset,
    fileset_id,
    file,
    bytes,
    md5sum,
    file_type,
    component
FROM receiveFile
JOIN receiveFileset USING(fileset_id)
JOIN receiveSource USING(source_id)
LEFT JOIN receiveResult
    ON receiveResult.file_id = receiveFile.file_id
WHERE
    receiveSource.state = 'enabled'
    AND ((receiveFileset.state = 'new')
    OR (receiveFileset.state = 'listed' AND receiveFile.component = 'dirinfo' AND receiveFileset.dirinfo IS NULL))
    AND receiveFileset.fault = 0
    AND receiveResult.file_id IS NULL
