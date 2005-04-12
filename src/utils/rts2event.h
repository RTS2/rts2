#ifndef __RTS2_EVENT__
#define __RTS2_EVENT__

// events number below that number shoudl be considered RTS2-reserved
#define RTS2_LOCAL_EVENT   1000

class Rts2Event
{
private:
  int type;
public:
    Rts2Event (int in_type)
  {
    type = in_type;
  }
  int getType ()
  {
    return type;
  }
};

#endif /*! __RTS2_EVENT__ */
