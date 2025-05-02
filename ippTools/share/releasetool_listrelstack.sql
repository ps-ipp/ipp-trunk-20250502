SELECT
    release_name,
    surveyName,
    relStack.*,
    stackSumSkyfile.path_base AS stack_path_base,
    stackSumSkyfile.quality,
    stackRun.state AS stack_state,
    stackRun.data_group AS stack_data_group,
    skycalResult.fwhm_major,
    skycalResult.quality as skycal_quality,
    skycalResult.path_base AS skycal_path_base,
    skycalRun.data_group AS skycal_data_group,
    staticskyResult.sky_id AS sky_id,
    staticskyResult.path_base AS staticsky_path_base,
    'GPC1' AS camera
FROM relStack
JOIN ippRelease USING(rel_id) 
JOIN survey USING(surveyID)
JOIN stackRun USING(stack_id, filter, tess_id, skycell_id)
JOIN stackSumSkyfile USING(stack_id)
LEFT JOIN skycalRun USING(skycal_id)
LEFT JOIN skycalResult USING(skycal_id)
LEFT JOIN staticskyResult USING(sky_id)
