/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/

/*
 * This file is 'mem-include-scan' clean, which means memdebug.h and
 * curl_memory.h are purposely not included in this file. See test 1132.
 *
 * The functions in this file are curlx functions which are not tracked by the
 * curl memory tracker memdebug.
 */

#include "curl_setup.h"

#if defined(_WIN32)

#include "curl_multibyte.h"

/*
 * MultiByte conversions using Windows kernel32 library.
 */

wchar_t *curlx_convert_UTF8_to_wchar(const char *str_utf8)
{
  wchar_t *str_w = NULL;

  if(str_utf8) {
    int str_w_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        str_utf8, -1, NULL, 0);
    if(str_w_len > 0) {
      str_w = malloc(str_w_len * sizeof(wchar_t));
      if(str_w) {
        if(MultiByteToWideChar(CP_UTF8, 0, str_utf8, -1, str_w,
                               str_w_len) == 0) {
          free(str_w);
          return NULL;
        }
      }
    }
  }

  return str_w;
}

char *curlx_convert_wchar_to_UTF8(const wchar_t *str_w)
{
  char *str_utf8 = NULL;

  if(str_w) {
    int bytes = WideCharToMultiByte(CP_UTF8, 0, str_w, -1,
                                    NULL, 0, NULL, NULL);
    if(bytes > 0) {
      str_utf8 = malloc(bytes);
      if(str_utf8) {
        if(WideCharToMultiByte(CP_UTF8, 0, str_w, -1, str_utf8, bytes,
                               NULL, NULL) == 0) {
          free(str_utf8);
          return NULL;
        }
      }
    }
  }

  return str_utf8;
}

#endif /* _WIN32 */

#if defined(USE_WIN32_LARGE_FILES) || defined(USE_WIN32_SMALL_FILES)


/* Fix excessive paths (paths that exceed MAX_PATH length of 260).
 *
 * This is a helper function to fix paths that would exceed the MAX_PATH
 * limitation check done by Windows APIs. It does so by normalizing the passed
 * in filename or path 'in' to its full canonical path, and if that path is
 * longer than MAX_PATH then setting 'out' to "\\?\" prefix + that full path.
 *
 * For example 'in' filename255chars in current directory C:\foo\bar is
 * fixed as \\?\C:\foo\bar\filename255chars for 'out' which will tell Windows
 * it's ok to access that filename even though the actual full path is longer
 * than 255 chars.
 *
 * For non-Unicode builds this function may fail sometimes because only the
 * Unicode versions of some Windows API functions can access paths longer than
 * MAX_PATH, for example GetFullPathNameW which is used in this function. When
 * the full path is then converted from Unicode to multibyte that fails if any
 * directories in the path contain characters not in the current codepage.
 */
static bool fix_excessive_path(const TCHAR *in, TCHAR **out)
{
  size_t needed, written;
  const wchar_t *in_w;
  wchar_t *fbuf = NULL;

#ifndef _UNICODE
  wchar_t *ibuf = NULL;
  char *obuf = NULL;
#endif

  *out = NULL;

#ifndef _UNICODE
  /* convert multibyte input to unicode */
  needed = mbstowcs(NULL, in, 0);
  if(needed == (size_t)-1 || needed >= (32767 - 4))
    goto error;
  ++needed; /* for NUL */
  ibuf = malloc(needed * sizeof(wchar_t));
  if(!ibuf)
    goto error;
  written = mbstowcs(ibuf, in, needed);
  if(written == (size_t)-1 || written >= needed)
    goto error;
  in_w = ibuf;
#else
  in_w = in;
#endif

  /* get full unicode path of the unicode filename or path */
  needed = (size_t)GetFullPathNameW(in_w, 0, NULL, NULL);
  if(!needed || needed > (32767 - 4))
    goto error;
  /* ignore paths which are not excessive and don't need modification */
  if(needed <= MAX_PATH)
    goto error;
  fbuf = malloc((needed + 4)* sizeof(wchar_t));
  if(!fbuf)
    goto error;
  wcsncpy(fbuf, L"\\\\?\\", 4);
  written = (size_t)GetFullPathNameW(in_w, needed, fbuf + 4, NULL);
  if(!written || written >= needed)
    goto error;

#ifndef _UNICODE
  /* convert unicode full path to multibyte output */
  needed = wcstombs(NULL, fbuf, 0);
  if(needed == (size_t)-1 || needed >= 32767)
    goto error;
  ++needed; /* for NUL */
  obuf = malloc(needed);
  if(!obuf)
    goto error;
  written = wcstombs(obuf, fbuf, needed);
  if(written == (size_t)-1 || written >= needed)
    goto error;
  *out = obuf;
  obuf = NULL;
#else
  *out = fbuf;
  fbuf = NULL;
#endif

error:
  free(fbuf);
#ifndef _UNICODE
  free(ibuf);
  free(obuf);
#endif
  return (*out ? true : false);
}

