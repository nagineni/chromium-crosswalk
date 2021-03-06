// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NACL_COMMON_NACL_HELPER_LINUX_H_
#define COMPONENTS_NACL_COMMON_NACL_HELPER_LINUX_H_

// A mini-zygote specifically for Native Client. This file defines
// constants used to implement communication between the nacl_helper
// process and the Chrome zygote.

#define kNaClMaxIPCMessageLength 2048

// Used by Helper to tell Zygote it has started successfully.
#define kNaClHelperStartupAck "NACLHELPER_OK"

enum NaClZygoteIPCCommand {
  kNaClForkRequest,
  kNaClGetTerminationStatusRequest,
};

// The next set of constants define global Linux file descriptors.
// For communications between NaCl loader and browser.
// See also content/common/zygote_main_linux.cc and
// http://code.google.com/p/chromium/wiki/LinuxZygote

// For communications between NaCl loader and zygote.
#define kNaClZygoteDescriptor 3

#endif  // COMPONENTS_NACL_COMMON_NACL_HELPER_LINUX_H_
