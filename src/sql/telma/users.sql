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

CREATE TABLE telma_users (
  	-- usr_login, pasword and email are stored in users table
	usr_id           integer NOT NULL REFERENCES users (usr_id),
	birthday         date,
	birthplace       varchar(50),
	sex              char,
	cellphone        varchar(15),
	address          varchar(200),
	city             varchar(50),
	country          integer REFERENCES countries(country_id)
);


