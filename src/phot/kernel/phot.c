/***************************************************************************\

    Copyright (c) 2004 Petr Kubánek
    
    All rights reserved.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, and/or sell copies of the Software, and to permit persons
    to whom the Software is furnished to do so, provided that the above
    copyright notice(s) and this permission notice appear in all copies of
    the Software and that both the above copyright notice(s) and this
    permission notice appear in supporting documentation.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
    OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
    HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
    INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
    FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
    NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
    WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

    Except as contained in this notice, the name of a copyright holder
    shall not be used in advertising or otherwise to promote the sale, use
    or other dealings in this Software without prior written authorization
    of the copyright holder.

\***************************************************************************/

#include <linux/version.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <asm/io.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>

#include "phot.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
MODULE_AUTHOR ("Petr Kubánek, petr@lascaux.asu.cas.cz");
MODULE_DESCRIPTION ("Optec Astronomy photometr driver");
MODULE_LICENSE ("GPL");
#endif

#define  PORTS_LEN	8

int base_port = 0x300;

static struct work_struct do_command_que;

MODULE_PARM (base_port, "i");
MODULE_PARM_DESC (base_port, "The base I/O port (default 0x300)");

#define BUF_LEN			128	// circular write buffer len
#define PHOT_COMMAND_LEN	3	// command maximal lenght
#define MAX_WAIT		1000	// maximal wait time - integration
#define intargs(f)	(*(u16*)f)

// command request gets stored to that list
struct command_list_struct
{
  struct command_list_struct *next;
  char command[PHOT_COMMAND_LEN];
};

// device structure
struct device_struct
{
  int base_port;
  u16 buf[BUF_LEN];		// buffer to write data to be read
  int buf_index;		// buffer index
  int buf_last_read;		// buffer last read index
  int filter_position;
  int desired_position;
  u16 integration_time;		// desired integration time
  int status;
  int integration_enabled;	// if it's save to integrate
  struct command_list_struct *command_list;
  int command_pending;
  struct timer_list command_timer;
  struct semaphore sem_list;
  struct semaphore sem;
}
device;				//static, so 0 filled

unsigned int major_number = 254;

#define command_delay() udelay (10)

// this que will be waked up, when some data are available
DECLARE_WAIT_QUEUE_HEAD (operation_wait);


void process_command (struct device_struct *dev);

void
free_command_list (struct device_struct *dev)
{
  struct command_list_struct *next = dev->command_list, *next2;
  while (next)
    {
      next2 = next->next;
      kfree (next);
      next = next2;
    }
  dev->command_list = NULL;
}

void
add_reply (struct device_struct *dev, u16 repl, u16 param)
{
  dev->buf[dev->buf_index] = repl;
  dev->buf_index++;
  dev->buf[dev->buf_index] = param;
  dev->buf_index++;
  dev->buf_index %= BUF_LEN;
  wake_up_interruptible (&operation_wait);
}

void
filter_endmove (struct device_struct *dev)
{
  dev->status &= ~PHOT_S_FILTERMOVE;	// clear move flag
  add_reply (dev, '0', dev->filter_position);
  return;
}

// handles filter movement
void
filter_routine (unsigned long ptr)
{
  struct device_struct *dev = (struct device_struct *) ptr;
  if (dev->status & PHOT_S_INTEGRATION_DIS)
    {
      filter_endmove (dev);
      return;
    }
  if (dev->filter_position < dev->desired_position)
    {
      // move stepper motor one step in direction 1
      outb (3, base_port);
      outb (2, base_port);
      dev->filter_position++;
    }
  else if (dev->filter_position > dev->desired_position)
    {
      outb (1, base_port);
      outb (2, base_port);
      dev->filter_position--;
    }
  else				// filter_position == desired_position
    {
      filter_endmove (dev);
      // process any next command
      dev->command_pending = 0;
      INIT_WORK (&do_command_que, process_command, (void *) dev);
      schedule_work (&do_command_que);
      return;
    }
  dev->status |= PHOT_S_FILTERMOVE;
  init_timer (&dev->command_timer);
  dev->command_timer.expires = jiffies + 2 * HZ / 50;
  dev->command_timer.function = filter_routine;
  add_timer (&dev->command_timer);
}


