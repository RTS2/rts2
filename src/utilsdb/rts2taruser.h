#ifndef __RTS2_TARUSER__
#define __RTS2_TARUSER__

#include <ostream>
#include <vector>

#define USER_EMAIL_LEN		200

class Target;

/**
 * Holds informations about target - users relation,
 * which is used to describe which event should be mailed
 * to which user.
 *
 * User can be entered also in type_user table.
 *
 * @author petr
 */
class Rts2UserEvent
{
private:
  std::string usr_email;
  int event_mask;
public:
  // copy constructor
    Rts2UserEvent (const Rts2UserEvent & in_user);
    Rts2UserEvent (const char *in_usr_email, int in_event_mask);
    virtual ~ Rts2UserEvent (void);

  bool haveMask (int in_mask)
  {
    return (in_mask & event_mask);
  }
  std::string & getUserEmail ()
  {
    return usr_email;
  }

  friend bool operator == (Rts2UserEvent _user1, Rts2UserEvent _user2);
};

bool operator == (Rts2UserEvent _user1, Rts2UserEvent _user2);

/**
 * Holds set of Rts2UserEvent relations.
 *
 * Provides method to use set of Rts2UserEvent relations to distribute events.
 */
class Rts2TarUser
{
private:
  //! users which belongs to event
  std::vector < Rts2UserEvent > users;
  int tar_id;
  char type_id;
public:
    Rts2TarUser (int in_target, char in_type_id);
    virtual ~ Rts2TarUser (void);

  int load ();
  // returns users for given event
    std::string getUsers (int in_event_mask, int &count);
};

#endif /* !__RTS2_TARUSER__ */
