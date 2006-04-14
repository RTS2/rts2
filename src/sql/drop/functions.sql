DROP FUNCTION night_num (timestamp with time zone) RETURNS double precision;

DROP FUNCTION isinwcs (float8, float8, wcs) RETURNS bool;

DROP FUNCTION imgrange (wcs) RETURNS varchar;

DROP FUNCTION imgpath(integer, integer, varchar(8), varchar(8), integer, integer, abstime) RETURNS varchar(100);

DROP FUNCTION img_fits_name (timestamp, integer) RETURNS varchar (30);

DROP FUNCTION dark_name(integer, integer, timestamp, integer,   integer, varchar(8)) RETURNS varchar(250);

DROP FUNCTION ell_update (varchar (150), float4, float4, float4, float4, float4, float4, float8, float4, float4) RETURNS integer;