void
reset (struct device_struct *dev)
{
  printk (KERN_INFO "reset\n");
  // timer 0 to generate a square wave
  outb (54, base_port + 7);
  command_delay ();
  // timer 1 to operate as a digital one shot
  outb (114, base_port + 7);
  command_delay ();
  // timer 2 to operate as an event counter
  outb (176, base_port + 7);
  command_delay ();
  // write divide by 2 to timer 0 to generate 1kHz square wave, LSB and then MSB
  outb (2, base_port + 4);
  command_delay ();
  outb (0, base_port + 4);
  command_delay ();
  // write 1000 to Timer 1 to generate a 1 second pulse, LSB and then MSB
  outb (1000 % 256, base_port + 5);
  command_delay ();
  outb (1000 / 256, base_port + 5);
  // set ouput port bit #0 to 0 bit #1 to 1 and bit #2 to 0
  command_delay ();
  outb (2, base_port + 1);
  device.filter_position = 200;
  device.desired_position = 0;
  // reset stepper motor controller
  outb (0, base_port + 1);
  outb (2, base_port + 1);
  free_command_list (dev);
  // reset any waiting data
  dev->buf_last_read = dev->buf_index = 0;
  // start filter move
  dev->command_pending = 1;
  filter_routine ((unsigned long) dev);
  printk (KERN_INFO "Reset complete\n");
}

void
end_integrate (unsigned long ptr)
{
  struct device_struct *dev = (struct device_struct *) ptr;
  int i;
  int b = 0;
  int frequency = '-';
  if (dev->status & PHOT_S_INTEGRATION_DIS)
    {
      add_reply (dev, '-', '-');
      dev->status &= ~PHOT_S_INTEGRATING_ONCE;
      return;
    }
  for (i = 0; i < MAX_WAIT; i++)
    {
      outb (228, dev->base_port + 7);
      command_delay ();
      b = inb (dev->base_port + 5);
      if ((b & 128) == 128)
	goto got_it;
      command_delay ();
    }
  add_reply (dev, '-', '-');
  goto out;

got_it:
  // check if any pulses were received thus loading timer 2's counter with a valid count
  command_delay ();
  outb (232, dev->base_port + 7);
  command_delay ();
  b = inb (dev->base_port + 6);
  if ((b & 64) == 0)
    {
      frequency = inb (dev->base_port + 6);
      command_delay ();
      frequency += inb (dev->base_port + 6) * 256;
      frequency = 65536 - frequency;
      add_reply (dev, 'A', frequency);
    }
out:
  dev->command_pending = 0;
  dev->status &= ~PHOT_S_INTEGRATING_ONCE;	// clear any once integration
  INIT_WORK (&do_command_que, process_command, (void *) dev);
  schedule_work (&do_command_que);
}

// start integration
void
start_integrate (struct device_struct *dev, int once)
{
  int actper = dev->integration_time * 1000;

  outb (actper % 256, dev->base_port + 5);
  command_delay ();
  outb (actper / 256, dev->base_port + 5);

  // clear timer 2 counter
  outb (255, dev->base_port + 6);
  command_delay ();
  outb (255, dev->base_port + 6);

  // strobe bit 2 of 4-bit control port to initiate counting
  outb (4, dev->base_port);
  command_delay ();
  outb (0, dev->base_port);
  // que timer readout
  init_timer (&dev->command_timer);
  dev->command_timer.expires = jiffies + HZ * dev->integration_time;
  dev->command_timer.function = end_integrate;
  if (once)
    dev->status |= PHOT_S_INTEGRATING_ONCE;
  else
    dev->status |= PHOT_S_INTEGRATING;
  dev->command_pending = 1;
  add_timer (&dev->command_timer);
}

