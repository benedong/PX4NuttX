/****************************************************************************
 * drivers/bch/bchdev_unregister.c
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Compilation Switches
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#include <assert.h>
#include <debug.h>

#include <nuttx/fs.h>
#include <nuttx/ioctl.h>

#include "bch_internal.h"

/****************************************************************************
 * Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: bchdev_unregister
 *
 * Description:
 *   Undo the setup performed by losetup
 *
 ****************************************************************************/

int bchdev_unregister(const char *chardev)
{
  FAR struct bchlib_s *bch;
  int fd;
  int ret;

  /* Sanity check */

#ifdef CONFIG_DEBUG
  if (!chardev)
    {
      return -EINVAL;
    }
#endif

  /* Open the character driver associated with chardev */

  fd = open(chardev, O_RDONLY);
  if (ret < 0)
    {
      dbg("Failed to open %s: %d\n", chardev, -ret);
      return ret;
    }

  /* Get a reference to the internal data structure */

  ret = ioctl(fd, DIOC_GETPRIV, (unsigned long)&bch);
  (void)close(fd);

  if (ret < 0)
    {
      dbg("ioctl failed: %d\n", errno);
      return -errno;
    }

  /* Lock out context switches */

  sched_lock();

  /* Check if the internal structure is non-busy (we hold one reference) */

  if (bch->refs > 1)
    {
      ret = -EBUSY;
      goto errout_with_lock;
    }

  /* Unregister the driver (this cannot suspend) */

  ret = unregister_driver(chardev);
  if (ret < 0)
    {
      goto errout_with_lock;
    }
  sched_unlock();

  /* Release the internal structure */

  bch->refs = 0;
  return bchlib_teardown(bch);

errout_with_lock:
  sched_unlock();
  bchlib_decref(bch);
  return ret;
}
