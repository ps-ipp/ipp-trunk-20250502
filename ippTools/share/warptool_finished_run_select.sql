SELECT
   warp_id,
   magicked
FROM
   (SELECT DISTINCT
       warpRun.warp_id,
       warpRun.label as label,
       warpSkyCellMap.warp_id as foo,
       warpSkyfile.warp_id as bar,
       warpSkyfile.magicked as magicked
   FROM warpRun
   JOIN warpSkyCellMap
       USING(warp_id)
   LEFT JOIN warpSkyfile
       USING(warp_id, skycell_id)
   WHERE
       warpRun.state = 'new'
   GROUP BY
       warpRun.warp_id
   HAVING
       COUNT(warpSkyCellMap.warp_id) = COUNT(warpSkyfile.warp_id)
       AND SUM(warpSkyfile.fault > 0) = 0
 ) as Foo
