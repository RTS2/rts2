/*!
 * Pohotometer list in VYPAR (Harmanec and al.) B format
 */

#include <stdio.h>
#include <string.h>

EXEC SQL include sqlca;

void
get_list ()
{
#define test_sql if (sqlca.sqlcode != 0) goto err

  EXEC SQL BEGIN DECLARE SECTION;
      int tar_id;
      int hh;
      int mm;
      int ss;
      int count_value;
      VARCHAR count_filter[3];
  EXEC SQL END DECLARE SECTION;
  EXEC SQL DECLARE targets_counts CURSOR FOR
    SELECT
      tar_id,
      EXTRACT (HOURS FROM count_date),
      EXTRACT (MINUTES FROM count_date),
      EXTRACT (SECONDS FROM count_date),
      count_value,
      count_filter
    FROM
      targets_counts;
      
  EXEC SQL OPEN targets_counts;
  test_sql;
  while (!sqlca.sqlcode)
  {
    EXEC SQL FETCH
      next
    FROM
      targets_counts
    INTO
      :tar_id,
      :hh,
      :mm,
      :ss,
      :count_value,
      :count_filter;
    printf ("%03i %02i %02i %02i %9i %c\n", tar_id, hh, mm, ss, count_value, count_filter.arr[0]);
  }
  EXEC SQL CLOSE targets_counts;
  return;
err:
  printf ("err select_next_alt: %li %s\n", sqlca.sqlcode,
	  sqlca.sqlerrm.sqlerrmc);
  return;
}

int main (int argc, char ** argv)
{
  EXEC SQL CONNECT TO stars;

  get_list ();

  EXEC SQL DISCONNECT all;
  return 0;
}
