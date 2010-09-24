alter table users add column usr_execute_permission boolean not null default false;

create table pi (
	pi_id		integer PRIMARY KEY,
	pi_name		varchar(250) NOT NULL
);

create table programs (
	program_id	integer PRIMARY KEY,
	program_name	varchar(250) NOT NULL
);

alter table targets add column pi_id integer REFERENCES pi(pi_id);

alter table targets add column program_id integer REFERENCES programs(program_id);
