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
	rectime			timestamp,
	value			float8
);

CREATE SEQUENCE recval_ids;

GRANT ALL ON recvals TO GROUP observers;
GRANT ALL ON records TO GROUP observers;
GRANT ALL ON recval_ids TO GROUP observers;
