ALTER TABLE auger ADD COLUMN auger_ra   	float8;
ALTER TABLE auger ADD COLUMN auger_dec		float8;

ALTER TABLE auger ALTER auger_ra  SET NOT NULL;
ALTER TABLE auger ALTER auger_dec SET NOT NULL;

CREATE SEQUENCE auger_t3id;

-- relation between observations and auger targets is kept in this table
CREATE TABLE auger_observation (
	auger_t3id	integer NOT NULL REFERENCES auger (auger_t3id),
	obs_id		integer NOT NULL REFERENCES observations (obs_id),
CONSTRAINT auger_obs_prim_key PRIMARY KEY (auger_t3id, obs_id)
);

GRANT all on auger_t3id to group observers;
