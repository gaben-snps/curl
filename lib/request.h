#ifndef HEADER_CURL_REQUEST_H
#define HEADER_CURL_REQUEST_H
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

/* This file is for lib internal stuff */

#include "curl_setup.h"

#include "bufq.h"

/* forward declarations */
struct UserDefined;

enum expect100 {
  EXP100_SEND_DATA,           /* enough waiting, just send the body now */
  EXP100_AWAITING_CONTINUE,   /* waiting for the 100 Continue header */
  EXP100_SENDING_REQUEST,     /* still sending the request but will wait for
                                 the 100 header once done with the request */
  EXP100_FAILED               /* used on 417 Expectation Failed */
};

enum upgrade101 {
  UPGR101_INIT,               /* default state */
  UPGR101_WS,                 /* upgrade to WebSockets requested */
  UPGR101_H2,                 /* upgrade to HTTP/2 requested */
  UPGR101_RECEIVED,           /* 101 response received */
  UPGR101_WORKING             /* talking upgraded protocol */
};


/*
 * Request specific data in the easy handle (Curl_easy).  Previously,
 * these members were on the connectdata struct but since a conn struct may
 * now be shared between different Curl_easys, we store connection-specific
 * data here. This struct only keeps stuff that's interesting for *this*
 * request, as it will be cleared between multiple ones
 */
struct SingleRequest {
  curl_off_t size;        /* -1 if unknown at this point */
  curl_off_t maxdownload; /* in bytes, the maximum amount of data to fetch,
                             -1 means unlimited */
  curl_off_t bytecount;         /* total number of bytes read */
  curl_off_t writebytecount;    /* number of bytes written */

  curl_off_t pendingheader;      /* this many bytes left to send is actually
                                    header and not body */
  struct curltime start;         /* transfer started at this time */
  unsigned int headerbytecount;  /* received server headers (not CONNECT
                                    headers) */
  unsigned int allheadercount;   /* all received headers (server + CONNECT) */
  unsigned int deductheadercount; /* this amount of bytes doesn't count when
                                     we check if anything has been transferred
                                     at the end of a connection. We use this
                                     counter to make only a 100 reply (without
                                     a following second response code) result
                                     in a CURLE_GOT_NOTHING error code */
  int headerline;               /* counts header lines to better track the
                                   first one */
  curl_off_t offset;            /* possible resume offset read from the
                                   Content-Range: header */
  int httpcode;                 /* error code from the 'HTTP/1.? XXX' or
                                   'RTSP/1.? XXX' line */
  int keepon;
  struct curltime start100;      /* time stamp to wait for the 100 code from */
  enum expect100 exp100;        /* expect 100 continue state */
  enum upgrade101 upgr101;      /* 101 upgrade state */

  /* Client Writer stack, handles trasnfer- and content-encodings, protocol
   * checks, pausing by client callbacks. */
  struct Curl_cwriter *writer_stack;
  struct bufq sendbuf; /* data which needs to be send to the server */
  time_t timeofdoc;
  long bodywrites;
  char *location;   /* This points to an allocated version of the Location:
                       header data */
  char *newurl;     /* Set to the new URL to use when a redirect or a retry is
                       wanted */

  /* 'upload_present' is used to keep a byte counter of how much data there is
     still left in the buffer, aimed for upload. */
  ssize_t upload_present;

  /* 'upload_fromhere' is used as a read-pointer when we uploaded parts of a
     buffer, so the next read should read from where this pointer points to,
     and the 'upload_present' contains the number of bytes available at this
     position */
  char *upload_fromhere;

  /* Allocated protocol-specific data. Each protocol handler makes sure this
     points to data it needs. */
  union {
    struct FILEPROTO *file;
    struct FTP *ftp;
    struct HTTP *http;
    struct IMAP *imap;
    struct ldapreqinfo *ldap;
    struct MQTT *mqtt;
    struct POP3 *pop3;
    struct RTSP *rtsp;
    struct smb_request *smb;
    struct SMTP *smtp;
    struct SSHPROTO *ssh;
    struct TELNET *telnet;
  } p;
#ifndef CURL_DISABLE_DOH
  struct dohdata *doh; /* DoH specific data for this request */
#endif
  char fread_eof[2]; /* the body read callback (index 0) returned EOF or
                        the trailer read callback (index 1) returned EOF */
#ifndef CURL_DISABLE_COOKIES
  unsigned char setcookies;
#endif
  BIT(header);        /* incoming data has HTTP header */
  BIT(content_range); /* set TRUE if Content-Range: was found */
  BIT(download_done); /* set to TRUE when download is complete */
  BIT(eos_written);   /* iff EOS has been written to client */
  BIT(upload_done);   /* set to TRUE when doing chunked transfer-encoding
                         upload and we're uploading the last chunk */
  BIT(ignorebody);    /* we read a response-body but we ignore it! */
  BIT(http_bodyless); /* HTTP response status code is between 100 and 199,
                         204 or 304 */
  BIT(chunk);         /* if set, this is a chunked transfer-encoding */
  BIT(ignore_cl);     /* ignore content-length */
  BIT(upload_chunky); /* set TRUE if we are doing chunked transfer-encoding
                         on upload */
  BIT(getheader);    /* TRUE if header parsing is wanted */
  BIT(forbidchunk);  /* used only to explicitly forbid chunk-upload for
                        specific upload buffers. See readmoredata() in http.c
                        for details. */
  BIT(no_body);      /* the response has no body */
};

/**
 * Initialize the state of the request for first use.
 */
CURLcode Curl_req_init(struct SingleRequest *req);

/**
 * The request is about to start.
 */
CURLcode Curl_req_start(struct SingleRequest *req,
                        struct Curl_easy *data);

/**
 * The request is done. If not aborted, make sure that buffers are
 * flushed to the client.
 * @param req        the request
 * @param data       the transfer
 * @param aborted    TRUE iff the request was aborted/errored
 */
CURLcode Curl_req_done(struct SingleRequest *req,
                       struct Curl_easy *data, bool aborted);

/**
 * Free the state of the request, not usable afterwards.
 */
void Curl_req_free(struct SingleRequest *req, struct Curl_easy *data);

/**
 * Reset the state of the request for new use, given the
 * settings.
 */
void Curl_req_reset(struct SingleRequest *req, struct Curl_easy *data);


#endif /* HEADER_CURL_REQUEST_H */