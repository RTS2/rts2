-- table for events, recorded by eventd
CREATE TABLE message (
	message_time	timestamp,
	message_oname	varchar(8),
	message_type	integer,
	message_string	varchar(200)
);

CREATE INDEX message_time ON message (message_time);

CREATE INDEX message_oname ON message (message_oname);

CREATE INDEX message_type ON message (message_type);

-- filters infomration
CREATE TABLE filters (
	filter_id	varchar(3) PRIMARY KEY,
	offset_ra	float8,
	offset_dec	float8,
	-- standart name - UBVR & others
	standart_name	varchar(10),
	description	varchar(2000),
	-- medium wavelenght in nm
	medium_wl	float,
	-- filter width in nm
	width		float
);

UPDATE images SET img_filter = 'UNK' WHERE img_filter is NULL;

INSERT INTO filters (filter_id, standart_name) SELECT DISTINCT img_filter, img_filter FROM images;

ALTER TABLE images ADD CONSTRAINT "fk_images_filters" FOREIGN KEY (img_filter) REFERENCES filters(filter_id);

CREATE INDEX images_filters ON images (img_filter);

COPY types FROM stdin;
L	Planets
\.

COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
190	L	Sun	0	0	Sun (planet)	t	0	0	\N
191	L	Mercury	0	0	Mercury (planet)	t	0	0	\N
192	L	Venus	0	0	Venus (planet)	t	0	0	\N
193	L	Moon	0	0	Moon (Earth)	t	0	0	\N
194	L	Mars	0	0	Mars (planet)	t	0	0	\N
195	L	Jupiter	0	0	Jupiter (planet)	t	0	0	\N
196	L	Saturn	0	0	Saturn (planet)	t	0	0	\N
197	L	Urans	0	0	Uran (planet)	t	0	0	\N
198	L	Neptune	0	0	Neptun (planet)	t	0	0	\N
199	L	Pluto	0	0	Pluto (planet)	t	0	0	\N
\.
