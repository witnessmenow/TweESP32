# TweESP32

A Twitter API library for the ESP32 that can tweet

**Work in progress library - expect changes!**

## Supported Boards:

### ESP32

This library uses some lower level methods that are included with the ESP32 for SHA1 encrypting and Base64 encoding, so it will only work with the ESP32.

It may work with other variants (ESP32-S2 etc) but I have not tested it yet.

## Help support what I do!

I put a lot of effort into getting a solution for being able to tweet directly from an ESP32, hopefully people find it useful!

[If you enjoy my work, please consider becoming a Github sponsor!](https://github.com/sponsors/witnessmenow/)

## Library Features:

The Library supports the following features:

- Send Tweets:
  - Basic tweets and replies to specific tweets
- Search Tweets

### What needs to be added:

A lot! My main goal was to get basic tweeting working, but now the Oauth is working, there is no reason why other API features could not be added.

The code also requires a good bit of cleanup, there is a healthy mix of different String types being used at the moment, and probably doesn't need to be.

### Oauth Signature

The twitter API requires the use of Oauth signature for authentication, I could not find any examples of people doing this on the ESP32, so with the lack of any better resource, this library could be used as a template for doing Oauth signature for other APIs if needed.

The OAuth Signature code is mainly contained in the `calculateSignature` funciton, which is heavily based on code from the [Arduino_OAuth library](https://github.com/arduino-libraries/Arduino_OAuth). This library seems to be geared towards WiFi Nina based boards (e.g MKR1010). My changes involved porting this method to make use of lower level ESP32 funcitons for SHA1 encrypting and Base64 encoding. Major thanks to the people who worked on the library, it certainly helped a lot.

## Setup Instructions

### Twitter Developer Account

- Sign into the folllwing with your twitter account (the one that will tweet) https://developer.twitter.com/en/portal/dashboard
- Create an Project and an Application
- Generate keys, you will need the following:
  - For Sending a tweet you need the following:
    - consumerKey
    - consumerSecret
    - accessToken (Make sure the access tokens are created with Read & Write permissions)
    - accessTokenSecret
  - For searching for tweets you need the following:
    - bearerToken

### Installation

Download zip from Github and install to the Arduino IDE using that.

#### Dependancies

- V6 of Arduino JSON - can be installed through the Arduino Library manager.
- UrlEncode - can be installed through the Arduino Library manager.

### Running

Open one of the examples, fill in your Wifi details and Keys.

## Usage

#### Send a tweet:

```
twitter.sendTweet("Hello World!");
```

returns true on sucess. `twitter.lastTweetId` will also be updated with the ID of the tweet

#### Send a tweet as a reply:

```
twitter.sendTweet("I am replying to your message!", "1544835197568425985");
```

The second param is the twitter ID you are replying to (can be found in the URL or you can use `twitter.lastTweetId`)

returns true on sucess. `twitter.lastTweetId` will also be updated with the ID of the tweet

#### Searching for tweets:

```
twitter.searchTweets(processTweetsCallback, "%23BlinkBriansLight", true, "1234567890954");
```

Searches Tweets for matches in the last 7 days

- First param is a callback for each tweet that is found by the search
- Second is the search query, it must be URL encoded. [Check here for details](https://developer.twitter.com/en/docs/twitter-api/tweets/search/integrate/build-a-query)
- Third is for include Usernames. If you pass false it will save some bandwidth and be slightly faster. Defaults to true
- Fourth is "from_ID", if this is included only tweets newer than this will be returned. Defaults to NULL

- Returns number of results found if succesful, or -1 if it failed. Note: 0 means it was successful but returned no results.

##### Basic example

```
bool printTweet(TweetSearchResult tweet, int index, int numMessages)
{
  Serial.print(tweet.username);
  Serial.print(": ");
  Serial.println(tweet.text);

  // Also available
  //tweet.authorId
  //tweet.tweetId
  //tweet.name // Brian Lough

  return true; // returning false here stops the callbacks
}
}

void searchTweet(){

    // query must be url encoded. urlEncode is provided by the URLEncode.h library
    String query = urlEncode("#BlinkBriansLight");
    int numberOfResponses = twitter.searchTweets(processTweets, (char *)query.c_str(), true);

    //you could skip the encoding if you it manually
    //numberOfResponses = twitter.searchTweets(processTweets, "%23BlinkBriansLight", true);
}

```

The second param is the twitter ID you are replying to (can be found in the URL or you can use `twitter.lastTweetId`)

returns true on sucess. `twitter.lastTweetId` will also be updated with the ID of the tweet

## Compile flag configuration

There are some flags that you can set in the `TweESP32.h` that can help with debugging

```

//#define TWEESP32_DEBUG 1
// Enables extra debug messages on the serial.
// Will be disabled by default when library is released.
// NOTE: Do not use this option on live-streams, it will reveal your private tokens!

#define TWEESP32_SERIAL_OUTPUT 1
// Comment out if you want to disable any serial output from this library
// (also comment out DEBUG and PRINT_JSON_PARSE)

//#define TWEESP32_PRINT_JSON_PARSE 1
// Prints the JSON received to serial (only use for debugging as it will be slow)
// Requires the installation of ArduinoStreamUtils (https://github.com/bblanchon/ArduinoStreamUtils)

```
