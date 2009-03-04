// Copyright (c) 2009 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#include "chrome/renderer/renderer_webkitclient_impl.h"

#include "WebString.h"
#include "WebURL.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/render_messages.h"
#include "chrome/plugin/npobject_util.h"
#include "chrome/renderer/net/render_dns_master.h"
#include "chrome/renderer/render_thread.h"
#include "webkit/glue/glue_util.h"
#include "webkit/glue/webkit_glue.h"

using WebKit::WebString;
using WebKit::WebURL;

//------------------------------------------------------------------------------

void RendererWebKitClientImpl::setCookies(
    const WebURL& url, const WebURL& policy_url, const WebString& value) {
  RenderThread::current()->Send(
      new ViewHostMsg_SetCookie(url, policy_url, UTF16ToUTF8(value)));
}

WebKit::WebString RendererWebKitClientImpl::cookies(
    const WebKit::WebURL& url, const WebKit::WebURL& policy_url) {
  std::string value;
  RenderThread::current()->Send(
      new ViewHostMsg_GetCookies(url, policy_url, &value));
  return UTF8ToUTF16(value);
}

void RendererWebKitClientImpl::prefetchHostName(
    const WebKit::WebString& hostname) {
  if (!hostname.isEmpty()) {
    std::string hostname_utf8;
    UTF16ToUTF8(hostname.characters(), hostname.length(), &hostname_utf8);
    DnsPrefetchCString(hostname_utf8.data(), hostname_utf8.length());
  }
}

WebKit::WebString RendererWebKitClientImpl::defaultLocale() {
  // TODO(darin): Eliminate this webkit_glue call.
  return WideToUTF16(webkit_glue::GetWebKitLocale());
}

//------------------------------------------------------------------------------

WebString RendererWebKitClientImpl::MimeRegistry::mimeTypeForExtension(
    const WebString& file_extension) {
  if (IsPluginProcess())
    return SimpleWebMimeRegistryImpl::mimeTypeForExtension(file_extension);

  // The sandbox restricts our access to the registry, so we need to proxy
  // these calls over to the browser process.
  std::string mime_type;
  RenderThread::current()->Send(new ViewHostMsg_GetMimeTypeFromExtension(
      webkit_glue::WebStringToFilePathString(file_extension), &mime_type));
  return ASCIIToUTF16(mime_type);
  
}

WebString RendererWebKitClientImpl::MimeRegistry::mimeTypeFromFile(
    const WebString& file_path) {
  if (IsPluginProcess())
    return SimpleWebMimeRegistryImpl::mimeTypeFromFile(file_path);

  // The sandbox restricts our access to the registry, so we need to proxy
  // these calls over to the browser process.
  std::string mime_type;
  RenderThread::current()->Send(new ViewHostMsg_GetMimeTypeFromFile(
      FilePath(webkit_glue::WebStringToFilePathString(file_path)),
      &mime_type));
  return ASCIIToUTF16(mime_type);
  
}

WebString RendererWebKitClientImpl::MimeRegistry::preferredExtensionForMIMEType(
    const WebString& mime_type) {
  if (IsPluginProcess())
    return SimpleWebMimeRegistryImpl::preferredExtensionForMIMEType(mime_type);

  // The sandbox restricts our access to the registry, so we need to proxy
  // these calls over to the browser process.
  FilePath::StringType file_extension;
  RenderThread::current()->Send(
      new ViewHostMsg_GetPreferredExtensionForMimeType(UTF16ToASCII(mime_type),
          &file_extension));
  return webkit_glue::FilePathStringToWebString(file_extension);
}
