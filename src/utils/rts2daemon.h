#ifndef __RTS2_DAEMON__
#define __RTS2_DAEMON__

#include "rts2block.h"
#include "rts2logstream.h"
#include "rts2value.h"
#include "rts2valuelist.h"
#include "rts2valuestat.h"
#include "rts2valueminmax.h"

#include <vector>

/**
 * Abstract class for centrald and all devices.
 *
 * This class contains functions which are common to components with one listening socket.
 *
 * @ingroup RTS2Block
 */
class Rts2Daemon:public Rts2Block
{
private:
  // 0 - don't daemonize, 1 - do daemonize, 2 - is already daemonized, 3 - daemonized & centrald is running, don't print to stdout
  enum
  { DONT_DAEMONIZE, DO_DAEMONIZE, IS_DAEMONIZED, CENTRALD_OK } daemonize;
  int listen_sock;
  void addConnectionSock (int in_sock);
  int lockf;

  // daemon state
  int state;

  Rts2CondValueVector values;
  // values which do not change, they are send only once at connection
  // initialization
  Rts2ValueVector constValues;

  // This vector holds list of values which are currenlty beeing changed
  // It is used to manage BOP mask
  Rts2ValueVector bopValues;

  Rts2ValueTime *info_time;

  time_t idleInfoInterval;
  time_t nextIdleInfo;

  /**
   * Adds value to list of values supported by daemon.
   *
   * @param value Rts2Value which will be added.
   */
  void addValue (Rts2Value * value, int queCondition = 0, bool save_value =
		 false);

  Rts2Value *duplicateValue (Rts2Value * old_value, bool withVal = false);

  /**
   * Perform value changes. Check if value can be changed before performing change.
   * 
   * @return 0 when value change can be performed, -2 on error, -1 when value change is qued.
   */
  int setValue (Rts2CondValue * old_value_cond, char op,
		Rts2Value * new_value);

  /**
   * Holds vector of values which are indendet to be saved. There are two methods, saveValues and loadValues.
   * saveValues is called from setValue(Rts2Value,char,Rts2ValueVector) before first value value is changed. Once values are saved,
   * values_were_saved turns to true and we don't call saveValues before loadValues is called.
   * loadValues reset values_were_saved flag, load all values from savedValues
   */
  Rts2ValueVector savedValues;

  void saveValue (Rts2CondValue * val);
  void deleteSaveValue (Rts2CondValue * val);

  bool doHupIdleLoop;

protected:
  /**
   * Send new value over the wire to all connections.
   */
  void sendValueAll (Rts2Value * value);

  int checkLockFile (const char *lock_fname);
  void setNotDeamonize ()
  {
    daemonize = DONT_DAEMONIZE;
  }
  int doDeamonize ();
  int lockFile ();
  virtual void addSelectSocks (fd_set * read_set);
  virtual void selectSuccess (fd_set * read_set);

  Rts2ValueQueVector queValues;

  Rts2Value *getValue (const char *v_name);
  Rts2CondValue *getCondValue (const char *v_name);

  void loadValues ();

  /**
   * Create new value, and return pointer to it.
   * It also saves value pointer to the internal values list.
   *
   * @param value          Rts2Value which will be created.
   * @param in_val_name    Value name.
   * @param in_description Value description.
   * @param writeToFits    When true, value will be writen to FITS.
   * @param displayType    Value display type, one of the RTS2_DT_xxx constant.
   */
  template < typename T > void createValue (T * &val, char *in_val_name,
					    std::string in_description,
					    bool writeToFits =
					    true, int32_t valueFlags =
					    0, int queCondition =
					    0, bool save_value = false)
  {
    val = new T (in_val_name, in_description, writeToFits, valueFlags);
    addValue (val, queCondition, save_value);
  }
  /**
   * Create new constant value, and return pointer to it.
   *
   * @param value          Rts2Value which will be created
   * @param in_val_name    value name
   * @param in_description value description
   * @param writeToFits    when true, value will be writen to FITS
   */
  template < typename T > void createConstValue (T * &val, char *in_val_name,
						 std::string in_description,
						 bool writeToFits =
						 true, int32_t valueFlags = 0)
  {
    val = new T (in_val_name, in_description, writeToFits, valueFlags);
    addConstValue (val);
  }
  void addConstValue (Rts2Value * value);
  void addConstValue (char *in_name, const char *in_desc, char *in_value);
  void addConstValue (char *in_name, const char *in_desc, double in_value);
  void addConstValue (char *in_name, const char *in_desc, int in_value);
  void addConstValue (char *in_name, char *in_value);
  void addConstValue (char *in_name, double in_value);
  void addConstValue (char *in_name, int in_value);

