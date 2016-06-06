-- sequence for filters
CREATE SEQUENCE filter_id;

-- change filter_id from VARCHAR to integer, clear DB design
ALTER TABLE images ADD COLUMN filter_id integer;
DROP INDEX fk_images_filters;
ALTER TABLE filters RENAME COLUMN filter_id TO f_id;

ALTER TABLE filters ADD COLUMN filter_id integer;
UPDATE filters SET filter_id = nextval('filter_id');
UPDATE images SET filter_id = (SELECT filter_id FROM filters WHERE f_id = img_filter);

ALTER TABLE filters DROP COLUMN f_id CASCADE;
ALTER TABLE images DROP COLUMN img_filter CASCADE;

CREATE UNIQUE INDEX fk_filters ON filters(filter_id);
ALTER TABLE images ADD CONSTRAINT "fk_images_filters2" FOREIGN KEY (filter_id) REFERENCES filters(filter_id);
CREATE UNIQUE INDEX index_filter_names ON filters(standart_name);

-- recreate views dropped during upgrade
CREATE VIEW observations_noimages AS 
SELECT observations.obs_id, 0 AS img_count 
	FROM observations 
	WHERE (NOT (EXISTS 
		(SELECT * FROM images 
			WHERE observations.obs_id = images.obs_id)));

CREATE VIEW observations_images AS 
SELECT obs_id, img_count 
	FROM observations_imgcount 
UNION SELECT obs_id, img_count 
	FROM observations_noimages;

CREATE VIEW images_nights AS
SELECT images.*, date_part('day', (timestamptz(images.img_date) - interval '12:00')) AS img_night, 
	date_part('month', (timestamptz(images.img_date) - interval '12:00')) AS img_month, 
	date_part('year', (timestamptz(images.img_date) - interval '12:00')) AS img_year 
	FROM images;

CREATE TABLE queues_targets (
	qid         integer PRIMARY KEY,
	queue_id    integer NOT NULL,
	tar_id 	    integer REFERENCES targets(tar_id),
	plan_id     integer REFERENCES plan(plan_id),
	time_start  timestamp with time zone,
	time_end    timestamp with time zone,
	queue_order integer
);

CREATE TABLE queues (
	queue_id                integer PRIMARY KEY,
	queue_type              integer NOT NULL,
	skip_below_horizon      boolean NOT NULL,
	test_constraints        boolean NOT NULL,
	remove_after_execution  boolean NOT NULL,
	block_until_visible     boolean NOT NULL,
	queue_enabled           boolean NOT NULL,
	queue_window            float NOT NULL
);

CREATE SEQUENCE qid;

-- add bb ID for planned entries
ALTER TABLE plan ADD COLUMN bb_observatory_id integer;
ALTER TABLE plan ADD COLUMN bb_schedule_id integer;

ALTER TABLE observations ADD COLUMN plan_id integer REFERENCES plan(plan_id);
ALTER TABLE plan DROP COLUMN obs_id;

ALTER TABLE targets ADD COLUMN tar_telescope_mode integer default NULL;

ALTER TABLE users ADD COLUMN allowed_devices varchar(2000);

CREATE OR REPLACE FUNCTION delete_target (integer) RETURNS integer AS
'
DELETE FROM images WHERE EXISTS (SELECT * FROM observations WHERE observations.obs_id = images.obs_Id AND observations.tar_id = $1);
DELETE FROM plan WHERE tar_id = $1;
DELETE FROM observations WHERE tar_id = $1;
DELETE FROM scripts WHERE tar_id = $1;
DELETE FROM target_labels WHERE tar_id = $1;
DELETE FROM grb WHERE tar_id = $1;
DELETE FROM queues_targets WHERE tar_id = $1;
DELETE FROM targets WHERE tar_id = $1;
SELECT 1;
' LANGUAGE 'SQL';

