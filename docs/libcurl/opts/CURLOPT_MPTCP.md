---
c: Copyright (C) Dorian Craps, <dorian.craps@student.vinci.be>, et al.
SPDX-License-Identifier: curl
Title: CURLOPT_MPTCP
Section: 3
Source: libcurl
See-also:
  - CURLOPT_TCP_FASTOPEN (3)
Protocol:
  - All
---

# NAME

CURLOPT_MPTCP - enable Multipath TCP

# SYNOPSIS

~~~c
#include <curl/curl.h>

CURLcode curl_easy_setopt(CURL *handle, CURLOPT_MPTCP, long enable);
~~~

# DESCRIPTION

Pass a long as parameter set to 1L to enable or 0 to disable.

MPTCP is an extension to the standard TCP that allows multiple TCP streams
over different network paths between the same source and destination.
This can enhance bandwidth and improve reliability by using multiple paths
simultaneously.
MPTCP is beneficial in networks where multiple paths exist between clients
and servers, such as mobile networks where a device may switch between WiFi
and cellular data or in wired networks with multiple Internet Service Providers.

Enabling MPTCP can improve the performance and reliability of network requests,
particularly in environments where multiple network paths (e.g., WiFi and
cellular) are available.

MPTCP support depends on the underlying operating system and network
infrastructure. Some networks might drop unknown MPTCP, and its effectiveness
varies based on the network configuration and conditions. If MPTCP is not
supported by the network or the end server, the connection seamlessly falls back
to TCP.

# DEFAULT

0

# EXAMPLE

~~~c
int main(void)
{
  CURL *curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
    curl_easy_setopt(curl, CURLOPT_MPTCP, 1L);
    curl_easy_perform(curl);
  }
}
~~~

# AVAILABILITY

Support for MPTCP in libcurl requires Linux 5.6 or later. Only TCP connections
are modified, hence this option does not effect HTTP/3 (QUIC) connections.

The features availability in libcurl can also depend on the version of libcurl.
Added in 8.8.0.

# RETURN VALUE

Returns CURLE_OK if MPTCP is successfully enabled for the connection,
otherwise returns an error code specific to the reason it could not be enabled,
which might include lack of operating system support or libcurl not being built
with MPTCP support.
