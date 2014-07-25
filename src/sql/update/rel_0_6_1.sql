-- ra, dec, lng, lat, JD
CREATE OR REPLACE FUNCTION ln_airmass (float8, float8, float8, float8, float8)
  RETURNS float4 AS 'pg_astrolib.so', 'ln_airmass' LANGUAGE 'c';

ALTER TABLE scripts ADD CONSTRAINT scripts_uniq_cam_tar UNIQUE (tar_id, camera_name);
