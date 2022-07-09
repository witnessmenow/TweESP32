/*
TweESP32 - A Twitter API library for the ESP32 that can tweet

Copyright (c) 2022  Brian Lough.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef TweESP32_h
#define TweESP32_h

// I find setting these types of flags unreliable from the Arduino IDE
// so uncomment this if its not working for you.
// NOTE: Do not use this option on live-streams, it will reveal your
// private tokens!

//#define TWEESP32_DEBUG 1

// Comment out if you want to disable any serial output from this library (also comment out DEBUG and PRINT_JSON_PARSE)
#define TWEESP32_SERIAL_OUTPUT 1

// Prints the JSON received to serial (only use for debugging as it will be slow)
//#define TWEESP32_PRINT_JSON_PARSE 1

#include <Arduino.h>
#include <Client.h>

// Dependent:
#include <ArduinoJson.h>
#include <UrlEncode.h>

#include "time.h"

#include "mbedtls/md.h"
#include "mbedtls/base64.h"

#ifdef TWEESP32_PRINT_JSON_PARSE
#include <StreamUtils.h>
#endif

#define TWEESP32_HOST "api.twitter.com"

// Fingerprint for "api.twitter.com", correct as of July 6th, 2022
#define TWEESP32_FINGERPRINT "01 6A 26 7A 7B FB 76 87 02 BA 1D 3E C5 A8 A5 75 66 1E 23 B9"

#define TWEESP32_TIMEOUT 2000

#define TWEESP32_NONCE_LENGTH 32
#define TWEESP32_SIGNING_KEY_LENGTH 120

#define TWEESP32_TWEET_ID_LENGTH 30

#define TWEESP32_TWEETS_ENDPOINT "/2/tweets"

struct TweetSearchResult
{
  const char *authorId;
  const char *tweetId;
  const char *text;
  const char *name;
  const char *username;
};

typedef bool (*processTweetSearch)(TweetSearchResult result, int index, int numResults);

class TweESP32
{
public:
  TweESP32(Client &client);
  TweESP32(Client &client, const char *consumerKey, const char *consumerSecret, const char *accessToken, const char *accessTokenSecret, const char *bearerToken = NULL);
  TweESP32(Client &client, const char *bearerToken);

  // Auth Methods
  void updateSigningKey();
  void updateNonce();
  bool calculateSignature(const char *method, const char *url, unsigned long time, const char *queryParams, const char *bodyParams, char *out_sig);
  unsigned long getEpoch();
  void generateAuthHeader(unsigned long time, char *sig, char *outAuth);
  void timeConfig();

  // Generic Request Methods
  int makeGetRequest(const char *command, const char *authorization, const char *accept = "application/json", const char *host = TWEESP32_HOST);
  int makeRequestWithBody(const char *type, const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = TWEESP32_HOST);
  int makePostRequest(const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = TWEESP32_HOST);
  int makePutRequest(const char *command, const char *authorization, const char *body = "", const char *contentType = "application/json", const char *host = TWEESP32_HOST);

  // User methods
  bool sendTweet(char *message, char *replyTo = NULL);
  int searchTweets(processTweetSearch searchCallback, char *query, bool includeUsername = true, char *since_id = NULL);

  int portNumber = 443;

  char lastTweetId[TWEESP32_TWEET_ID_LENGTH];

  const char *ntpServer = "pool.ntp.org";

  int searchWithNameBufferSize = 4500;

  Client *client;
  void lateInit(const char *consumerKey, const char *consumerSecret, const char *accessToken, const char *accessTokenSecret);
  void setBearerToken(const char *bearerToken);

#ifdef TWEESP32_DEBUG
  char *stack_start;
#endif

private:
  char _nonce[TWEESP32_NONCE_LENGTH + 1];
  char _signingKey[TWEESP32_SIGNING_KEY_LENGTH];
  const char *_consumerKey;
  const char *_consumerSecret;
  const char *_accessToken;
  const char *_accessTokenSecret;
  const char *_bearerToken;

  const char *searchEndpointAndParams =
      R"(/2/tweets/search/recent?max_results=10&query=%s)";
  const char *searchIncludeNameParams =
      R"(&expansions=author_id&user.fields=username)";

  int getContentLength();
  int getHttpStatusCode();
  void skipHeaders(bool tossUnexpectedForJSON = true);
  void closeClient();
  void parseError();
#ifdef TWEESP32_DEBUG
  void printStack();
#endif
};

#endif
