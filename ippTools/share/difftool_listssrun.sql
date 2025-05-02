SELECT DISTINCT
    diffRun.diff_id,
    diffRun.state,
    diffRun.workdir,
    diffRun.label,
    diffRun.data_group,
    diffRun.dist_group,
    diffRun.reduction,
    diffRun.note,
    diffRun.tess_id
FROM diffRun
JOIN diffInputSkyfile USING(diff_id)
JOIN stackRun AS stackInputRun
    ON stackInputRun.stack_id = diffInputSkyfile.stack1
JOIN stackSumSkyfile AS stackInput
    ON stackInputRun.stack_id = stackInput.stack_id
JOIN stackRun AS stackTemplateRun
    ON stackTemplateRun.stack_id = diffInputSkyfile.stack2
JOIN stackSumSkyfile AS stackTemplate
    ON stackTemplateRun.stack_id = stackTemplate.stack_id
WHERE diff_mode = 4
