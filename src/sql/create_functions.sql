DROP FUNCTION obj_alt (float4, float4, float4, float4, float4);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_alt (float4, float4, float4, float4, float4) 
	RETURNS float4 AS 'pg_astrolib', 'obj_alt' LANGUAGE 'C';

DROP FUNCTION obj_az (float4, float4, float4, float4, float4);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_az (float4, float4, float4, float4, float4) 
	RETURNS float4 AS 'pg_astrolib', 'obj_az' LANGUAGE 'C';

DROP FUNCTION obj_rise (float4, float4, float4, float4);

CREATE FUNCTION obj_rise (float4, float4, float4, float4)
	RETURNS float4 AS 'pg_astrolib', 'obj_rise' LANGUAGE 'C';

DROP FUNCTION obj_set (float4, float4, float4, float4);

CREATE FUNCTION obj_set (float4, float4, float4, float4)
	RETURNS float4 AS 'pg_astrolib', 'obj_set' LANGUAGE 'C';
