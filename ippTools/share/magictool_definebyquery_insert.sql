-- Insert the best list of diffs as magic inputs
INSERT INTO magicInputSkyfile
SELECT
    @MAGIC_ID@, -- Update this with the appropriate magic_id
    skycell_id
FROM diffSkyfile
WHERE
    diff_id = @DIFF_ID@ -- Update this with the appropriate diff_id
    AND fault = 0
    AND quality = 0
