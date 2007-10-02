#ifndef __RTS2_CONFIG_RAW__
#define __RTS2_CONFIG_RAW__
/**
 * Class which enable access to configuration values, stored in ini file.
 *
 * @author petr
 */

#include <fstream>
#include <list>
#include <stdio.h>
#include <string>
#include <vector>

#define SEP	" "

/**
 * This class represent section value. Section value have name, possible sufix
 * which is separated from name by ".", and value, which is after value name
 * and suffix followed by "=" sign.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConfigValue
{
private:
  std::string valueName;
  std::string valueSuffix;
  std::string value;
public:
  Rts2ConfigValue (std::string in_valueName, std::string in_valueSuffix,
		   std::string in_value)
  {
    valueName = in_valueName;
    valueSuffix = in_valueSuffix;
    value = in_value;
  }

  bool isValue (std::string name)
  {
    return (valueName == name && valueSuffix.length () == 0);
  }

  std::string getValueName ()
  {
    return valueName;
  }

  std::string getSuffix ()
  {
    return valueSuffix;
  }

  std::string getValue ()
  {
    return value;
  }

  double getValueDouble ()
  {
    return atof (value.c_str ());
  }

  friend std::ostream & operator << (std::ostream & _os, Rts2ConfigValue val);
};


std::ostream & operator << (std::ostream & _os, Rts2ConfigValue val);

/**
 * This class represent section of configuration file.
 * Section start with [section_name] and ends with next section.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ConfigSection:public
  std::list <
  Rts2ConfigValue >
{
private:
  std::string
    sectName;
public:
  Rts2ConfigSection (const char *name);
  bool
  isSection (std::string name)
  {
    return sectName == name;
  }
  std::string
  getName ()
  {
    return sectName;
  }
  /**
   * Return NULL if value cannot be found.
   */
  Rts2ConfigValue *
  getValue (const char *param);
};

/**
 * This class represent whole config file. It have methods to reload config
 * file, get list of sections, and access sections.
 *
 * It is raw as it does not contain any call to Libnova function. For full
 * config, which depends on Libnova functions and provides ObjectChecker, 
 * \see Rts2Config.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

class
  Rts2ConfigRaw:
  public
  std::vector <
Rts2ConfigSection * >
{
private:
  void
  clearSections ();
  int
  parseConfigFile ();

  Rts2ConfigSection *
  getSection (const char *section);
  Rts2ConfigValue *
  getValue (const char *section, const char *param);
protected:
  std::ifstream *
    configStream;
  virtual void
  getSpecialValues ()
  {
  }
public:
  Rts2ConfigRaw ();
  virtual ~ Rts2ConfigRaw (void);
  int
  loadFile (char *filename = NULL);
  int
  getString (const char *section, const char *param, std::string & buf);
  int
  getInteger (const char *section, const char *param, int &value);
  int
  getFloat (const char *section, const char *param, float &value);
  double
  getDouble (const char *section, const char *param);
  int
  getDouble (const char *section, const char *param, double &value);
  bool
  getBoolean (const char *section, const char *param, bool def = false);
};

#endif /*! __RTS2_CONFIG_RAW__ */
