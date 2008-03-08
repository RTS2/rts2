#ifndef __RTS2_SQL_COLUMN__
#define __RTS2_SQL_COLUMN__

#include <ostream>

#define ORDER_DESC  -1
#define UNORDER   0
#define ORDER_ASC 1

class Rts2SqlColumn
{
	private:
		char *sql;
		char *name;
		int order;
		int fieldSize;
	public:
		Rts2SqlColumn (const char *in_sql, const char *in_name, int in_order =
			UNORDER);
		virtual ~ Rts2SqlColumn (void);
		char *genSql ()
		{
			return sql;
		}
		const char *getName ()
		{
			return name;
		}
		const char *getHeader (int in_fieldSize)
		{
			fieldSize = in_fieldSize;
			return name;
		}
		int getOrderBy ()
		{
			return order;
		}
		int getFieldSize ()
		{
			return fieldSize;
		}
		// value should be casted to right type
		virtual void processValue (void *val, int sqlType, int isNull);
		virtual void printStatistics (std::ostream & os);
};

// for sorting..
bool operator < (Rts2SqlColumn lhs, Rts2SqlColumn rhs);

class Rts2SqlColumnObsState:public Rts2SqlColumn
{
	unsigned int interupted;
	unsigned int acq_failed;
	unsigned int ok;
	public:
		Rts2SqlColumnObsState (const char *in_sql, const char *in_name,
			int in_order = UNORDER);
		virtual void processValue (void *val, int sqlType, int isNull);
		virtual void printStatistics (std::ostream & os);
};
#endif							 /* !__RTS2_SQL_COLUMN__ */
