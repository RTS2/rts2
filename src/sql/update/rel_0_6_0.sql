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

COPY types FROM stdin;
p	Plan target
\.

COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
7	p	Master plan	0	0	Observe by plan	t	100	0	\N
\.
