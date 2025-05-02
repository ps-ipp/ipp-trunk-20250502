UPDATE summitExp SET fault = 0
WHERE
    fault != 0
    -- fault is older than 10 minutes
    AND ABS(TIMESTAMPDIFF(SECOND, epoch, NOW())) > 60 * 10 
    -- fault was logged in the last 3 days
    AND ABS(TIMESTAMPDIFF(SECOND, epoch, NOW())) < 86400 * 3
    AND (
        -- HTTP 503: internal server error
        fault = 203
        -- HTTP 500: timeout
        OR fault = 200
        -- HTTP 404: unknown datastore internal problem
        OR fault = 104
        -- ipptool errors
        OR fault < 100
        -- perl untrapped die()
        OR fault = 255
        -- HTTP 502 Proxy Error (seen now at ITC)
        OR fault = 202
    )
