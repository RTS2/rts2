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
