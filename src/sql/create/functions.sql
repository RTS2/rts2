CREATE OR REPLACE FUNCTION night_num (timestamp with time zone) RETURNS double precision AS 
	'SELECT FLOOR((EXTRACT(EPOCH FROM $1) - 43200) / 86400)' LANGUAGE 'SQL';

CREATE OR REPLACE FUNCTION isinwcs (float8, float8, wcs)
  RETURNS bool AS 'pg_wcs.so', 'isinwcs' LANGUAGE 'C';

CREATE OR REPLACE FUNCTION imgrange (wcs)
  RETURNS varchar AS 'pg_wcs.so', 'imgrange' LANGUAGE 'C';

-- 			med_id  epoch    mount_name camera_name obs_id  tar_id   date
CREATE OR REPLACE FUNCTION imgpath(integer, integer, varchar(8), varchar(8), integer, integer, abstime) RETURNS varchar(100) AS '
DECLARE
	path	varchar(50);
	tar_id	varchar(10);
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
	IF LENGTH($6) < 5 THEN
	  tar_id := LPAD ($6, 5, ''0'');
	ELSE
	  tar_id := $6;
	END IF;
	name:=EXTRACT(YEAR FROM $7) || LPAD(EXTRACT(MONTH FROM $7), 2, ''0'') || LPAD(EXTRACT(DAY FROM $7), 2, ''0'') || LPAD(EXTRACT(HOUR FROM $7), 2, ''0'') || LPAD(EXTRACT(MINUTE FROM $7), 2, ''0'') || LPAD(EXTRACT(SECOND FROM $7), 2, ''0'');
	ep := LPAD ($2, 3, ''0'');
		
	RETURN path || ''/'' || ep || ''/'' || $4 || ''/'' || tar_id || ''/'' || name || ''.fits'';
	
END;
' LANGUAGE plpgsql;

--                                        date       usec
CREATE OR REPLACE FUNCTION img_fits_name (timestamp, integer) RETURNS varchar (30) AS '
BEGIN
   return EXTRACT(YEAR FROM $1) || LPAD(EXTRACT(MONTH FROM $1), 2, ''0'') || LPAD(EXTRACT(DAY FROM $1), 2, ''0'')
	  || LPAD(EXTRACT(HOUR FROM $1), 2, ''0'') || LPAD(EXTRACT(MINUTE FROM $1), 2, ''0'')
	  || LPAD(EXTRACT(SECOND FROM $1), 2, ''0'') || ''-'' || LPAD ($2 / 1000, 4, ''0'');
END
' LANGUAGE plpgsql;

-- 			             obs_id   img_id   dark_date  dark_usec  epoch_id camera_name
CREATE OR REPLACE FUNCTION dark_name(integer, integer, timestamp, integer,   integer, varchar(8)) RETURNS varchar(250) AS '
DECLARE
	d_tar_id integer;
	d_type_id char;
BEGIN
	SELECT INTO
		d_tar_id
		tar_id
	FROM
		observations
	WHERE
		obs_id = $1;

	IF d_tar_id = 1 THEN
		return ''/images/'' || LPAD ($5, 3, ''0'') || ''/darks/'' 
			|| $6 || ''/'' || img_fits_name ($3, $4) || ''.fits'';
	ELSE
		return ''/images/'' || LPAD ($5, 3, ''0'') || ''/archive/''
			|| LPAD (d_tar_id, 5, ''0'') || ''/'' || $6 || ''/darks/''
			|| img_fits_name ($3, $4) || ''.fits'';
	END IF;
END;
' LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION ell_update (varchar (150), float4, float4, 
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
                
                INSERT INTO targets (
			tar_id,
			type_id,
			tar_name,
                	tar_ra,
			tar_dec,
			tar_comment,
			tar_enabled
		)
                VALUES (
			new_tar_id,
			''E'',
			name,
			0,
			0,
			name,
			false
		);
                
                INSERT INTO ell (
			tar_id,
			ell_a,
			ell_e,
			ell_i,
			ell_w,
			ell_omega,
			ell_n,
			ell_minpause,
			ell_priority,
                  	ell_JD,
			ell_mag_1,
			ell_mag_2
		)
                VALUES (
			new_tar_id,
			in_ell_a,
			in_ell_e,
			in_ell_i,
                	in_ell_w,
			in_ell_omega,
			in_ell_n,
			''10 minutes'',
			-1,
			in_ell_JD,
			in_ell_mag_1,
			in_ell_mag_2
		);
         END IF;
         RETURN 1;
END;
' LANGUAGE plpgsql;
