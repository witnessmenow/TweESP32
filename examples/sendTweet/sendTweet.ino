/*******************************************************************
    A sample project for sending a tweet directly from an ESP32

    NOTE: This will automatically tweet from your account!

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

const char *consumerKey = "MEOW";

const char *consumerSecret = "WOOF";

const char *accessToken = "MOOOO";

const char *accessTokenSecret = "BAAAAA";

// ----------------------------

const char *ntpServer = "pool.ntp.org";

// For HTTPS requests
WiFiClientSecure client;

TweESP32 twitter(client, consumerKey, consumerSecret, accessToken, accessTokenSecret);

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

    //Required for Oauth
    twitter.timeConfig();

    // Checking the cert is the best way on an ESP32
    // This will verify the server is trusted.
    client.setCACert(twitter_server_cert);

    // ----------------------------
    // NOTE: This will automatically tweet from your account!
    // ----------------------------

    bool success = twitter.sendTweet("Hello World! (Sent from my ESP32 using #TweESP32)");
    if (success)
    {
        delay(5000);
        twitter.sendTweet("I can even reply to tweets to create a thread", twitter.lastTweetId);
    }
}

void loop()
{
    // put your main code here, to run repeatedly:
}