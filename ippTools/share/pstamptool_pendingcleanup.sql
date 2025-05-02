-- postage stamp requests pending cleanup
SELECT
    pstampRequest.*,
    IFNULL(Label.priority, 10000) AS priority
FROM pstampRequest
    LEFT JOIN Label ON pstampRequest.label = Label.label
WHERE pstampRequest.state = 'goto_cleaned'
    AND pstampRequest.fault = 0
    AND (Label.active OR Label.active IS NULL)
