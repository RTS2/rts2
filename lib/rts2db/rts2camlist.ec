#include "rts2camlist.h"

#include "app.h"

Rts2CamList::Rts2CamList ()
{
}

Rts2CamList::~Rts2CamList ()
{
}

int Rts2CamList::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
    VARCHAR d_camera_name[8];
  EXEC SQL END DECLARE SECTION;

  EXEC SQL DECLARE cur_cams CURSOR FOR
    SELECT
      camera_name
    FROM
      cameras;
  EXEC SQL OPEN cur_cams;

  if (sqlca.sqlcode)
  {
    logStream (MESSAGE_ERROR)
      << "Cannot load camera list: "
      << sqlca.sqlcode
      << " (" << sqlca.sqlerrm.sqlerrmc << ")"
      << sendLog;
    return -1;
  }
  while (1)
  {
    EXEC SQL FETCH NEXT
      FROM
        cur_cams
      INTO
        :d_camera_name;
    if (sqlca.sqlcode)
      break;
    push_back (std::string (d_camera_name.arr));
  }
  EXEC SQL CLOSE cur_cams;
  return 0;
}
