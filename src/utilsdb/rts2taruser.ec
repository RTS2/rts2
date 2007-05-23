#include "rts2taruser.h"
#include "../utils/rts2block.h"

Rts2UserEvent::Rts2UserEvent (const Rts2UserEvent &in_user)
{
  usr_email = in_user.usr_email;
  event_mask = in_user.event_mask;
}


Rts2UserEvent::Rts2UserEvent (const char *in_usr_email, int in_event_mask)
{
  usr_email = std::string (in_usr_email);
  event_mask = in_event_mask;
}


Rts2UserEvent::~Rts2UserEvent (void)
{
}


bool operator == (Rts2UserEvent _user1, Rts2UserEvent _user2)
{
  return (_user1.usr_email == _user2.usr_email);
}


/**********************************************************************
 *  Rts2TarUser class
 **********************************************************************/

Rts2TarUser::Rts2TarUser (int in_target, char in_type_id)
{
  tar_id = in_target;
  type_id = in_type_id;
}


Rts2TarUser::~Rts2TarUser (void)
{
}


int
Rts2TarUser::load ()
{
  EXEC SQL BEGIN DECLARE SECTION;
    int db_tar_id = tar_id;
    char db_type_id = type_id;

    int db_usr_id;
    VARCHAR db_usr_email[USER_EMAIL_LEN];
    int db_event_mask;
  EXEC SQL END DECLARE SECTION;

  // get Rts2TarUsers from targets_users

  EXEC SQL DECLARE cur_targets_users CURSOR FOR
    SELECT
      users.usr_id,
      usr_email,
      event_mask
    FROM
      users,
      targets_users
    WHERE
      tar_id = :db_tar_id
    AND targets_users.usr_id = users.usr_id;

  EXEC SQL OPEN cur_targets_users;
  while (1)
  {
    EXEC SQL FETCH next FROM cur_targets_users INTO
        :db_usr_id,
        :db_usr_email,
        :db_event_mask;
    if (sqlca.sqlcode)
      break;
    users.push_back (Rts2UserEvent (db_usr_email.arr, db_event_mask));
  }
  if (sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    logStream (MESSAGE_ERROR) << "Rts2TarUsers::load cannot get users " << sqlca.sqlerrm.sqlerrmc << sendLog;
    EXEC SQL CLOSE cur_targets_users;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_targets_users;
  EXEC SQL COMMIT;

  // get Rts2TarUsers from type_users

  EXEC SQL DECLARE cur_type_users CURSOR FOR
    SELECT
      users.usr_id,
      usr_email,
      event_mask
    FROM
      users,
      type_users
    WHERE
      type_id = :db_type_id
    AND type_users.usr_id = users.usr_id;

  EXEC SQL OPEN cur_type_users;
  while (1)
  {
    EXEC SQL FETCH next FROM cur_type_users INTO
        :db_usr_id,
        :db_usr_email,
        :db_event_mask;
    if (sqlca.sqlcode)
      break;
    Rts2UserEvent newUser = Rts2UserEvent (db_usr_email.arr, db_event_mask);
    if (std::find (users.begin (), users.end (), newUser) != users.end ())
    {
      logStream (MESSAGE_ERROR) << "Rts2TarUsers::load user already exists (tar_id " << tar_id << ")" << sendLog;
    }
    else
    {
      users.push_back (Rts2UserEvent (newUser));
    }
  }
  if (sqlca.sqlcode && sqlca.sqlcode != ECPG_NOT_FOUND)
  {
    logStream (MESSAGE_ERROR) << "Rts2TarUsers::load cannot get users '" << sqlca.sqlerrm.sqlerrmc << "'" << sendLog;
    EXEC SQL CLOSE cur_type_users;
    EXEC SQL ROLLBACK;
    return -1;
  }
  EXEC SQL CLOSE cur_type_users;
  EXEC SQL COMMIT;
  return 0;
}


std::string
Rts2TarUser::getUsers (int in_event_mask, int &count)
{
  std::vector <Rts2UserEvent>::iterator user_iter;
  int ret;
  std::string email_list = "";
  if (users.empty ())
  {
    ret = load ();
    if (ret)
      return std::string ("");
  }
  count = 0;
  for (user_iter = users.begin (); user_iter != users.end (); user_iter++)
  {
    if ((*user_iter).haveMask (in_event_mask))
    {
      if (count != 0)
        email_list += ", ";
      email_list += (*user_iter).getUserEmail ();
      count++;
    }
  }
  return email_list;
}
