-- add errors to images table..
ALTER TABLE images ADD COLUMN img_err_ra	float8;
ALTER TABLE images ADD COLUMN img_err_dec	float8;
ALTER TABLE images ADD COLUMN img_err		float8;

ALTER TABLE images ADD CONSTRAINT images_obs_img UNIQUE (obs_id, img_id);

-- add table for "airmass selector"
CREATE TABLE airmass_cal_images (
	air_airmass_start  float NOT NULL,
	air_airmass_end    float NOT NULL,
	air_last_image     timestamp DEFAULT null,
	-- referenced image - can be null
	obs_id             integer,
	img_id             integer,
FOREIGN KEY (obs_id, img_id) REFERENCES images (obs_id, img_id)
);
