#include "rts2messagedb.h"
#include "../utils/rts2block.h"

#include <iostream>

EXEC SQL include sqlca;

Rts2MessageDB::Rts2MessageDB (const struct timeval &in_messageTime, std::string in_messageOName, messageType_t in_messageType, std::string in_messageString): Rts2Message (in_messageTime, in_messageOName, in_messageType, in_messageString)
{
}


Rts2MessageDB::~Rts2MessageDB (void)
{
}


void
Rts2MessageDB::insertDB ()
{
  EXEC SQL BEGIN DECLARE SECTION;
    double d_message_time = messageTime.tv_sec + (double) messageTime.tv_usec / USEC_SEC;
    varchar d_message_oname[8];
    int d_message_type = messageType;
    varchar d_message_string[200];
  EXEC SQL END DECLARE SECTION;

  strncpy (d_message_oname.arr, messageOName.c_str(), 8);
  d_message_oname.len = strlen (d_message_oname.arr);
  if (d_message_oname.len > 8)
    d_message_oname.len = 8;

  strncpy (d_message_string.arr, messageString.c_str(), 200);
  d_message_string.len = strlen (d_message_string.arr);
  if (d_message_string.len > 200)
    d_message_string.len = 200;

  EXEC SQL INSERT
    INTO
      message
      (
      message_time,
      message_oname,
      message_type,
      message_string
      )
    VALUES
      (
      to_timestamp (:d_message_time),
      :d_message_oname,
      :d_message_type,
      :d_message_string
      );
  if (sqlca.sqlcode)
  {
    std::cerr << "Error writing to DB: " << sqlca.sqlerrm.sqlerrmc << " " << sqlca.sqlcode << std::endl;
    EXEC SQL ROLLBACK;
    return;
  }
  EXEC SQL COMMIT;
}
