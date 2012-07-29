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

-- filters information
CREATE TABLE filters (
	filter_id	integer PRIMARY KEY,
	offset_ra	float8,
	offset_dec	float8,
	-- standart name - UBVR & others
	standart_name	varchar(40),
	-- medium wavelenght in nm
	medium_wl	float,
	-- filter width in nm
	width		float
);

UPDATE images SET img_filter = 'UNK' WHERE img_filter is NULL;

INSERT INTO filters (filter_id, standart_name) SELECT DISTINCT img_filter, img_filter FROM images;

ALTER TABLE images ADD CONSTRAINT "fk_images_filters" FOREIGN KEY (img_filter) REFERENCES filters(filter_id);

CREATE INDEX images_filters ON images (img_filter);
