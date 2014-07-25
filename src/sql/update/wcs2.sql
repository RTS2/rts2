-- wcs2 type
CREATE FUNCTION wcs2_in (opaque) -- OR REPLACE
  RETURNS wcs2 AS 'pg_wcs2.so','wcs2_in' LANGUAGE 'c';

CREATE FUNCTION wcs2_out (opaque) -- OR REPLACE
  RETURNS opaque AS 'pg_wcs2.so', 'wcs2_out' LANGUAGE 'c';

CREATE TYPE wcs2 (
  internallength = 80,
  input          = wcs2_in,
  output         = wcs2_out
);

-- wcs2 functions
CREATE OR REPLACE FUNCTION isinwcs2 (float8, float8, wcs2)
  RETURNS bool AS 'pg_wcs2.so', 'isinwcs2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION imgrange2 (wcs2)
  RETURNS varchar AS 'pg_wcs2.so', 'imgrange2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_naxis1 (wcs2)
  RETURNS int AS 'pg_wcs2.so', 'img_wcs2_naxis1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_naxis2 (wcs2)
  RETURNS int AS 'pg_wcs2.so', 'img_wcs2_naxis2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs_crpix1 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_crpix1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_crpix2 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_crpix2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_crval1 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_crval1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_crval2 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_crval2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_cd1_1 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_cd1_1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_cd1_2 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_cd1_2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_cd2_1 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_cd2_1' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_cd2_2 (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_cd2_2' LANGUAGE 'c';

CREATE OR REPLACE FUNCTION img_wcs2_equinox (wcs2)
  RETURNS float8 AS 'pg_wcs2.so', 'img_wcs2_equinox' LANGUAGE 'c';
