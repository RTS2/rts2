-- creates PLPGSQL language

CREATE FUNCTION plpgsql_call_handler () RETURNS LANGUAGE_HANDLER AS
   '$libdir/plpgsql' LANGUAGE C;

CREATE TRUSTED PROCEDURAL LANGUAGE plpgsql
    HANDLER plpgsql_call_handler;
