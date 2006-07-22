CREATE TABLE flats ( 
        obs_id		integer REFERENCES observations(obs_id),
	img_id		integer NOT NOT,
	flat_date	timestamp,
	flat_date_usec  integer,
	flat_exposure	integer,
	flat_temperature integer,
	epoch_id	integer NOT NULL REFERENCES epoch(epoch_id),
	camera_name	varchar(8) REFERENCES cameras(camera_name),
CONSTRAINT flats_prim_key PRIMARY KEY (obs_id, img_id)
);

ALTER TABLE auger ADD COLUMN auger_ra   	float8;
ALTER TABLE auger ADD COLUMN auger_dec		float8;

ALTER TABLE auger ALTER auger_ra  SET NOT NULL;
ALTER TABLE auger ALTER auger_dec SET NOT NULL;

CREATE SEQUENCE auger_t3id;
