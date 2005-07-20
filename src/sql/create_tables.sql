DROP TABLE terestial;
DROP TABLE ell;
DROP TABLE ot;
DROP TABLE grb_gcn;
DROP TABLE grb;
DROP TABLE swift_observation;
DROP TABLE swift;
DROP TABLE integral;
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
	tar_enabled     boolean NOT NULL DEFAULT true,
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
	grb_id		integer NOT NULL,
	grb_seqn	integer NOT NULL,
	grb_type	integer NOT NULL, -- type of first notice which create that log entry
	grb_ra		float8,
	grb_dec		float8,
	grb_is_grb	boolean NOT NULL DEFAULT true,
	grb_date	timestamp,
	grb_last_update	timestamp,
	grb_errorbox	float,
CONSTRAINT grb_primary_key PRIMARY KEY (grb_id, grb_seqn, grb_type)
);

-- holds GCN packets for retrieved alerts, so we get for each burst
-- every information GCN sends to the world

CREATE TABLE grb_gcn (
	grb_id		integer,
	grb_seqn	integer,
	grb_type	integer,
	grb_update	timestamp,
	grb_update_usec	integer,
	packet		int8[]
);

CREATE INDEX grb_gcn_prim ON grb_gcn (grb_id, grb_seqn, grb_type);

-- holds script for GRB observations

CREATE TABLE grb_script (
	camera_name		varchar(8) REFERENCES cameras (camera_name),
	grb_script_end		integer,  -- post GRB second end
	  -- time, can be null for default script
	grb_script_script	varchar(2000) NOT NULL,
CONSTRAINT prim_grb_script PRIMARY KEY (camera_name, grb_script_start)
);

-- swift pointing

CREATE TABLE swift (
	swift_id	integer PRIMARY KEY,
	swift_ra	float8 NOT NULL,
	swift_dec	float8 NOT NULL,
	swift_roll	float8,
        swift_received	timestamp NOT NULL,
	swift_time	timestamp,
        swift_name      varchar(70),
        swift_obstime   float,
	swift_merit	float
);

CREATE TABLE integral (
	integral_id	integer PRIMARY KEY,
	integral_ra	float8 NOT NULL,
	integral_dec	float8 NOT NULL,
	integral_received	timestamp NOT NULL,
	integral_time	timestamp
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

DROP TABLE observations;

CREATE TABLE observations (
	tar_id		integer REFERENCES targets (tar_id),
	obs_id		integer PRIMARY KEY NOT NULL,
	obs_ra		float8,
	obs_dec		float8,
	obs_alt		float,
	obs_az		float,
	obs_slew	timestamp,  -- start of slew
	obs_start	timestamp,  -- start of observation
	obs_state	integer NOT NULL DEFAULT 0, -- observing, processing, ...
	obs_end		timestamp
);

DROP TABLE darks;

CREATE TABLE darks (
	obs_id		integer REFERENCES observations(obs_id),
	img_id		integer NOT NULL,
	dark_date	timestamp,
	dark_usec	integer,
	dark_exposure	float,
	dark_temperature float,
	epoch_id	integer NOT NULL REFERENCES epoch(epoch_id),
	camera_name	varchar(8) REFERENCES cameras(camera_name),
CONSTRAINT darks_prim_key PRIMARY KEY (obs_id, img_id)
);

CREATE INDEX darks_all ON darks (dark_exposure, dark_temperature, dark_date);

CREATE INDEX darks_dates ON darks (dark_date);

CREATE INDEX darks_exposures ON darks (dark_date);

CREATE INDEX darks_temperature ON darks (dark_temperature);

DROP TABLE flats;

CREATE TABLE flats ( 
	flat_name	varchar(250) NOT NULL,
	flat_date	timestamp,
	flat_exposure	integer,
	flat_temperature integer,
	epoch_id	integer NOT NULL REFERENCES epoch(epoch_id),
	camera_name	varchar(8) REFERENCES cameras(camera_name),
CONSTRAINT flats_prim_key PRIMARY KEY (flat_date, camera_name)
);

DROP SEQUENCE tar_id;

CREATE SEQUENCE tar_id START WITH 1000;

DROP SEQUENCE grb_tar_id;

CREATE SEQUENCE grb_tar_id START WITH 50000;

DROP SEQUENCE point_id;

CREATE SEQUENCE point_id;

CREATE TABLE swift_observation (
        swift_id        integer NOT NULL REFERENCES swift (swift_id),
	obs_id		integer NOT NULL REFERENCES observations (obs_id),
CONSTRAINT swift_obs_prim_key PRIMARY KEY (swift_id, obs_id)
);

DROP TABLE medias;

CREATE TABLE medias (
	med_id		integer PRIMARY KEY NOT NULL,
	med_path	varchar(50),
	med_mounted	boolean DEfAULT TRUE NOT NULL
);

DROP TABLE images;

CREATE TABLE images (
	obs_id		integer REFERENCES observations(obs_id),
	img_id		integer NOT NULL,
	-- A - image used for acquistion
	-- S - science image (not acqusition)
	obs_subtype	char(1),
	img_date	timestamp NOT NULL,
	img_usec	integer NOT NULL,
	img_exposure	float,
	img_temperature	float,
	img_filter	varchar(3),
	img_alt		float,
	img_az		float,
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
	script		varchar (2000) NOT NULL
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



