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
A	Auger particle shower
\.

COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
12	A	Auger particle shower	0	0	Catching hibrid showers	f	100	0	\N
\.


GRANT ALL ON auger TO GROUP observers;
GRANT ALL ON auger_observation TO GROUP observers;