int curlx_win32_open(const char *filename, int oflag, ...)
{
  int pmode = 0;
  int result = -1;
  TCHAR *fixed = NULL;
  const TCHAR *target = NULL;

#ifdef _UNICODE
  wchar_t *filename_w = curlx_convert_UTF8_to_wchar(filename);
#endif

  va_list param;
  va_start(param, oflag);
  if(oflag & O_CREAT)
    pmode = va_arg(param, int);
  va_end(param);

#ifdef _UNICODE
  if(filename_w) {
    if(fix_excessive_path(filename_w, &fixed))
      target = fixed;
    else
      target = filename_w;
    result = _wopen(target, oflag, pmode);
    curlx_unicodefree(filename_w);
  }
  else
    errno = EINVAL;
#else
  if(fix_excessive_path(filename, &fixed))
    target = fixed;
  else
    target = filename;
  result = (_open)(target, oflag, pmode);
#endif

  free(fixed);
  return result;
}

FILE *curlx_win32_fopen(const char *filename, const char *mode)
{
  FILE *result = NULL;
  TCHAR *fixed = NULL;
  const TCHAR *target = NULL;

#ifdef _UNICODE
  wchar_t *filename_w = curlx_convert_UTF8_to_wchar(filename);
  wchar_t *mode_w = curlx_convert_UTF8_to_wchar(mode);
  if(filename_w && mode_w) {
    if(fix_excessive_path(filename_w, &fixed))
      target = fixed;
    else
      target = filename_w;
    result = _wfopen(target, mode_w);
  }
  else
    errno = EINVAL;
  curlx_unicodefree(filename_w);
  curlx_unicodefree(mode_w);
#else
  if(fix_excessive_path(filename, &fixed))
    target = fixed;
  else
    target = filename;
  result = (fopen)(target, mode);
#endif

  free(fixed);
  return result;
}

int curlx_win32_stat(const char *path, struct_stat *buffer)
{
  int result = -1;
  TCHAR *fixed = NULL;
  const TCHAR *target = NULL;

#ifdef _UNICODE
  wchar_t *path_w = curlx_convert_UTF8_to_wchar(path);
  if(path_w) {
    if(fix_excessive_path(path_w, &fixed))
      target = fixed;
    else
      target = path_w;
#if defined(USE_WIN32_SMALL_FILES)
    result = _wstat(target, buffer);
#else
    result = _wstati64(target, buffer);
#endif
    curlx_unicodefree(path_w);
  }
  else
    errno = EINVAL;
#else
  if(fix_excessive_path(path, &fixed))
    target = fixed;
  else
    target = path;
#if defined(USE_WIN32_SMALL_FILES)
  result = _stat(target, buffer);
#else
  result = _stati64(target, buffer);
#endif
#endif

  free(fixed);
  return result;
}

int curlx_win32_access(const char *path, int mode)
{
  int result = -1;
  TCHAR *fixed = NULL;
  const TCHAR *target = NULL;

#if defined(_UNICODE)
  wchar_t *path_w = curlx_convert_UTF8_to_wchar(path);
  if(path_w) {
    if(fix_excessive_path(path_w, &fixed))
      target = fixed;
    else
      target = path_w;
    result = _waccess(target, mode);
    curlx_unicodefree(path_w);
  }
  else
    errno = EINVAL;
#else
  if(fix_excessive_path(path, &fixed))
    target = fixed;
  else
    target = path;
  result = _access(target, mode);
#endif

  free(fixed);
  return result;
}

#endif /* USE_WIN32_LARGE_FILES || USE_WIN32_SMALL_FILES */
