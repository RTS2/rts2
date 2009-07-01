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

-- Add column with full image path

ALTER TABLE images ADD COLUMN img_path varchar(100);

-- create table to record double values

CREATE TABLE recvals (
	recval_id		integer PRIMARY KEY,
	device_name		varchar(25),
	value_name		varchar(25)
);

CREATE TABLE records (
	recval_id		integer REFERENCES recvals(recval_id) not NULL,
	rectime			timestamp not NULL,
	value			float8
);

CREATE INDEX records_time ON records (rectime);
CREATE INDEX records_recval_id ON records (recval_id);
CREATE UNIQUE INDEX records_id_time ON records (recval_id, rectime);

CREATE VIEW recvals_statistics AS
SELECT
	recval_id,
	device_name,
	value_name,
	(SELECT min(rectime) FROM records WHERE records.recval_id = recvals.recval_id) AS time_from,
	(SELECT max(rectime) FROM records WHERE records.recval_id = recvals.recval_id) AS time_to,
	(SELECT count (*) FROM records WHERE records.recval_id = recvals.recval_id) AS nrec
FROM
	recvals;
	

CREATE SEQUENCE recval_ids;

CREATE VIEW records_day AS
SELECT
	recval_id,
	date_trunc('day',rectime) as day,
	avg(value) as avg_value,
	min(value) as min_value,
	max(value) as max_value,
	count(*) as nrec
FROM
	records
GROUP BY
	recval_id,
	day
ORDER BY
	day,
	recval_id;


CREATE VIEW records_hour AS
SELECT
	recval_id,
	date_trunc('hour',rectime) as hour,
	avg(value) as avg_value,
	min(value) as min_value,
	max(value) as max_value,
	count(*) as nrec
FROM
	records
GROUP BY
	recval_id,
	hour
ORDER BY
	hour,
	recval_id;

SELECT * INTO mv_records_day FROM records_day;
SELECT * INTO mv_records_hour FROM records_hour;

CREATE INDEX mv_records_day_day ON mv_records_day (day);
CREATE INDEX mv_records_day_recvalid ON mv_records_day (recval_id);
CREATE UNIQUE INDEX mv_records_day_record ON mv_records_day (recval_id, day);

CREATE INDEX mv_records_hour_hour ON mv_records_hour (hour);
CREATE INDEX mv_records_hour_recvalid ON mv_records_hour (recval_id);
CREATE UNIQUE INDEX mv_records_hour_record ON mv_records_hour (recval_id, hour);

CREATE FUNCTION mv_refresh () RETURNS void AS '
DELETE FROM mv_records_hour;
DELETE FROM mv_records_day;

INSERT INTO mv_records_day (SELECT * FROM records_day);
INSERT INTO mv_records_hour (SELECT * FROM records_hour);' LANGUAGE SQL;

GRANT ALL ON recvals TO GROUP observers;
GRANT ALL ON records TO GROUP observers;
GRANT ALL ON recval_ids TO GROUP observers;
GRANT ALL ON mv_records_day TO GROUP observers;
GRANT ALL ON mv_records_hour TO GROUP observers;
GRANT ALL ON recvals_statistics TO GROUP observers;


