DROP FUNCTION obj_alt (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_alt (float8, float8, float8, float8, float8) 
	RETURNS float8 AS '/usr/lib/pgsql/pg_astrolib.so', 'obj_alt' LANGUAGE 'C';

DROP FUNCTION obj_az (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_az (float8, float8, float8, float8, float8) 
	RETURNS float8 AS '/usr/lib/pgsql/pg_astrolib.so', 'obj_az' LANGUAGE 'C';

DROP FUNCTION obj_rise (float8, float8, float8, float8);

CREATE FUNCTION obj_rise (float8, float8, float8, float8)
	RETURNS float8 AS '/usr/lib/pgsql/pg_astrolib.so', 'obj_rise' LANGUAGE 'C';

DROP FUNCTION obj_set (float8, float8, float8, float8);

CREATE FUNCTION obj_set (float8, float8, float8, float8)
	RETURNS float8 AS '/usr/lib/pgsql/pg_astrolib.so', 'obj_set' LANGUAGE 'C';

DROP FUNCTION obj_airmass (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_airmass (float8, float8, float8, float8, float8) 
	RETURNS float8 AS '/usr/lib/pgsql/pg_astrolib.so', 'obj_airmass' LANGUAGE 'C';

DROP FUNCTION night_num (timestamp);

CREATE FUNCTION night_num (timestamp) RETURNS numeric AS 
	'SELECT FLOOR((EXTRACT(EPOCH FROM $1) - 43200) / 86400)' LANGUAGE 'SQL';
