select
	rel.exp_id AS exp_id,
	rel.chip_id AS chip_id,
	false AS private,
	false AS active,
	false AS pairwise
FROM
	(select exp_id,chip_id FROM ippRelease
	 JOIN relExp USING(rel_id)
	 JOIN rawExp USING(exp_id)
	 WHERE
	 @RELWHERE@
	) AS rel
LEFT JOIN
	(select MIN(lap_id) AS lap_id,
	 exp_id,chip_id FROM lapRun
	 JOIN lapExp USING(lap_id)
	 WHERE
	 @LAPWHERE@
	 GROUP BY exp_id,chip_id
	) AS lap
USING(exp_id)
WHERE lap_id IS NULL
