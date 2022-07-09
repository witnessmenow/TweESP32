/*******************************************************************
    Search for tweets

    Parts:
    ESP32 Dev Board
       Aliexpress: * - https://s.click.aliexpress.com/e/_dSi824B
       Amazon: * - https://amzn.to/3gArkAY

 *  * = Affilate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow

 *******************************************************************/

// ----------------------------
// Standard Libraries
// ----------------------------

#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "time.h"

// ----------------------------
// Required Libraries
// ----------------------------

#include <TweESP32.h>          // Install from Github - https://github.com/witnessmenow/TweESP32
#include <TwitterServerCert.h> // included with above

// ----------------------------
// Dependant Libraries
// ----------------------------

#include <UrlEncode.h> //Install from library manager

#include <ArduinoJson.h> //Install from library manager

// ----------------------------
// ------- Replace the following! ------
// ----------------------------

char ssid[] = "SSID";         // your network SSID (name)
char password[] = "Password"; // your network key

// Create a project and an app here to get keys https://developer.twitter.com/en/portal/dashboard

const char *bearerToken = "QUACK";

// ----------------------------

// For HTTPS requests
WiFiClientSecure client;

TweESP32 twitter(client, bearerToken);

unsigned long requestDueTime;               //time when request due
unsigned long delayBetweenRequests = 45000; // Time between requests (45 seconds) // Rate limit seems to be 450 reqs per 15 minutes (30 seconds)

char lastTweetId[50];
bool haveLastTweetId = false;

void setup()
{

    Serial.begin(115200);

    // Connect to the WiFI
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Checking the cert is the best way on an ESP32
    // This will verify the server is trusted.
    client.setCACert(twitter_server_cert);
}

void printTweet(TweetSearchResult tweet)
{
    Serial.print(tweet.username); //witnessmenow
    Serial.print(": ");
    Serial.println(tweet.text);

    // Also available
    //tweet.authorId
    //tweet.tweetId
    //tweet.name // Brian Lough
}

bool processTweets(TweetSearchResult tweet, int index, int numMessages)
{
    printTweet(tweet);
    if (index == 0)
    {
        // This will save the most recent tweetID so only newer tweets than this will be returned
        strcpy(lastTweetId, tweet.tweetId);
        haveLastTweetId = true;
    }
    return true; // You can stop the callback by returning false
}

void loop()
{
    if (millis() > requestDueTime)
    {
        int numberOfResponses = 0;

        //Info for creating search queries: https://developer.twitter.com/en/docs/twitter-api/tweets/search/integrate/build-a-query

        // query must be url encoded. urlEncode is provided by the URLEncode.h library
        String query = urlEncode("#dogs");
        if (haveLastTweetId)
        {
            numberOfResponses = twitter.searchTweets(processTweets, (char *)query.c_str(), true, lastTweetId);
        }
        else
        {
            numberOfResponses = twitter.searchTweets(processTweets, (char *)query.c_str(), true);
            //you could skip the encoding if you it manually
            //numberOfResponses = twitter.searchTweets(processTweets, "%23dogs", true);
        }

        // 0 tweets back is valid, your search might not have matched anything in the last 7 days.
        if (numberOfResponses >= 0)
        {
            Serial.print("Recieved ");
            Serial.print(numberOfResponses);
            Serial.println(" tweets");
        }
        else
        {
            Serial.println("error getting tweets");
        }

        requestDueTime = millis() + delayBetweenRequests;
    }
}