ssize_t
phot_read (struct file *filp, char *buf, size_t count, loff_t * f_pos)
{
  struct device_struct *dev = (struct device_struct *) filp->private_data;
  int len;
  ssize_t ret = 0;
  if ((filp->f_flags & O_NONBLOCK) && (dev->buf_index == dev->buf_last_read))
    return -EAGAIN;
  // wait for data
  wait_event_interruptible (operation_wait,
			    (dev->buf_index != dev->buf_last_read));
  if (down_interruptible (&dev->sem))
    return -EINTR;
  len = (dev->buf_last_read < dev->buf_index) ? dev->buf_index - dev->buf_last_read : BUF_LEN - dev->buf_last_read;	// this one is in sizeof(u16i)
  len *= sizeof (u16);
  if (len > count)
    len = count - count % sizeof (u16);	// round count to sizeof(u16)
  if (copy_to_user (buf, dev->buf + dev->buf_last_read, len))
    {
      ret = -EFAULT;
      goto out;
    }
  *f_pos += len;
  dev->buf_last_read += len / sizeof (u16);
  dev->buf_last_read %= BUF_LEN;
  ret = len;
out:
  up (&dev->sem);
  return ret;
}

void
process_command (struct device_struct *dev)
{
  struct command_list_struct *next;
  down (&dev->sem_list);
  if (!dev->command_list
      || (dev->command_pending
	  && dev->command_list->command[0] != PHOT_CMD_MOVEFILTER))
    goto out;
  switch (dev->command_list->command[0])
    {
    case PHOT_CMD_RESET:
      reset (dev);
      break;
    case PHOT_CMD_INTEGRATE_ONCE:
      dev->integration_time = intargs (&dev->command_list->command[1]);
      start_integrate (dev, 1);
      break;
    case PHOT_CMD_INTEGRATE:
      dev->integration_time = intargs (&dev->command_list->command[1]);
      start_integrate (dev, 0);
      break;
    case PHOT_CMD_STOP_INTEGRATE:
      dev->status &= ~PHOT_S_INTEGRATING;
      // reset any waiting data
      dev->buf_last_read = dev->buf_index = 0;
      INIT_WORK (&do_command_que, process_command, (void *) dev);
      schedule_work (&do_command_que);
      break;
    case PHOT_CMD_MOVEFILTER:
      if (dev->integration_enabled)
	{
	  del_timer_sync (&dev->command_timer);
	  dev->desired_position = intargs (&dev->command_list->command[1]);
	  dev->command_pending = 1;
	  filter_routine ((unsigned long) dev);
	}
      else
	{
	  printk (KERN_ERR "Integration disabled, move filter ignored\n");
	}
      break;
    case PHOT_CMD_INTEGR_ENABLED:
      dev->integration_enabled = intargs (&dev->command_list->command[1]);
      printk (KERN_INFO "Integration %s\n",
	      (dev->integration_enabled ? "enabled" : "disabled"));
      if (!dev->integration_enabled)
	{
	  if (dev->command_pending)
	    {
	      dev->status |= PHOT_S_INTEGRATION_DIS;
	      // cancel any running procedure, write out fake results
	      dev->command_timer.function ((unsigned long) dev);
	      dev->status &= ~PHOT_S_INTEGRATION_DIS;
	    }
	  del_timer_sync (&dev->command_timer);
	  dev->desired_position = 0;
	  dev->command_pending = 1;
	  filter_routine ((unsigned long) dev);
	}
      break;
    default:
      printk (KERN_WARNING "Unknow command '%c' (%x)\n",
	      dev->command_list->command[0], dev->command_list->command[0]);
      break;
    }
  // after completiton
  if (dev->command_list)
    {
      next = dev->command_list->next;
      kfree (dev->command_list);
      dev->command_list = next;
    }
out:
  if ((!dev->command_pending) && (dev->status & PHOT_S_INTEGRATING))
    {
      // if integrating bit is set and no command is pending, start integrating
      start_integrate (dev, 0);
    }
  up (&dev->sem_list);
}

void
append_command (struct device_struct *dev, char command[3])
{
  struct command_list_struct *next;
  down (&dev->sem_list);
  next = dev->command_list;
  // new entry
  if (!next)
    {
      dev->command_list =
	kmalloc (sizeof (struct command_list_struct), GFP_KERNEL);
      next = dev->command_list;
    }
  else
    {
      while (next->next)
	next = next->next;
      next->next = kmalloc (sizeof (struct command_list_struct), GFP_KERNEL);
      next = next->next;
    }
  memcpy (next->command, command, PHOT_COMMAND_LEN);
  next->next = NULL;
  up (&dev->sem_list);
}

