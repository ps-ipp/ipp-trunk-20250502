DROP TABLE IF EXISTS diffs;

-- Create a temporary table with useful indices
CREATE TEMPORARY TABLE diffs (
    diff_id BIGINT,
    exp_id BIGINT,
    PRIMARY KEY(diff_id, exp_id),
    KEY(exp_id)
) ENGINE=MEMORY;


-- Get list of exposures that have diffs
INSERT INTO diffs
SELECT DISTINCT
    diffWarps.diff_id,
    exp_id
FROM (
    -- Forward diffs
    SELECT
        diffRun.diff_id,
        warp1 AS warp_id
    FROM diffRun
    JOIN diffInputSkyfile USING(diff_id)
    WHERE warp1 IS NOT NULL
    -- WHERE hook %s
        AND diffRun.label = 'ThreePi.Run2.r.v0'
    UNION
    -- Backward diffs
    SELECT
        diffRun.diff_id,
        warp2 AS warp_id
    FROM diffRun
    JOIN diffInputSkyfile
        ON diffInputSkyfile.diff_id = diffRun.diff_id
        AND diffRun.bothways = 1
    WHERE warp2 IS NOT NULL
    -- WHERE hook %s
        AND diffRun.label = 'ThreePi.Run2.r.v0'
    ) AS diffWarps
JOIN warpRun USING(warp_id)
JOIN fakeRun USING(fake_id)
JOIN camRun USING(cam_id)
JOIN chipRun USING(chip_id)
;



-- EXPLAIN
SELECT
    inputWarpRun.warp_id AS input_warp_id,
    inputWarpRun.tess_id AS tess_id,
    inputRawExp.exp_id AS input_exp_id,
    SUBSTRING_INDEX(GROUP_CONCAT(templateWarpRun.warp_id ORDER BY ABS(ASIN(SQRT(POW(SIN(0.5*(inputRawExp.decl - templateRawExp.decl)),2) + COS(inputRawExp.decl) * COS(templateRawExp.decl) * POW(SIN(0.5*(inputRawExp.ra - templateRawExp.ra)),2))))), ',', 1) AS template_warp_id
FROM warpRun AS inputWarpRun
JOIN fakeRun AS inputFakeRun USING(fake_id)
JOIN camRun AS inputCamRun USING(cam_id)
JOIN chipRun AS inputChipRun USING(chip_id)
JOIN rawExp AS inputRawExp USING(exp_id)
-- To find exposures that haven't been diffed, insert newline here:%s
 LEFT JOIN diffs USING(exp_id)
JOIN warpRun AS templateWarpRun
    ON templateWarpRun.warp_id != inputWarpRun.warp_id -- Don't use self as template!
    AND templateWarpRun.tess_id = inputWarpRun.tess_id -- Ensure using same tessellation
JOIN fakeRun AS templateFakeRun
    ON templateFakeRun.fake_id = templateWarpRun.fake_id
JOIN camRun AS templateCamRun
    ON templateCamRun.cam_id = templateFakeRun.cam_id
JOIN chipRun AS templateChipRun
    ON templateChipRun.chip_id = templateCamRun.chip_id
JOIN rawExp AS templateRawExp
    ON templateRawExp.exp_id = templateChipRun.exp_id
    AND templateRawExp.filter = inputRawExp.filter
-- WHERE hook %s
WHERE (inputWarpRun.label = 'ThreePi.Run2.r.v0')
    AND ((DEGREES(2*ASIN(SQRT(POW(SIN(inputRawExp.decl - templateRawExp.decl),2) + COS(inputRawExp.decl)*COS(templateRawExp.decl)*POW(SIN(inputRawExp.ra - templateRawExp.ra),2)))) < 0.10000000 + 0.00000119))
    AND ((TIME_TO_SEC(TIMEDIFF(inputRawExp.dateobs, templateRawExp.dateobs)) > 0.00000000 - 0.00000119))
    AND (templateWarpRun.label = 'ThreePi.Run2.r.v0')
--    AND (diffs.diff_id IS NULL)
    AND inputWarpRun.state = 'full'
-- WHERE done
GROUP BY input_warp_id
;
