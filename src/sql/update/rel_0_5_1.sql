-- add errors to images table..
ALTER TABLE images ADD COLUMN img_err_ra	float8;
ALTER TABLE images ADD COLUMN img_err_dec	float8;
ALTER TABLE images ADD COLUMN img_err		float8;
