-- this selects the minidvodbs that have complete addRuns
SELECT minidvodb_id, minidvodb_name, minidvodb_path, state, minidbg 
AS minidvodb_group, (cnt - cnt2) as not_full, cnt as addRun_count
FROM minidvodbRun 
JOIN 
   (SELECT minidvodb_name, count(state) as cnt2, cnt, minidbg, minidbn 
   FROM addRun LEFT JOIN 
       -- this select grabs (and counts) all the addRuns with any state
       -- and groups them by the name
       (SELECT minidvodb_group as minidbg, minidvodb_name as minidbn, count(state) 
        AS cnt 
        FROM addRun where minidvodb_group = '%s'
        GROUP BY minidvodb_name ) 
   AS foo1 ON minidvodb_name = minidbn 
        -- the second select grabs and counts all the addRuns with state
        -- of 'full'         
   WHERE addRun.state = 'full' and minidvodb_group = '%s'
   GROUP BY minidvodb_name ) 
AS foo2 
USING(minidvodb_name) 
