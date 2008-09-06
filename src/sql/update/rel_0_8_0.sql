CREATE TABLE accounts (
	account_id      integer PRIMARY KEY,
	account_name    varchar (150) NOT NULL,
	account_share   float NOT NULL   --- share of account on observation time (in arbitary units, but it is assumed that share is equal to account_share / sum (account_share)
);

--- Modify target table to include account

ALTER TABLE targets ADD COLUMN account_id integer;

INSERT INTO accounts VALUES (1, 'Default account', 100);

UPDATE TARGETS set account_id = 1;

ALTER TABLE ONLY targets ADD CONSTRAINT targets_accounts
	FOREIGN KEY (account_id) REFERENCES accounts (account_id); 

-- Drop darks and flats

DROP TABLE darks;

DROP TABLE flats;
