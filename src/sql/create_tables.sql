DROP TABLE terestial;
DROP TABLE ell;
DROP TABLE ot;
DROP TABLE grb;
DROP TABLE targets;
DROP TABLE epoch;
DROP TABLE types;

CREATE GROUP "observers";

CREATE TABLE epoch (
	epoch_id	integer PRIMARY KEY,		
	epoch_start	timestamp,
	epoch_end	timestamp
);

CREATE TABLE types (
	type_id		char PRIMARY KEY,
	type_description varchar(200)
);

CREATE TABLE targets (
	tar_id		integer PRIMARY KEY,
	type_id		char REFERENCES types(type_id),
	tar_name	varchar(150),
	tar_ra		float8,
	tar_dec		float8,
	tar_comment	text,
	tar_enabled     boolean,
	-- priority bonus for target
	tar_priority    integer,
	-- here start site dependent part - that depends highly on
	-- local object visibility
	tar_bonus       integer,
	-- how long will bonus last
	tar_bonus_time  timestamp
);

CREATE TABLE phot (
	tar_id		integer REFERENCES targets (tar_id),
	phot_type	integer,
	phot_epoch	integer,
	phot_mag1	float4,
	phot_mag2	float4,
	phot_index1	float4,
	phot_index2	float4,
	phot_sao_num	int8
);

CREATE TABLE grb (
	tar_id		integer REFERENCES targets (tar_id),
	grb_id		integer PRIMARY KEY,
	grb_seqn	integer,
	grb_date	timestamp,
	grb_last_update	timestamp
);

CREATE TABLE ot (
	tar_id		integer REFERENCES targets (tar_id),
	ot_imgcount	integer,
	ot_minpause	interval DEFAULT NULL,
	ot_priority	integer
);

--- elliptical target elements; if ell_a < 0, then it's parabollic target
CREATE TABLE ell (
        tar_id          integer REFERENCES targets (tar_id),
        ell_a           float8,
        ell_e           float8,
        ell_i           float8,
        ell_w           float8,
        ell_omega       float8,
        ell_n           float8,
        ell_minpause    interval DEfAULT NULL,
        ell_priority    integer,
        ell_JD          float8,
        ell_mag_1       float8,
        ell_mag_2       float8
);

CREATE TABLE terestial (
	tar_id		integer REFERENCES targets (tar_id),
	ter_minutes	integer, -- observation every ter_minutes
	ter_offset	integer -- observation offset in minutes
);

DROP TABLE cameras;

CREATE TABLE cameras (
	camera_name	varchar(8) PRIMARY KEY,
	camera_description	varchar(100)
);

DROP TABLE mounts;

CREATE TABLE mounts (
	mount_name	varchar(8) PRIMARY KEY,
	mount_long	float8,
	mount_lat	float8,
	mount_alt	float8,
	mount_desc	varchar(100)
);

DROP TABLE counters;

CREATE TABLE counters ( 
	counter_name	varchar(8) PRIMARY KEY,
	mount_name	varchar(8) REFERENCES mounts(mount_name)
);

DROP TABLE darks;

CREATE TABLE darks (
	dark_name	varchar(250) NOT NULL,
	dark_date	timestamp,
	dark_exposure	integer,
	dark_temperature integer,
	epoch_id	integer NOT NULL REFERENCES epoch(epoch_id),
	camera_name	varchar(8) REFERENCES cameras(camera_name)
);

DROP TABLE flats;

CREATE TABLE flats ( 
	flat_name	varchar(250) NOT NULL,
	flat_date	timestamp,
	flat_exposure	integer,
	flat_temperature integer,
	epoch_id	integer NOT NULL REFERENCES epoch(epoch_id),
	camera_name	varchar(8) REFERENCES cameras(camera_name)
);

DROP SEQUENCE tar_id;

CREATE SEQUENCE tar_id;

DROP TABLE observations;

CREATE TABLE observations (
	tar_id		integer REFERENCES targets (tar_id),
	obs_id		integer PRIMARY KEY NOT NULL,
	obs_slew	timestamp,  -- start of slew
	obs_start	timestamp,  -- start of observation
	obs_state	integer NOT NULL DEFAULT 0, -- observing, processing, ...
	obs_end		timestamp
);

DROP TABLE medias;

CREATE TABLE medias (
	med_id		integer PRIMARY KEY NOT NULL,
	med_path	varchar(50),
	med_mounted	boolean DEfAULT TRUE NOT NULL
);

DROP TABLE images;

CREATE TABLE images (
	img_id		integer NOT NULL,
	obs_id		integer REFERENCES observations(obs_id),
	-- A - image used for acquistion
	-- S - science image (not acqusition)
	obs_subtype	char(1),
	img_date	timestamp NOT NULL,
	img_usec	integer NOT NULL,
	img_exposure	integer,
	img_temperature	integer,
	img_filter	varchar(3),
	/* astrometry */
	astrometry	wcs,
	epoch_id	integer NOT NULL REFERENCES epoch(epoch_id),
	med_id		integer NOT NULL REFERENCES medias(med_id),
	
	camera_name	varchar(8) REFERENCES cameras(camera_name),
	mount_name	varchar(8) REFERENCES mounts(mount_name),
	delete_flag	boolean	NOT NULL DEFAULT TRUE,
	process_bitfield	integer NOT NULL default 0,
	-- processed, with darks, get astrometry etc..
	-- bit 0 - astrometry not/OK
	-- bit 1 - dark image substracted
	-- bit 2 - flatfield correction
CONSTRAINT images_prim_key PRIMARY KEY (obs_id, img_id)
);

DROP TABLE counts;

CREATE TABLE counts ( 
	obs_id		integer REFERENCES observations(obs_id),
	count_date	abstime NOT NULL,
	count_value	integer NOT NULL,
	count_exposure	float,
	count_filter	varchar(3),
	count_ra	float8,
	count_dec	float8,
	counter_name	varchar(8) REFERENCES counters(counter_name)
);

DROP TABLE scripts;

CREATE TABLE scripts (
	tar_id		integer REFERENCES targets(tar_id),
	camera_name	varchar(8) REFERENCES cameras(camera_name),
	script		varchar (2000)
);

CREATE INDEX images_obs_id ON images (obs_id);

CREATE INDEX counts_obs_id ON counts (obs_id);

DROP SEQUENCE img_id;

CREATE SEQUENCE img_id;

DROP SEQUENCE med_id;

CREATE SEQUENCE med_id;

DROP TABLE users;

CREATE TABLE users (
	usr_login	varchar(25) NOT NULL,
	usr_passwd	varchar(25) NOT NULL,
	usr_tmp		integer
);



CREATE INDEX observations_tar_id ON observations (tar_id);

DROP SEQUENCE obs_id;

CREATE SEQUENCE obs_id;

DROP VIEW observations_nights;

CREATE VIEW observations_nights AS 
	SELECT *, extract (day from (obs_start - interval '12:0:0')) AS obs_night,
		extract (month from (obs_start - interval '12:0:0')) AS obs_month,
		extract (year from (obs_start - interval '12:0:0')) AS obs_year
		FROM observations;

DROP VIEW images_nights;

CREATE VIEW images_nights AS 
	SELECT *, extract (day from (img_date - interval '12:0:0')) AS img_night, 
		extract (month from (img_date - interval '12:0:0')) AS img_month,
		extract (year from (img_date - interval '12:0:0')) AS img_year
		FROM images;



