-- Exposures that have diffs
-- Temporary table has useful indices that make future query faster!
CREATE TEMPORARY TABLE diffs (
    diff_id BIGINT,
    exp_id BIGINT,
    PRIMARY KEY(diff_id, exp_id),
    KEY(exp_id)
) ENGINE=MEMORY;
