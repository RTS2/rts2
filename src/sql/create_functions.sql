DROP FUNCTION obj_alt (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_alt (float8, float8, float8, float8, float8) 
	RETURNS float8 AS '/usr/lib/postgresql/lib/pg_astrolib.so', 'obj_alt' LANGUAGE 'C';

DROP FUNCTION obj_az (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_az (float8, float8, float8, float8, float8) 
	RETURNS float8 AS '/usr/lib/postgresql/lib/pg_astrolib.so', 'obj_az' LANGUAGE 'C';

DROP FUNCTION obj_rise (float8, float8, float8, float8);

CREATE FUNCTION obj_rise (float8, float8, float8, float8)
	RETURNS float8 AS '/usr/lib/postgresql/lib/pg_astrolib.so', 'obj_rise' LANGUAGE 'C';

DROP FUNCTION obj_set (float8, float8, float8, float8);

CREATE FUNCTION obj_set (float8, float8, float8, float8)
	RETURNS float8 AS '/usr/lib/postgresql/lib/pg_astrolib.so', 'obj_set' LANGUAGE 'C';

DROP FUNCTION obj_airmass (float8, float8, float8, float8, float8);

-- 			ra       dec     JD      lon     lat
CREATE FUNCTION obj_airmass (float8, float8, float8, float8, float8) 
	RETURNS float8 AS '/usr/lib/postgresql/lib/pg_astrolib.so', 'obj_airmass' LANGUAGE 'C';

DROP FUNCTION night_num (timestamp);

CREATE FUNCTION night_num (timestamp with time zone) RETURNS double precision AS 
	'SELECT FLOOR((EXTRACT(EPOCH FROM $1) - 43200) / 86400)' LANGUAGE 'SQL';

DROP FUNCTION isinwcs (float4, float4, wcs);

CREATE FUNCTION isinwcs (float4, float4, wcs)
  RETURNS bool AS '/usr/lib/postgresql/lib/pg_wcs.so', 'isinwcs' LANGUAGE 'C';

DROP FUNCTION imgrange (wcs);

CREATE FUNCTION imgrange (wcs)
  RETURNS varchar AS '/usr/lib/postgresql/lib/pg_wcs.so', 'imgrange' LANGUAGE 'C';

DROP FUNCTION imgpath(integer, char(3), varchar(8), varchar(8), integer, integer, abstime);
-- 			med_id  epoch    mount_name camera_name obs_id  tar_id   date
CREATE FUNCTION imgpath(integer, char(3), varchar(8), varchar(8), integer, integer, abstime) RETURNS varchar(100) AS '
DECLARE
	path	varchar(50);
	tar_id	varchar(5);
	ep 	varchar(3);
	name	varchar(14);
	mounted boolean;
BEGIN
        SELECT INTO mounted med_mounted FROM medias WHERE med_id = $1;
	IF NOT FOUND THEN
		RAISE EXCEPTION ''Cannot found med_id %'', $1;
		RETURN '''';
	END IF;
	IF NOT mounted THEN
		RETURN '''';
	END IF;
	SELECT INTO path med_path FROM medias WHERE med_id = $1;
	tar_id:=LPAD ($6, 5, ''0'');
	name:=EXTRACT(YEAR FROM $7) || LPAD(EXTRACT(MONTH FROM $7), 2, ''0'') || LPAD(EXTRACT(DAY FROM $7), 2, ''0'') || LPAD(EXTRACT(HOUR FROM $7), 2, ''0'') || LPAD(EXTRACT(MINUTE FROM $7), 2, ''0'') || LPAD(EXTRACT(SECOND FROM $7), 2, ''0'');
	ep:=$2;
		
	RETURN path || ''/'' || ep || ''/'' || $4 || ''/'' || tar_id || ''/'' || name || ''.fits'';
	
END;
' LANGUAGE 'plpgsql';

DROP FUNCTION ell_update (varchar (150), float4, float4, 
float4, float4, float4, float4, float8, float4, float4);

CREATE FUNCTION ell_update (varchar (150), float4, float4, 
float4, float4, float4, float4, float8, float4, float4) 
RETURNS integer AS '
DECLARE
        name ALIAS FOR $1;
        in_ell_a ALIAS FOR $2;
        in_ell_e ALIAS FOR $3;
        in_ell_i ALIAS FOR $4;
        in_ell_w ALIAS FOR $5;
        in_ell_omega ALIAS FOR $6;
        in_ell_n ALIAS FOR $7;
        in_ell_jd ALIAS FOR $8;
        in_ell_mag_1 ALIAS FOR $9;
        in_ell_mag_2 ALIAS FOR $10;
        new_tar_id integer;
BEGIN
        IF EXISTS (SELECT * FROM targets WHERE tar_name = name) THEN
                SELECT INTO new_tar_id tar_id FROM targets WHERE 
                        tar_name = name;
                UPDATE ell SET
                        ell_a = in_ell_a,
                        ell_e = in_ell_e,
                        ell_i = in_ell_i,
                        ell_w = in_ell_w,
                        ell_omega = in_ell_omega,
                        ell_n = in_ell_n,
                        ell_jd = in_ell_jd,
                        ell_mag_1 = in_ell_mag_1,
                        ell_mag_2 = in_ell_mag_2
                WHERE
                        tar_id = new_tar_id;
         ELSE
                SELECT INTO new_tar_id nextval (''tar_id''); 
                
                INSERT INTO targets (tar_id, type_id, tar_name,
                tar_ra, tar_dec, tar_comment)
                VALUES (new_tar_id, ''P'', name, 0, 0, name);
                
                INSERT INTO ell (tar_id, ell_a, ell_e, ell_i, ell_w,
                  ell_omega, ell_n, ell_minpause, ell_priority,
                  ell_JD, ell_mag_1, ell_mag_2)
                VALUES (new_tar_id, in_ell_a, in_ell_e, in_ell_i,
                in_ell_w, in_ell_omega, in_ell_n, ''10 minutes'', -1,
                in_ell_JD, in_ell_mag_1, in_ell_mag_2);
         END IF;
         RETURN 1;
END;
' LANGUAGE plpgsql;
