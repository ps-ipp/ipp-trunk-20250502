DELETE FROM pzDownloadImfile
WHERE
    fault != 0
    -- fault was logged in the last 3 days
    AND ABS(TIMESTAMPDIFF(SECOND, epoch, NOW())) < 86400 * 3
    AND (
        -- HTTP 503: try again
        fault = 203
        -- HTTP 500: timeout
        OR fault = 200
        -- HTTP 404: unknown datastore internal problem
        OR fault = 104
        -- ipptool errors
	OR fault = 115
	-- nebulous timout / ipptool errors
        OR fault < 100
        -- perl untrapped die()
        OR fault = 255
        -- HTTP 502 Proxy Error (seen now at ITC)
        OR fault = 202
    )
