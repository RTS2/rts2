# $Id$

sqldatadir = ${datadir}/doc/@PACKAGE@/sql

dist_sqldata_DATA = 	create_language.sql	\
			create_typ_wcs.sql	\
			create_functions.sql	\
			create_tables.sql	\
			create_views.sql	\
			init_data.sql		\
			grant.sql.in

grant.sql: grant.sql.in
	cat $< | sed -e 's/__WWWUSER__/www-data/g' > $@
