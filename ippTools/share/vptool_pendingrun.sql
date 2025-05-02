SELECT vp_id,
       rawExp.camera,
       CONCAT_WS('.', exp_name, exp_id) AS exp_tag,
       vpRun.label,
       vpRun.workdir,
       vpRun.dest_id,
       rcDestination.name AS dest_name,
       rcDestination.dbname AS ds_dbname,
       rcDestination.dbhost AS ds_dbhost,
       IFNULL(Label.priority, 10000) as priority
FROM vpRun
    JOIN rawExp USING(exp_id)
    LEFT JOIN rcDestination USING(dest_id)
    LEFT JOIN Label ON vpRun.label = Label.label
WHERE vpRun.state = 'new'
    AND vpRun.fault = 0
    AND (Label.active OR Label.active IS NULL)
