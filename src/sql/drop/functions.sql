DROP FUNCTION night_num (timestamp with time zone);

DROP FUNCTION isinwcs (float8, float8, wcs);

DROP FUNCTION imgrange (wcs);

DROP FUNCTION imgpath(integer, integer, varchar(8), varchar(8), integer, integer, abstime);

DROP FUNCTION img_fits_name (timestamp, integer);

DROP FUNCTION dark_name(integer, integer, timestamp, integer, integer, varchar(8));

DROP FUNCTION ell_update (varchar (150), float4, float4, float4, float4, float4, float4, float8, float4, float4);
