SELECT *
FROM (
    SELECT * FROM 
    (SELECT
        summitImfile.*,
        summitExp.dateobs
    FROM summitImfile
    JOIN pzDownloadExp
    	USING(summit_id)
    LEFT JOIN pzDownloadImfile
        USING(summit_id, class, class_id)
    JOIN summitExp
    	USING(summit_id)
    WHERE 
        pzDownloadExp.state = 'run'
	AND pzDownloadImfile.summit_id IS NULL
        AND pzDownloadImfile.exp_name IS NULL
        AND pzDownloadImfile.camera IS NULL
        AND pzDownloadImfile.telescope IS NULL
        AND pzDownloadImfile.class IS NULL
        AND pzDownloadImfile.class_id IS NULL
	AND summitImfile.exp_name NOT LIKE 'c%'
    ORDER BY dateobs) AS partA
UNION
    SELECT * FROM 
    (SELECT
        summitImfile.*,
        summitExp.dateobs
    FROM summitImfile
    JOIN pzDownloadExp
    	USING(summit_id)
    LEFT JOIN pzDownloadImfile
        USING(summit_id, class, class_id)
    JOIN summitExp
    	USING(summit_id)
    WHERE 
        pzDownloadExp.state = 'run'
	AND pzDownloadImfile.summit_id IS NULL
        AND pzDownloadImfile.exp_name IS NULL
        AND pzDownloadImfile.camera IS NULL
        AND pzDownloadImfile.telescope IS NULL
        AND pzDownloadImfile.class IS NULL
        AND pzDownloadImfile.class_id IS NULL
	AND summitImfile.exp_name LIKE 'c%'
    ORDER BY dateobs) AS partB
) as Foo
