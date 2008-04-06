-- list of countries for country

CREATE TABLE countries ( 
	country_id       integer NOT NULL,
	country_name     varchar(50),
CONSTRAINT country_primary_key PRIMARY KEY (country_id)
);

COPY countries (country_id, country_name) FROM stdin;
1	Espana
2	Unidos Estatos
\.

-- users table, which holds users extended details

ALTER TABLE users ADD COLUMN birthday date;
ALTER TABLE users ADD COLUMN birthplace varchar(50);
ALTER TABLE users ADD COLUMN sex char;
ALTER TABLE users ADD COLUMN cellphone varchar(15);
ALTER TABLE users ADD COLUMN address varchar(200);
ALTER TABLE users ADD COLUMN city varchar(50);
ALTER TABLE users ADD COLUMN country integer REFERENCES (countries (country_id));
