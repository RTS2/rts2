DROP FUNCTION obj_alt (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_alt (float8, float8, float8, float8, float8) 
	RETURNS float8 AS 'pg_astrolib', 'obj_alt' LANGUAGE 'C';

DROP FUNCTION obj_az (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_az (float8, float8, float8, float8, float8) 
	RETURNS float8 AS 'pg_astrolib', 'obj_az' LANGUAGE 'C';

DROP FUNCTION obj_rise (float8, float8, float8, float8);

CREATE FUNCTION obj_rise (float8, float8, float8, float8)
	RETURNS float8 AS 'pg_astrolib', 'obj_rise' LANGUAGE 'C';

DROP FUNCTION obj_set (float8, float8, float8, float8);

CREATE FUNCTION obj_set (float8, float8, float8, float8)
	RETURNS float8 AS 'pg_astrolib', 'obj_set' LANGUAGE 'C';

DROP FUNCTION obj_airmass (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_airmass (float8, float8, float8, float8, float8) 
	RETURNS float8 AS 'pg_astrolib', 'obj_airmass' LANGUAGE 'C';

