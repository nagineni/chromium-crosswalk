// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(WIN32) || defined(__linux__)

#include <errno.h>

#include "nacl_io/kernel_wrap.h"
#include "nacl_io/kernel_wrap_real.h"

// "real" functions, i.e. the unwrapped original functions. For Windows/Linux
// host builds we don't wrap, so the real functions aren't accessible. In most
// cases, we just fail.

int _real_close(int fd) {
  return ENOSYS;
}

int _real_fstat(int fd, struct stat *buf) {
  return 0;
}

int _real_getdents(int fd, void* nacl_buf, size_t nacl_count, size_t *nread) {
  return ENOSYS;
}

int _real_lseek(int fd, off_t offset, int whence, off_t* new_offset) {
  return ENOSYS;
}

int _real_mkdir(const char* pathname, mode_t mode) {
  return ENOSYS;
}

int _real_mmap(void** addr, size_t length, int prot, int flags, int fd,
               off_t offset) {
  return ENOSYS;
}

int _real_munmap(void* addr, size_t length) {
  return ENOSYS;
}

int _real_open(const char* pathname, int oflag, mode_t cmode, int* newfd) {
  return ENOSYS;
}

int _real_open_resource(const char* file, int* fd) {
  return ENOSYS;
}

int _real_read(int fd, void *buf, size_t count, size_t *nread) {
  *nread = count;
  return 0;
}

int _real_rmdir(const char* pathname) {
  return ENOSYS;
}

int _real_write(int fd, const void *buf, size_t count, size_t *nwrote) {
  int rtn = write(fd, buf, count);
  if (rtn < 0)
    return -1;

  *nwrote = rtn;
  return 0;
}

#endif

#if defined(__linux__)

void kernel_wrap_init() {
}

void kernel_wrap_uninit() {
}

#endif