  /**
   * BOP management routines.
   * Those routines are used to manage BOP (Block OPeration) mask. It is used
   * to decide, which actions can be performed without disturbing observation flow.
   *
   * @param in_value Value which will be added to blocking values.
   *
   * @note This function and @see Rts2Daemon::removeBopValue are called
   * dynamically during execution of value changes when it is discovered, that
   * value cannot be changed due to different state of device then is required.
   */
  void addBopValue (Rts2Value * in_value);

  /**
   * Remove value from BOP list.
   *
   * @see Rts2Daemon::addBopValue
   *
   * @param in_value Value which will be removed.
   *
   * @pre Value was sucessfully updated.
   */
  void removeBopValue (Rts2Value * in_value);

  /**
   * Check if some of the BOP values can be removed from the list.
   */
  void checkBopStatus ();

  /** 
   * Set value. That one should be owerwrited in descendants.
   * 
   * @param  old_value	Old value (pointer), can be directly
   * 	accesed th with pointer stored in object.
   * @param  new_value	New value.
   *
   * @return 1 when value can be set, but it will take longer time to perform,
   * 0 when value can be se imedietly, -1 when value set was qued and -2 on an
   * error.
   */
  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);

  /**
   * Really perform value change.
   *
   * @param old_cond_value
   * @param op Operation string. Permited values depends on value type
   *   (numerical or string), but "=", "+=" and "-=" are ussualy supported.
   * @param new_value 
   */
  int doSetValue (Rts2CondValue * old_cond_value, char op,
		  Rts2Value * new_value);

  /**
   * Returns whenever value change with old_value needs to be qued or
   * not.
   * \param old_value Rts2CondValue object describing the old_value
   */
  virtual bool queValueChange (Rts2CondValue * old_value) = 0;

  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int initValues ();
  virtual int idle ();

public:
  Rts2Daemon (int in_argc, char **in_argv);
  virtual ~ Rts2Daemon (void);
  virtual int run ();

  /**
   * Init daemon.
   * This is call to init daemon. It calls @see Rts2Daemon::init and @see Rts2Daemon::initValues
   * functions to complete daemon initialization.
   */
  void initDaemon ();

  void setIdleInfoInterval (time_t interval)
  {
    idleInfoInterval = interval;
    setTimeoutMin ((long int) interval * USEC_SEC);
  }

  virtual void forkedInstance ();
  virtual void sendMessage (messageType_t in_messageType,
			    const char *in_messageString);
  virtual void centraldConnRunning ();
  virtual void centraldConnBroken ();

  virtual int baseInfo ();
  int baseInfo (Rts2Conn * conn);
  int sendBaseInfo (Rts2Conn * conn);

  virtual int info ();
  int info (Rts2Conn * conn);
  int infoAll ();
  void constInfoAll ();
  int sendInfo (Rts2Conn * conn);
  int sendMetaInfo (Rts2Conn * conn);

  virtual int setValue (Rts2Conn * conn, bool overwriteSaved);

  // state management functions
protected:
  /**
   * Called to set new state value
   */
  void setState (int new_state, const char *description);
  virtual void stateChanged (int new_state, int old_state,
			     const char *description);
public:
  /**
   * Called when state is changed.
   */
  void maskState (int state_mask, int new_state, const char *description =
		  NULL);
  int getState ()
  {
    return state;
  }

  /**
   * Return daemon state without ERROR information.
   */
  int getDaemonState ()
  {
    return state & ~DEVICE_ERROR_MASK;
  }

  /**
   * Called from idle loop after HUP signal occured.
   *
   * This is most probably callback you needed for handling HUP signal.
   * Handling HUP signal when it occurs can be rather dangerous, as it might
   * reloacte memory location - if you read pointer before HUP signal and use
   * it after HUP signal, RTS2 does not guarantee that it will be still valid.
   */
  virtual void signaledHUP ();
  virtual void sigHUP (int sig);
};

#endif /* ! __RTS2_DAEMON__ */
