SELECT DISTINCT
       summitExp.exp_name,
       rawImfile.uri,
       summitExp.dateobs AS registered,
       summitExp.imfiles,
       summitExp.fault AS summit_fault,
       pzDownloadExp.state AS download_state,
       newExp.state AS newExp_state,
       rawExp.state AS rawExp_state,
       rawImfile.data_state AS imfile_state,
       newExp.exp_id,
       summitExp.exp_type,
       rawImfile.dateobs,
       rawImfile.obs_mode AS obs_mode,
       rawImfile.object AS object,
       rawImfile.burntool_state AS burntool_state,
       rawImfile.class_id,
       summitImfile.class_id AS summit_class_id,
       (pzDownloadExp.state = 'stop') AS is_downloaded,
       (rawImfile.burntool_state IS NOT NULL) AS is_registered
       FROM
       summitExp
       JOIN summitImfile USING(summit_id)
       LEFT JOIN pzDownloadExp USING(summit_id)
       LEFT JOIN newExp USING(summit_id)
       LEFT JOIN newImfile ON (summitImfile.class_id = newImfile.tmp_class_id AND newExp.exp_id = newImfile.exp_id)
       LEFT JOIN rawExp ON rawExp.exp_id = newExp.exp_id
       LEFT JOIN rawImfile ON (rawImfile.exp_id = newExp.exp_id AND rawImfile.tmp_class_id = newImfile.tmp_class_id)
       WHERE
             summitExp.dateobs >= '@DATEOBS_BEGIN@'
	     AND summitExp.dateobs <= '@DATEOBS_END@'
	     AND summitImfile.class_id = '@CLASS_ID@'
	     AND (summitExp.exp_name <= '@EXP_NAME@')                                                                     
ORDER BY summitExp.dateobs