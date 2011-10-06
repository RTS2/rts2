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
' LANGUAGE 'SQL';

alter table grb add column grb_autodisabled boolean not null default false;

alter table targets add column interruptible boolean not null default true;
