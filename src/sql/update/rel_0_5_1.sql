-- add errors to images table..
ALTER TABLE images ADD COLUMN img_err_ra	float8;
ALTER TABLE images ADD COLUMN img_err_dec	float8;
ALTER TABLE images ADD COLUMN img_err		float8;

ALTER TABLE images ADD CONSTRAINT images_obs_img UNIQUE (obs_id, img_id);

-- add email for originators/interested parties
ALTER TABLE users ADD COLUMN usr_email VARCHAR(200);
ALTER TABLE users ADD COLUMN usr_id integer;

ALTER TABLE ONLY users ADD CONSTRAINT users_pkey PRIMARY KEY (usr_id);

CREATE TABLE targets_users (
	tar_id		integer REFERENCES targets(tar_id),
	usr_id		integer REFERENCES users(usr_id),
	event_mask	integer DEFAULT 1
);

-- add table for "airmass selector"
CREATE TABLE airmass_cal_images (
	air_airmass_start  float NOT NULL,
	air_airmass_end    float NOT NULL,
	air_last_image     timestamp NOT NULL default abstime (0),
	-- referenced image - can be null
	obs_id             integer,
	img_id             integer,
FOREIGN KEY (obs_id, img_id) REFERENCES images (obs_id, img_id)
);

CREATE OR REPLACE FUNCTION img_wcs_naxis1 (wcs)
  RETURNS int AS 'pg_wcs.so', 'img_wcs_naxis1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_naxis2 (wcs)
  RETURNS int AS 'pg_wcs.so', 'img_wcs_naxis2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_ctype1 (wcs)
  RETURNS varchar AS 'pg_wcs.so', 'img_wcs_ctype1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_ctype2 (wcs)
  RETURNS varchar AS 'pg_wcs.so', 'img_wcs_ctype2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_crpix1 (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_crpix1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_crpix2 (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_crpix2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_crval1 (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_crval1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_crval2 (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_crval2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_cdelt1 (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_cdelt1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_cdelt2 (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_cdelt2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_crota (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_crota' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_epoch (wcs)
  RETURNS float8 AS 'pg_wcs.so', 'img_wcs_epoch' LANGUAGE 'c';
