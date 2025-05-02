-- postage stamp requests ready for download and parsing
SELECT
    pstampRequest.*,
    pstampDataStore.outProduct AS ds_outProduct,
    IFNULL(pstampDataStore.need_magic, 1) AS need_magic,
    IFNULL(Label.priority, 10000) AS priority
FROM pstampRequest
    LEFT JOIN pstampDataStore USING(ds_id)
    LEFT JOIN Label ON pstampRequest.label = Label.label
WHERE pstampRequest.state = 'new'
    AND pstampRequest.fault = 0
    AND (Label.active OR Label.active IS NULL)
