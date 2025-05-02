SELECT DISTINCT
        warpRun.label,
	Label.label AS label_label,
        IFNULL(Label.priority, 10000) AS priority,
	count(warpRun.warp_id)
    FROM warpRun
    LEFT JOIN Label ON warpRun.label = Label.label
    WHERE
        (warpRun.state = 'new' OR warpRun.state = 'update')
        AND (Label.active OR Label.active IS NULL)
        -- where hook %s
	group by warpRun.label
        ORDER BY priority DESC, warp_id
