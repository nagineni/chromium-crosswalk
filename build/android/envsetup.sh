#!/bin/bash
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Sets up environment for building Chromium on Android.  It can either be
# compiled with the Android tree or using the Android SDK/NDK. To build with
# NDK/SDK: ". build/android/envsetup.sh".  Environment variable
# ANDROID_SDK_BUILD=1 will then be defined and used in the rest of the setup to
# specifiy build type.

# Make sure we're being sourced (possibly by another script). Check for bash
# since zsh sets $0 when sourcing.
if [[ -n "$BASH_VERSION" && "${BASH_SOURCE:-$0}" == "$0" ]]; then
  echo "ERROR: envsetup must be sourced."
  exit 1
fi

# Source functions script.  The file is in the same directory as this script.
SCRIPT_DIR="$(dirname "${BASH_SOURCE:-$0}")"
. "${SCRIPT_DIR}"/envsetup_functions.sh

export ANDROID_SDK_BUILD=1  # Default to SDK build.

process_options "$@"

# When building WebView as part of Android we can't use the SDK. Other builds
# default to using the SDK.
if [[ "${CHROME_ANDROID_BUILD_WEBVIEW}" -eq 1 ]]; then
  export ANDROID_SDK_BUILD=0
fi

if [[ "${ANDROID_SDK_BUILD}" -ne 1 ]]; then
  echo "Initializing for non-SDK build."
fi

# Get host architecture, and abort if it is 32-bit, unless --try-32
# is also used.
host_arch=$(uname -m)
case "${host_arch}" in
  x86_64)  # pass
    ;;
  i?86)
    if [[ -z "${try_32bit_host_build}" ]]; then
      echo "ERROR: Android build requires a 64-bit host build machine."
      echo "If you really want to try it on this machine, use the \
--try-32bit-host flag."
      echo "Be warned that this may fail horribly at link time, due \
very large binaries."
      return 1
    else
      echo "WARNING: 32-bit host build enabled. Here be dragons!"
      host_arch=x86
    fi
    ;;
  *)
    echo "ERROR: Unsupported host architecture (${host_arch})."
    echo "Try running this script on a Linux/x86_64 machine instead."
    return 1
esac

case "${host_os}" in
  "linux")
    toolchain_dir="linux-${host_arch}"
    ;;
  "mac")
    toolchain_dir="darwin-${host_arch}"
    ;;
  *)
    echo "Host platform ${host_os} is not supported" >& 2
    return 1
esac

CURRENT_DIR="$(readlink -f "${SCRIPT_DIR}/../../")"
if [[ -z "${CHROME_SRC}" ]]; then
  # If $CHROME_SRC was not set, assume current directory is CHROME_SRC.
  export CHROME_SRC="${CURRENT_DIR}"
fi

if [[ "${CURRENT_DIR/"${CHROME_SRC}"/}" == "${CURRENT_DIR}" ]]; then
  # If current directory is not in $CHROME_SRC, it might be set for other
  # source tree. If $CHROME_SRC was set correctly and we are in the correct
  # directory, "${CURRENT_DIR/"${CHROME_SRC}"/}" will be "".
  # Otherwise, it will equal to "${CURRENT_DIR}"
  echo "Warning: Current directory is out of CHROME_SRC, it may not be \
the one you want."
  echo "${CHROME_SRC}"
fi

if [[ "${ANDROID_SDK_BUILD}" -eq 1 ]]; then
  if [[ -z "${TARGET_ARCH}" ]]; then
    return 1
  fi
  sdk_build_init
# Sets up environment for building Chromium for Android with source. Expects
# android environment setup and lunch.
elif [[ -z "$ANDROID_BUILD_TOP" || \
        -z "$ANDROID_TOOLCHAIN" || \
        -z "$ANDROID_PRODUCT_OUT" ]]; then
  echo "Android build environment variables must be set."
  echo "Please cd to the root of your Android tree and do: "
  echo "  . build/envsetup.sh"
  echo "  lunch"
  echo "Then try this again."
  echo "Or did you mean NDK/SDK build. Run envsetup.sh without any arguments."
  return 1
elif [[ -n "$CHROME_ANDROID_BUILD_WEBVIEW" ]]; then
  webview_build_init
fi

# Workaround for valgrind build
if [[ -n "$CHROME_ANDROID_VALGRIND_BUILD" ]]; then
# arm_thumb=0 is a workaround for https://bugs.kde.org/show_bug.cgi?id=270709
  DEFINES+=" arm_thumb=0 release_extra_cflags='-fno-inline\
 -fno-omit-frame-pointer -fno-builtin' release_valgrind_build=1\
 release_optimize=1"
fi

# Source a bunch of helper functions
. ${CHROME_SRC}/build/android/adb_device_functions.sh

ANDROID_GOMA_WRAPPER=""
if [[ -d $GOMA_DIR ]]; then
  ANDROID_GOMA_WRAPPER="$GOMA_DIR/gomacc"
  num_cores="$(grep --count ^processor /proc/cpuinfo)"
# Goma is IO-ish you want more threads than you have cores.
  let "goma_threads=num_cores*2"
  if [ -z "${GOMA_COMPILER_PROXY_THREADS}" -a "${goma_threads}" -gt 16 ]; then
# The default is 16 threads, if the machine has many cores we crank it up a bit
    GOMA_COMPILER_PROXY_THREADS="${goma_threads}"
    export GOMA_COMPILER_PROXY_THREADS
  fi
fi
export ANDROID_GOMA_WRAPPER

# Declare Android are cross compile.
export GYP_CROSSCOMPILE=1

# Performs a gyp_chromium run to convert gyp->Makefile for android code.
android_gyp() {
  # This is just a simple wrapper of gyp_chromium, please don't add anything
  # in this function.
  echo "GYP_GENERATORS set to '$GYP_GENERATORS'"
  (
    "${CHROME_SRC}/build/gyp_chromium" --depth="${CHROME_SRC}" --check "$@"
  )
}

# FLOCK needs to be null on system that has no flock
which flock > /dev/null || export FLOCK=
