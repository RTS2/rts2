CREATE TABLE observatories (
	observatory_id	integer PRIMARY KEY,
	longitude	float8, -- geographical informations
	latitude	float8,
	altitude	float8,
	apiurl		VARCHAR(100)
);

CREATE TABLE targets_observatories (
	observatory_id	integer REFERENCES observatories(observatory_id),
	tar_id		integer REFERENCES targets (tar_id),
	obs_tar_id	integer NOT NULL
);

CREATE TABLE bb_schedules (
	schedule_id	integer PRIMARY KEY,
	tar_id		integer REFERENCES targets (tar_id)
);

CREATE TABLE observatory_schedules (
	schedule_id	integer REFERENCES schedules (schedule_id),
	observatory_id	integer REFERENCES observatories (observatory_id),
	state		integer NOT NULL
	created		timestamp NOT NULL,
	last_update	timestamp
);

CREATE TABLE observatory_observations (
	observatory_id	integer REFERENCES observatories (observatory_id),
	obs_id 		integer NOT NULL,
	tar_id		integer REFERENCES targets (tar_id),
	obs_ra		float8,
	obs_dec		float8,
	obs_slew	timestamp,
	obs_start	timestamp,
	obs_end		timestamp,
	onsky		float8, -- total second of on-sky images
	good_images	integer,
	bad_images	integer
);

CREATE SEQUENCE bb_schedule_id;
