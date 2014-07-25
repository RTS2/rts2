alter table users add column usr_execute_permission boolean not null default false;

create table labels (
	label_id	integer PRIMARY KEY,
	label_type	integer,
	label_text	varchar(500) NOT NULL
);

create table target_labels (
	tar_id		integer REFERENCES targets(tar_id),
	label_id	integer REFERENCES labels(label_id)
);

create sequence label_id start WITH 1;

CREATE OR REPLACE FUNCTION delete_target (integer) RETURNS integer AS
'
DELETE FROM images WHERE EXISTS (SELECT * FROM observations WHERE observations.obs_id = images.obs_Id AND observations.tar_id = $1);
DELETE FROM plan WHERE tar_id = $1;
DELETE FROM observations WHERE tar_id = $1;
DELETE FROM scripts WHERE tar_id = $1;
DELETE FROM target_labels WHERE tar_id = $1;
DELETE FROM grb WHERE tar_id = $1;
DELETE FROM targets WHERE tar_id = $1;
SELECT 1;
' LANGUAGE 'sql';

alter table grb add column grb_autodisabled boolean not null default false;

alter table targets add column interruptible boolean not null default true;

--- add proper motion
alter table targets add column tar_pm_ra float8 default null;
alter table targets add column tar_pm_dec float8 default null;

CREATE TABLE records_integer (
	recval_id		integer REFERENCES recvals(recval_id) not NULL,
	rectime			timestamp not NULL,
	value			integer
);

CREATE INDEX records_integer_time ON records_integer (rectime);
CREATE INDEX records_integer_recval_id ON records_integer (recval_id);
CREATE UNIQUE INDEX records_integer_id_time ON records_integer (recval_id, rectime);

DROP VIEW recvals_integer_statistics;

CREATE VIEW recvals_integer_statistics AS
SELECT
	recval_id,
	device_name,
	value_name,
	value_type,
	(SELECT min(rectime) FROM records_integer WHERE records_integer.recval_id = recvals.recval_id) AS time_from,
	(SELECT max(rectime) FROM records_integer WHERE records_integer.recval_id = recvals.recval_id) AS time_to
FROM
	recvals
WHERE
	value_type & 15 = 2;

GRANT ALL ON records_integer TO GROUP observers;
GRANT ALL ON recvals_integer_statistics TO GROUP observers;

ALTER TABLE users DROP COLUMN usr_passwd;
ALTER TABLE users ADD COLUMN usr_passwd varchar(100);
