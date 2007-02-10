
typedef struct edt_device
{
  uint_t *fd;			/* file descriptor of opened device     */
  u_int unit_no;
  uint_t devid;
  uint_t size;			/* buffer size              */
  uint_t nbufs;			/* number of multiple buffers       */
  unsigned char **ring_buffers;	/* pointer to mulitple buffer pointers */
  uint_t *ring_tids;		/* for threads */
  unsigned char *tmpbuf;	/* for interlace merging, etc. */
  char devname[64];
  uint_t nextbuf;
  uint_t nextRW;
  uint_t nextwbuf;		/* for edt_next_writebuf      */
  uint_t donecount;
  uint_t ring_buffer_numbufs;
  uint_t ring_buffer_bufsize;
  uint_t ring_buffers_allocated;
  uint_t ring_buffers_configured;
  uint_t ring_buffer_setup_error;
  uint_t write_flag;
  uint_t foi_unit;
  uint_t debug_level;
  uint_t *dd_p;			/* device dependent struct        */
  struct edt_device *edt_p;	/* points to itself, for backwards compat */

  /* For callback rountines */

  uint_t event_funcs[EDT_MAX_KERNEL_EVENTS];

  u_int channel_no;

} PdvDev;
