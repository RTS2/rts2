-- creates PLPGSQL language
-- $Id: language.sql 1934 2005-09-06 09:16:26Z petr $

CREATE FUNCTION plpgsql_call_handler () RETURNS LANGUAGE_HANDLER AS
   '$libdir/plpgsql' LANGUAGE C;

CREATE TRUSTED PROCEDURAL LANGUAGE plpgsql
    HANDLER plpgsql_call_handler;
