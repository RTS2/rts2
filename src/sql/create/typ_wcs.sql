CREATE TYPE wcs;

CREATE FUNCTION wcs_in (cstring) -- OR REPLACE
  RETURNS wcs AS 'pg_wcs.so','wcs_in' LANGUAGE 'c';

CREATE FUNCTION wcs_out (wcs) -- OR REPLACE
  RETURNS cstring AS 'pg_wcs.so', 'wcs_out' LANGUAGE 'c';

CREATE TYPE wcs (
  internallength = 220,  -- zjisti si laskave presnou velikost, az to budes mit hotove
  input          = wcs_in,
  output         = wcs_out
);
