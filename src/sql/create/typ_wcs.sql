CREATE FUNCTION wcs_in (opaque) -- OR REPLACE
  RETURNS wcs AS 'pg_wcs.so','wcs_in' LANGUAGE 'c';

CREATE FUNCTION wcs_out (opaque) -- OR REPLACE
  RETURNS opaque AS 'pg_wcs.so', 'wcs_out' LANGUAGE 'c';

-- poprve se vypise varovani, ze typ neni

CREATE TYPE wcs (
  internallength = 220,  -- zjisti si laskave presnou velikost, az to budes mit hotove
  input          = wcs_in,
  output         = wcs_out
);
