#ifndef __RTS2_APPDB__
#define __RTS2_APPDB__

#include <list>
#include <ostream>

#include "rts2sqlcolumn.h"
#include "../utils/rts2cliapp.h"
#include "../utils/timestamp.h"

// for SQL tables display on console..
//
class Rts2SqlQuery
{
	private:
		std::list < Rts2SqlColumn * >columns;
		char *where;
		char *sql;
		char *from;
		void displayMinusPlusLine ();
	public:
		Rts2SqlQuery (const char *in_from);
		virtual ~ Rts2SqlQuery (void);

		void addColumn (Rts2SqlColumn * add_column);
		void addColumn (const char *in_sql, const char *in_name, int in_order =
			UNORDER);
		void addWhere (const char *in_where);
		char *genSql ();
		void display ();
};

class Rts2AppDb:public Rts2CliApp
{
	private:
		char *connectString;
		char *configFile;

		int initDB ();

	protected:
		virtual int processOption (int in_opt);
		/**
		 * Called for query if the application really needs database access.
		 *
		 * @return true when the application needs database access.
		 */
		virtual bool doInitDB ();
		virtual void afterProcessing ();

	public:
		Rts2AppDb (int argc, char **argv);
		virtual ~ Rts2AppDb (void);

		virtual int init ();

		int parseDate (const char *in_date, double &JD);

		int parseDate (const char *in_date, time_t *out_time);
};
#endif							 /* !__RTS2_APPDB__ */
