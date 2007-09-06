#ifndef __RTS2_OBJECT__
#define __RTS2_OBJECT__

#include "rts2event.h"

/**
 * Base RTS2 class.
 *
 * Its only important method is Rts2Object::postEvent, which works as 
 * event hook for descenadands.
 */
class Rts2Object
{
public:
  /**
   * Default object constructor.
   */
  Rts2Object ()
  {
  }

  /**
   * Default object destructor.
   */
  virtual ~ Rts2Object (void)
  {
  }

  /**
   * Distribute event throught class.
   *
   * This method is hook for descendands to distribute Rts2Event. Every descendand
   * overwriting it should call ancestor postEvent method at the end, so the event object
   * will be deleted.
   *
   * Ussuall implementation looks like:
   *
@code
void
MyObject::postEvent (Rts2Event * event)
{
  // test for event type..
  switch (event->getType ())
  {
    case MY_EVENT_1:
      do_something_with_event_1;
      break;
    case MY_EVENT_2:
      do_something_with_event_2;
      break;
  }
  MyParent::postEvent (event);
}
@endcode
   *
   * @warning { If you do not call parent postEvent method, and you do not delete event, event object
   * will stay in memory indefinitly. Most propably your application will shortly run out of memory and
   * core dump. }
   *
   * @param event Event to process.
   *
   * @see Rts2Event::getType
   */
  virtual void postEvent (Rts2Event * event)
  {
    // discard event..
    delete event;
  }

  /**
   * Called forked instance is run. This method can be used to close connecitns which should not
   * stay open in forked process.
   */
  virtual void forkedInstance ()
  {
  }
};

#endif /*! __RTS2_OBJECT__ */