ssize_t
phot_write (struct file *filp, const char *buf, size_t count, loff_t * f_pos)
{
  // read only command + parameters (if any)
  struct device_struct *dev = (struct device_struct *) filp->private_data;
  char command[PHOT_COMMAND_LEN];
  int rcount = 0;
  ssize_t ret = 0;
  if (count < PHOT_COMMAND_LEN)	// at least one command must be defined
    return -EFAULT;
  if (down_interruptible (&dev->sem))
    return -EINTR;
  do
    {
      rcount = count > PHOT_COMMAND_LEN ? PHOT_COMMAND_LEN : count;
      if (rcount < PHOT_COMMAND_LEN)
	{
	  ret = -EFAULT;
	  goto out;
	}
      if (copy_from_user (command, buf, rcount))
	{
	  if (!ret)		// ret == 0
	    ret = -EFAULT;
	  goto out;
	}
      append_command (dev, command);
      process_command (dev);
      count -= rcount;
      buf += rcount;
      ret += rcount;
    }
  while (count > 0);
  *f_pos += ret;
out:
  up (&dev->sem);
  return ret;
}

int
phot_open (struct inode *inode, struct file *filp)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
  MOD_INC_USE_COUNT;
#endif
  filp->private_data = &device;
  return 0;
}

int
phot_release (struct inode *inode, struct file *filsp)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
  MOD_DEC_USE_COUNT;
#endif
  return 0;
}

struct file_operations phot_fops = {
read:phot_read,
write:phot_write,
open:phot_open,
release:phot_release
};

int
phot_read_procmem (char *buf, char **start, off_t offset, int count, int *eof,
		   void *data)
{
  struct command_list_struct *next = device.command_list;
  int len;
  len =
    sprintf (buf,
	     "photometer name: Optec Photometer\ncommand_pending: %i\n",
	     device.command_pending);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
  len +=
    sprintf (buf,
	     "sem: %i\nsem_filter: %i\ncommand_pending: %i\n",
	     sem_getcount (&device.sem), sem_getcount (&device.sem_list));
#endif
  len +=
    sprintf (buf + len,
	     "integration_time: %i filter_position: %i desired_position: %i\n",
	     device.integration_time, device.filter_position,
	     device.desired_position);
  down (&device.sem_list);
  while (next)
    {
      len +=
	sprintf (buf + len, "command: %i arg: %i\n", next->command[0],
		 intargs (&next->command[1]));
      next = next->next;
    }
  up (&device.sem_list);
  len += sprintf (buf + len, "status: %i\n", device.status);
  *eof = 1;
  return len;
}

int
init_module (void)
{
  int err;
  device.base_port = base_port;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0))
  if (!request_region (device.base_port, PORTS_LEN, "Optec Photometer"))
    return -EBUSY;		/* device busy */
#else
  err = check_region (device.base_port, PORTS_LEN);
  if (err)
    return err;			/* device busy */
  request_region (device.base_port, PORTS_LEN, "Optec Photometer");
#endif
  device.command_timer.data = (unsigned long) &device;
  sema_init (&device.sem_list, 1);
  sema_init (&device.sem, 1);
  err = register_chrdev (major_number, "phot", &phot_fops);
  if (err < 0)
    {
      release_region (device.base_port, PORTS_LEN);
      return err;
    }
  reset (&device);
  device.integration_enabled = 0;
  create_proc_read_entry ("phot", 0, NULL, phot_read_procmem, NULL);
  printk (KERN_INFO "Module loaded\n");
  printk (KERN_INFO "Integration disabled\n");
  return 0;
}

void
cleanup_module (void)
{
  release_region (device.base_port, PORTS_LEN);
  del_timer_sync (&device.command_timer);
  unregister_chrdev (major_number, "phot");
  free_command_list (&device);
  remove_proc_entry ("phot", NULL);
  printk (KERN_DEBUG "Optec module unloaded\n");
}
