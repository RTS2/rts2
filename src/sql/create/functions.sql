CREATE OR REPLACE FUNCTION night_num (timestamp with time zone) RETURNS double precision AS 
	'SELECT FLOOR((EXTRACT(EPOCH FROM $1) - 43200) / 86400)' LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION isinwcs (float8, float8, wcs)
  RETURNS bool AS 'pg_wcs.so', 'isinwcs' LANGUAGE 'C';

CREATE OR REPLACE FUNCTION imgrange (wcs)
  RETURNS varchar AS 'pg_wcs.so', 'imgrange' LANGUAGE 'C';
