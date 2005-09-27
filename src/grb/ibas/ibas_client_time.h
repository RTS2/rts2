#define	IBC_DAYS_1_1_72_FROM_EPOCH	(5113)
#define	IBC_LEAP_SEC_TABLE_SIZE		(100)
#define	IBC_LEAP_SEC_END_MARKER		(999999999)


typedef struct IBC_UTC_TIME_STRUCT
{
  int year;			/* 1972 - 2000(+) */
  int month;			/* 0 - 11 */
  int day;			/* 0 - 28/29/30/31 */
  int hour;			/* 0 - 23 */
  int min;			/* 0 - 59 */
  int sec;			/* 0 - 59/60/61 */
  int usec;			/* 0 - 999999 */
}
IBC_UTC_TIME;

typedef struct IBC_ERT_TIME_STRUCT
{
  int day;			/* days from 1.1.58 */
  int msec;			/* 0 - 86400999, millisec of a day */
  int usec;			/* 0 - 999 */
}
IBC_ERT_TIME;

typedef struct IBC_LEAP_SEC_ENTRY_STRUCT
{
  int year;
  int month;
  int delta;
}
IBC_LEAP_SEC_ENTRY;



extern int ibc_utc_year_min;
extern int ibc_utc_month_min;
extern int ibc_utc_year_max;
extern int ibc_utc_month_max;
extern int ibc_month_days[12];
extern int ibc_month_days_leap[12];
extern IBC_LEAP_SEC_ENTRY ibc_tai_leap_sec[IBC_LEAP_SEC_TABLE_SIZE];


int ibc_compare_ym_pair (int y1, int m1, int y2, int m2);
int ibc_get_feb_days (int year);
int ibc_utc2tai2ert (IBC_UTC_TIME * utc, double *tai, IBC_ERT_TIME * ert);
int ibc_ert2utc (IBC_ERT_TIME * ert, IBC_UTC_TIME * utc);
int ibc_leap_entry2tai (IBC_LEAP_SEC_ENTRY * lse, double *tai);
int ibc_tai2utc (double tai, IBC_UTC_TIME * utc);
int ibc_current_utc (IBC_UTC_TIME * utc);	/* days from 01.Jan.1958 plus millisec of day */
int ibc_utc2str24 (IBC_UTC_TIME utc, char *str_utc);
int ibc_str242utc (char *str_utc, IBC_UTC_TIME * utc);
