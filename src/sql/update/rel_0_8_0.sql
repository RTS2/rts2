CREATE TABLE accounts (
	account_id      integer PRIMARY KEY,
	account_name    varchar (150) NOT NULL,
	account_share   float NOT NULL   --- share of account on observation time (in arbitary units, but it is assumed that share is equal to account_share / sum (account_share)
);

--- Scheduling ticket table

CREATE TABLE tickets (
	schedticket_id	integer PRIMARY KEY,
	tar_id 		integer REFERENCES targets(tar_id),
	account_id	integer REFERENCES accounts(account_id),
	sched_from	timestamp,
	sched_to	timestamp
);

--- Create default account

INSERT INTO accounts VALUES (1, 'Default account', 100);

--- Drop darks and flats

DROP TABLE darks;

DROP TABLE flats;
