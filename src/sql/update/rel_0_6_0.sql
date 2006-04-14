CREATE TABLE props (
	prop_id		integer PRIMARY KEY NOT NULL,
	tar_id		integer REFERENCES targets(tar_id),
	user_id		integer REFERENCES users (usr_id),
	prop_start	timestamp,
	prop_end	timestamp
);

CREATE SEQUENCE prop_id;


CREATE TABLE plan (
	plan_id		integer PRIMARY KEY NOT NULL,
	tar_id		integer REFERENCES targets(tar_id) NOT NULL,
	prop_id		integer REFERENCES props(prop_id),
	obs_id		integer REFERENCES observations(obs_id),
	plan_start	timestamp NOT NULL,
	plan_status	integer NOT NULL DEFAULT 0
);

CREATE SEQUENCE plan_id;

CREATE INDEX plan_plan_start ON plan (plan_start);

CREATE TABLE auger (
	auger_t3id	integer PRIMARY KEY NOT NULL,
	auger_date 	timestamp NOT NULL,
	auger_npixels	integer NOT NULL,
	auger_sdpphi	float8 NOT NULL,
	auger_sdptheta	float8 NOT NULL,
	auger_sdpangle  float8 NOT NULL
);

CREATE INDEX auger_date ON auger (auger_date);

COPY types FROM stdin;
p	Plan target
A	Auger particle shower
\.

COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
7	p	Master plan	0	0	Observe by plan	t	100	0	\N
12	A	Auger particle shower	0	0	Catching hibrid showers	f	100	0	\N
\.
