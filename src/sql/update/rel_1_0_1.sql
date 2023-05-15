-- Take care of lost support for long-deprecated properties - 'abstime' type and 'opaque' type in function's definition
CREATE OR REPLACE FUNCTION wcs_in (cstring)
  RETURNS wcs AS 'pg_wcs.so','wcs_in' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION wcs_out (wcs)
  RETURNS cstring AS 'pg_wcs.so', 'wcs_out' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION wcs2_in (cstring)
  RETURNS wcs2 AS 'pg_wcs2.so','wcs2_in' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION wcs2_out (wcs2)
  RETURNS cstring AS 'pg_wcs2.so', 'wcs2_out' LANGUAGE 'c';


-- we have to drop this view temporarily, just to be able to alter the "counts" table
DROP VIEW targets_counts;

ALTER TABLE counts ALTER COLUMN count_date TYPE timestamp with time zone;

CREATE VIEW targets_counts AS
SELECT
        targets.tar_id,
        counts.*
FROM
        targets, counts, observations
WHERE
        targets.tar_id = observations.tar_id
    AND observations.obs_id = counts.obs_id
ORDER BY
        count_date DESC;


ALTER TABLE airmass_cal_images ALTER COLUMN air_last_image TYPE timestamp with time zone;
ALTER TABLE airmass_cal_images ALTER COLUMN air_last_image SET default to_timestamp (0);


-- and also, remove this obviously long-forgotten relic
DROP VIEW images_path;

