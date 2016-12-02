CREATE TABLE flats ( 
        obs_id		integer REFERENCES observations(obs_id),
	img_id		integer NOT NULL,
	flat_date	timestamp,
	flat_date_usec  integer,
	flat_exposure	integer,
	flat_temperature integer,
	epoch_id	integer NOT NULL REFERENCES epoch(epoch_id),
	camera_name	varchar(8) REFERENCES cameras(camera_name),
CONSTRAINT flats_prim_key PRIMARY KEY (obs_id, img_id)
);

-- image quaility checks
ALTER TABLE images ADD COLUMN img_fwhm float4;
ALTER TABLE images ADD COLUMN img_limmag float4;
ALTER TABLE images ADD COLUMN img_qmagmax float4;

ALTER TABLE targets ADD COLUMN tar_next_observable timestamp with time zone;

-- relation for follow-up calibration observations
CREATE TABLE calibration_observation (
	cal_obs_id	integer NOT NULL REFERENCES observations (obs_id),
	obs_id		integer NOT NULL REFERENCES observations (obs_id),
CONSTRAINT calibration_obs_prim_key PRIMARY KEY (cal_obs_id, obs_id)
);

CREATE TABLE integral_observation (
        integral_id        integer NOT NULL REFERENCES integral (integral_id),
	obs_id		integer NOT NULL REFERENCES observations (obs_id),
CONSTRAINT integral_obs_prim_key PRIMARY KEY (integral_id, obs_id)
);
