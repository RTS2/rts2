CREATE OR REPLACE FUNCTION night_num (timestamp with time zone) RETURNS double precision AS 
	'SELECT FLOOR((EXTRACT(EPOCH FROM $1) - 43200) / 86400)' LANGUAGE 'sql';
