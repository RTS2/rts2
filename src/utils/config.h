#ifndef __RTS2_CONFIG__
#define __RTS2_CONFIG__


#define CONFIG_FILE	"/etc/rts2/rts2.conf"

#ifdef __cplusplus
extern "C"
{
#endif

#define CFG_STRING	0
#define CFG_DOUBLE	1

  extern int read_config (const char *filename);
  extern int get_string (const char *name, char **val);
  extern char *get_string_default (const char *name, char *def);
  extern int get_double (const char *name, double *val);
  extern double get_double_default (const char *name, double def);
  extern int get_device_string (const char *device, const char *name,
				char **val);
  extern char *get_device_string_default (const char *device,
					  const char *name, const char *def);
  extern int get_device_double (const char *device, const char *name,
				double *val);
  extern double get_device_double_default (const char *device,
					   const char *name, double def);

  extern int
    get_sub_device_string (const char *device, const char *name,
			   const char *sub, char **val);
  extern char *get_sub_device_string_default (const char *device,
					      const char *name,
					      const char *sub, char *def);
  extern int get_sub_device_double (const char *device, const char *name,
				    const char *sub, double *val);
  extern double get_sub_device_double_default (const char *device,
					       const char *name,
					       const char *sub, double def);
  extern const char *getImageBase (int epoch_id);

#ifdef __cplusplus
};
#endif

#endif /* __RTS2_CONFIG__ */
