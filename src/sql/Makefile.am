# $Id$

sqldatadir = ${datadir}/doc/@PACKAGE@/sql

nodist_sqldata_DATA = 	create_functions.sql	\
			create_tables.sql	\
			create_views.sql

dist_sqldata_DATA = 	create_typ_wcs.sql

grant.sql: grant.sql.in
	cat $< | sed -e 's/__WWWUSER__/www-data/g' > $@
