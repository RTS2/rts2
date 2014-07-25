CREATE TABLE accounts (
	account_id      integer PRIMARY KEY,
	account_name    varchar (150) NOT NULL,
	account_share   float NOT NULL   --- share of account on observation time (in arbitary units, but it is assumed that share is equal to account_share / sum (account_share)
);

--- Scheduling ticket table

CREATE TABLE tickets (
	schedticket_id		integer PRIMARY KEY,
	tar_id 			integer REFERENCES targets(tar_id) not NULL,
	account_id		integer REFERENCES accounts(account_id) not NULL,
	obs_num			integer default NULL, -- how many times observation can be repeated, null means unlimited
	sched_from		timestamp default NULL, -- scheduling from this date
	sched_to		timestamp default NULL, -- scheduling to this date
	sched_interval_min	interval default NULL,  -- minimal interval between observations. 
	sched_interval_max	interval default NULL  -- maximal interval between observations
);

--- Create default account

INSERT INTO accounts VALUES (1, 'Default account', 100);

GRANT ALL ON accounts TO GROUP observers;

GRANT ALL ON tickets TO GROUP observers;

--- Drop darks and flats

DROP TABLE darks;

DROP TABLE flats;

-- Drop epochs

ALTER TABLE images DROP COLUMN epoch_id CASCADE;

ALTER TABLE phot DROP COLUMN phot_epoch CASCADE;

DROP TABLE epoch;

DROP FUNCTION imgpath(integer, integer, varchar(8), varchar(8), integer, integer, abstime);
DROP FUNCTION img_fits_name(timestamp, integer);
DROP FUNCTION dark_name(integer, integer, timestamp, integer,   integer, varchar(8));
DROP FUNCTION ell_update(varchar (150), float4, float4, float4, float4, float4, float4, float8, float4, float4);

-- Drop ell - they are now stored as MPEC one-liners

DROP TABLE ell;

-- Add column with full image path

ALTER TABLE images ADD COLUMN img_path varchar(100);

-- create table to record double values

CREATE TABLE recvals (
	recval_id		integer PRIMARY KEY,
	device_name		varchar(25),
	value_name		varchar(25) NOT NULL,
	-- type of value - 0 is state, 1 is double record
	value_type		integer NOT NULL
);

CREATE SEQUENCE recval_ids;

CREATE TABLE records_state (
	recval_id		integer REFERENCES recvals(recval_id) not NULL,
	rectime			timestamp not NULL,
	state			integer
);

CREATE TABLE records_double (
	recval_id		integer REFERENCES recvals(recval_id) not NULL,
	rectime			timestamp not NULL,
	value			float8
);

CREATE INDEX records_double_time ON records_double (rectime);
CREATE INDEX records_double_recval_id ON records_double (recval_id);
CREATE UNIQUE INDEX records_double_id_time ON records_double (recval_id, rectime);

CREATE TABLE records_boolean (
	recval_id		integer REFERENCES recvals(recval_id) not NULL,
	rectime			timestamp not NULL,
	value			boolean
);

CREATE INDEX records_boolean_time ON records_boolean (rectime);
CREATE INDEX records_boolean_recval_id ON records_boolean (recval_id);
CREATE UNIQUE INDEX records_boolean_id_time ON records_boolean (recval_id, rectime);

CREATE VIEW recvals_state_statistics AS
SELECT
	recval_id,
	device_name,
	value_name,
	(SELECT min(rectime) FROM records_state WHERE records_state.recval_id = recvals.recval_id) AS time_from,
	(SELECT max(rectime) FROM records_state WHERE records_state.recval_id = recvals.recval_id) AS time_to
FROM
	recvals
WHERE
	value_type = 0;

DROP VIEW recvals_double_statistics;

CREATE VIEW recvals_double_statistics AS
SELECT
	recval_id,
	device_name,
	value_name,
	value_type,
	(SELECT min(rectime) FROM records_double WHERE records_double.recval_id = recvals.recval_id) AS time_from,
	(SELECT max(rectime) FROM records_double WHERE records_double.recval_id = recvals.recval_id) AS time_to
FROM
	recvals
WHERE
	value_type & 15 = 4;

CREATE VIEW recvals_boolean_statistics AS
SELECT
	recval_id,
	device_name,
	value_name,
	value_type,
	(SELECT min(rectime) FROM records_boolean WHERE records_boolean.recval_id = recvals.recval_id) AS time_from,
	(SELECT max(rectime) FROM records_boolean WHERE records_boolean.recval_id = recvals.recval_id) AS time_to
FROM
	recvals
WHERE
	value_type & 15 = 6;
	

CREATE VIEW records_double_day AS
SELECT
	recval_id,
	date_trunc('day',rectime) as day,
	avg(value) as avg_value,
	min(value) as min_value,
	max(value) as max_value,
	count(*) as nrec
FROM
	records_double
GROUP BY
	recval_id,
	day
ORDER BY
	day,
	recval_id;


CREATE VIEW records_double_hour AS
SELECT
	recval_id,
	date_trunc('hour',rectime) as hour,
	avg(value) as avg_value,
	min(value) as min_value,
	max(value) as max_value,
	count(*) as nrec
FROM
	records_double
GROUP BY
	recval_id,
	hour
ORDER BY
	hour,
	recval_id;

SELECT * INTO mv_records_double_day FROM records_double_day;
SELECT * INTO mv_records_double_hour FROM records_double_hour;

CREATE INDEX mv_records_double_day_day ON mv_records_double_day (day);
CREATE INDEX mv_records_double_day_recvalid ON mv_records_double_day (recval_id);
CREATE UNIQUE INDEX mv_records_double_day_record ON mv_records_double_day (recval_id, day);

CREATE INDEX mv_records_double_hour_hour ON mv_records_double_hour (hour);
CREATE INDEX mv_records_double_hour_recvalid ON mv_records_double_hour (recval_id);
CREATE UNIQUE INDEX mv_records_double_hour_record ON mv_records_double_hour (recval_id, hour);

CREATE FUNCTION mv_refresh () RETURNS void AS '
DELETE FROM mv_records_double_hour;
DELETE FROM mv_records_double_day;

INSERT INTO mv_records_double_day (SELECT * FROM records_double_day);
INSERT INTO mv_records_double_hour (SELECT * FROM records_double_hour);' LANGUAGE SQL;

CREATE UNIQUE INDEX plan_tar_id_start ON plan (tar_id, plan_start);

-- second parameter is site longitude
CREATE OR REPLACE FUNCTION to_night(timestamp with time zone, numeric) RETURNS timestamp without time zone AS
	'SELECT (to_timestamp (EXTRACT(EPOCH FROM $1) + 86400 * $2 / 360 - 43200) AT TIME ZONE ''UTC'' )' LANGUAGE 'sql';

GRANT ALL ON recvals TO GROUP observers;
GRANT ALL ON records_state TO GROUP observers;
GRANT ALL ON records_double TO GROUP observers;
GRANT ALL ON records_boolean TO GROUP observers;
GRANT ALL ON recval_ids TO GROUP observers;
GRANT ALL ON mv_records_double_day TO GROUP observers;
GRANT ALL ON mv_records_double_hour TO GROUP observers;
GRANT ALL ON recvals_state_statistics TO GROUP observers;
GRANT ALL ON recvals_double_statistics TO GROUP observers;
GRANT ALL ON recvals_boolean_statistics TO GROUP observers;
