/*
TweESP32 - An Arduino library to wrap the Spotify API

Copyright (c) 2021  Brian Lough.

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

#include "TweESP32.h"

TweESP32::TweESP32(Client &client)
{
    this->client = &client;
}

TweESP32::TweESP32(Client &client, const char *consumerKey, const char *consumerSecret, const char *accessToken, const char *accessTokenSecret, const char *bearerToken)
{
    this->client = &client;
    lateInit(consumerKey, consumerSecret, accessToken, accessTokenSecret);
    if (bearerToken != NULL)
    {
        setBearerToken(bearerToken);
    }
}

TweESP32::TweESP32(Client &client, const char *bearerToken)
{
    this->client = &client;
    setBearerToken(bearerToken);
}

int TweESP32::makeRequestWithBody(const char *type, const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    client->flush();
#ifdef TWEESP32_DEBUG
    Serial.println(host);
#endif
    client->setTimeout(TWEESP32_TIMEOUT);
    if (!client->connect(host, portNumber))
    {
#ifdef TWEESP32_SERIAL_OUTPUT
        Serial.println(F("Connection failed"));
#endif
        return -1;
    }

    // give the esp a breather
    yield();

    // Send HTTP request
    client->print(type);
    client->print(command);
    client->println(F(" HTTP/1.0"));

    //Headers
    client->print(F("Host: "));
    client->println(host);

    client->print(F("Content-Type: "));
    client->println(contentType);

    if (authorization != NULL)
    {
        client->print(F("Authorization: "));
        client->println(authorization);
    }

    client->print(F("Content-Length: "));
    client->println(strlen(body));

    client->println();

    client->print(body);

    if (client->println() == 0)
    {
#ifdef TWEESP32_SERIAL_OUTPUT
        Serial.println(F("Failed to send request"));
#endif
        return -2;
    }

    int statusCode = getHttpStatusCode();
    return statusCode;
}

int TweESP32::makePutRequest(const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    return makeRequestWithBody("PUT ", command, authorization, body, contentType);
}

int TweESP32::makePostRequest(const char *command, const char *authorization, const char *body, const char *contentType, const char *host)
{
    return makeRequestWithBody("POST ", command, authorization, body, contentType, host);
}

int TweESP32::makeGetRequest(const char *command, const char *authorization, const char *accept, const char *host)
{
    client->flush();
    client->setTimeout(TWEESP32_TIMEOUT);
    if (!client->connect(host, portNumber))
    {
#ifdef TWEESP32_SERIAL_OUTPUT
        Serial.println(F("Connection failed"));
#endif
        return -1;
    }

    // give the esp a breather
    yield();

    // Send HTTP request
    client->print(F("GET "));
    client->print(command);
    client->println(F(" HTTP/1.0"));

    //Headers
    client->print(F("Host: "));
    client->println(host);

    if (accept != NULL)
    {
        client->print(F("Accept: "));
        client->println(accept);
    }

    if (authorization != NULL)
    {
        client->print(F("Authorization: "));
        client->println(authorization);
    }

    if (client->println() == 0)
    {
#ifdef TWEESP32_SERIAL_OUTPUT
        Serial.println(F("Failed to send request"));
#endif
        return -2;
    }

    int statusCode = getHttpStatusCode();

    return statusCode;
}

void TweESP32::updateSigningKey()
{
    sprintf(_signingKey, "%s&%s", _consumerSecret, _accessTokenSecret);
    int length = strlen(_consumerSecret) + strlen(_accessTokenSecret) + 1;
    _signingKey[length] = '\0';
}

void TweESP32::updateNonce()
{
    int i = 0;
    while (i < TWEESP32_NONCE_LENGTH)
    {
        char c = random(255);

        if (isAlphaNumeric(c))
        {
            _nonce[i] = c;
            i++;
        }
    }
    _nonce[TWEESP32_NONCE_LENGTH] = '\0';
}

// Feels weird to have this here and not in the header, its the same as OAuthClient though
static int strcmp_pointer(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

// This function was ported to work with the ESP32 from https://github.com/arduino-libraries/Arduino_OAuth
// The changes I made were to make it work with the ESP32 SHA1 encryption and Base64 methods.

bool TweESP32::calculateSignature(const char *method, const char *url, unsigned long oauthTime, const char *queryParams, const char *bodyParams, char *out_sig)
{
    // This function is long due to the complexity of the OAuth signature.
    // It must collect all the parameters from the oauth, query, and body params,
    // then sort the param key values lexicographically. After these steps the
    // signature can be calculated.

    // calculate the OAuth params
    String oauthParams;

    oauthParams += "oauth_consumer_key=";
    oauthParams += _consumerKey;
    oauthParams += "&oauth_nonce=";
    oauthParams += _nonce;
    oauthParams += "&oauth_signature_method=HMAC-SHA1&oauth_timestamp=";
    oauthParams += String(oauthTime);
    oauthParams += "&oauth_token=";
    oauthParams += _accessToken;
    oauthParams += "&oauth_version=1.0";

    // calculate the length of all of the params
    int paramsLength = oauthParams.length();
    int queryParamsLength = strlen(queryParams);
    int bodyParamsLength = strlen(bodyParams);

    if (queryParams)
    {
        paramsLength += (1 + queryParamsLength);
    }

    if (bodyParams)
    {
        paramsLength += (1 + bodyParamsLength);
    }

    // copy the parameters to a buffer
    char params[paramsLength + 1];
    char *temp = params;

    temp = strcpy(temp, oauthParams.c_str());
    temp += oauthParams.length();

    if (queryParams)
    {
        *temp++ = '&';
        strcpy(temp, queryParams);
        temp += queryParamsLength;
    }

    if (bodyParams)
    {
        *temp++ = '&';
        strcpy(temp, bodyParams);
        temp += bodyParamsLength;
    }

    *temp = '\0';

    // calculate the number of parameters
    int numParams = 0;
    for (int i = 0; i < paramsLength; i++)
    {
        if (params[i] == '=')
        {
            numParams++;
        }
    }

    // collect the keys of the parameters to an array
    // and also replace the = and & characters with \0
    // this will help with the sorting later
    const char *paramKeys[numParams];
    int paramIndex = 0;
    const char *lastKey = params;

    temp = params;
    while (1)
    {
        char c = *temp;

        if (c == '\0')
        {
            break;
        }
        else if (c == '=')
        {
            paramKeys[paramIndex++] = lastKey;

            *temp = '\0';
        }
        else if (c == '&')
        {
            lastKey = (temp + 1);

            *temp = '\0';
        }

        temp++;
    }

    // sort the param keys
    qsort(paramKeys, numParams, sizeof(uintptr_t), strcmp_pointer);

    String payload = String(method);
    payload += "&";
    payload += urlEncode(url);
    payload += "&";
    for (int i = 0; i < numParams; i++)
    {
        const char *paramKey = paramKeys[i];
        int keyLength = strlen(paramKey);
        const char *paramValue = paramKey + keyLength + 1;

        payload += urlEncode(paramKey);
        payload += "%3D"; //urlEncode("=")
        payload += urlEncode(paramValue);

        if ((i + 1) < numParams)
        {
            payload += "%26"; //urlEncode("&")
        }
    }

    byte shaResult[20];

#ifdef TWEESP32_DEBUG
    Serial.println("payload:");
    Serial.println(payload.c_str());
#endif

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;

    const size_t payloadLength = strlen(payload.c_str());
    const size_t keyLength = strlen(_signingKey);

#ifdef TWEESP32_DEBUG
    Serial.print("_signingKey: ");
    Serial.println(_signingKey);
#endif

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *)_signingKey, keyLength);
    mbedtls_md_hmac_update(&ctx, (const unsigned char *)payload.c_str(), payloadLength);
    mbedtls_md_hmac_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

#ifdef TWEESP32_DEBUG
    Serial.print("Hash: ");

    for (int i = 0; i < sizeof(shaResult); i++)
    {
        char str[3];

        sprintf(str, "%02x", (int)shaResult[i]);
        Serial.print(str);
    }
    Serial.println("");
#endif

    int rawSignatureLength = sizeof(shaResult);

#ifdef TWEESP32_DEBUG
    Serial.print("rawSignatureLength: ");
    Serial.println(rawSignatureLength);
#endif

    //int signatureLength = (rawSignatureLength * 8) / 6;
    int signatureLength = 50; // I'm not sure how to work out the length properly ....
    char signature[signatureLength + 1];

    size_t outlen;
    int result = mbedtls_base64_encode((unsigned char *)signature, signatureLength, &outlen, shaResult, rawSignatureLength);

    if (result == 0)
    {
        signature[outlen + 1] = '\0';
        String encodedOut = urlEncode(signature);
#ifdef TWEESP32_DEBUG
        Serial.println("Encoded Sig: ");
        Serial.println(encodedOut);
#endif
        strncpy(out_sig, encodedOut.c_str(), strlen(encodedOut.c_str()));
        out_sig[strlen(encodedOut.c_str())] = '\0';
        return true;
    }
    else
    {
#ifdef TWEESP32_SERIAL_OUTPUT
        Serial.print("Base64 failed: ");
        Serial.println(result);
#endif
        return false;
    }
}

unsigned long TweESP32::getEpoch()
{
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
#ifdef TWEESP32_DEBUG
        Serial.println("Failed to obtain time");
#endif
        return (0);
    }
    time(&now);
    return now;
}

void TweESP32::generateAuthHeader(unsigned long time, char *sig, char *outAuth)
{
    sprintf(outAuth, "OAuth oauth_consumer_key=\"%s\",oauth_token=\"%s\",oauth_signature_method=\"HMAC-SHA1\",oauth_timestamp=\"%d\",oauth_nonce=\"%s\",oauth_version=\"1.0\",oauth_signature=\"%s\"",
            _consumerKey,
            _accessToken,
            time,
            _nonce,
            sig);
}

void TweESP32::timeConfig()
{
    configTime(0, 0, ntpServer);
}

bool TweESP32::sendTweet(char *message, char *replyTo)
{

    char body[500];
    if (replyTo != NULL)
    {
        sprintf(body, "{\"text\": \"%s\", \"reply\": {\"in_reply_to_tweet_id\": \"%s\"}}", message, replyTo);
    }
    else
    {
        sprintf(body, "{\"text\":\"%s\"}", message);
    }

    updateNonce();
    unsigned long currentTime = getEpoch();

#ifdef TWEESP32_DEBUG
    Serial.print("body: ");
    Serial.println(body);

    Serial.print("OAuth Nonce: ");
    Serial.println(_nonce);

    Serial.print("OAuth Time: ");
    Serial.println(currentTime);
#endif

    char sig[100];
    bool generatedSig = calculateSignature("POST", "https://api.twitter.com/2/tweets", currentTime, "", "", sig);
    if (!generatedSig)
    {
#ifdef TWEESP32_SERIAL_OUTPUT
        Serial.println(F("Failed to generate Oauth Sig"));
#endif
        return false;
    }

    char auth[900];
    generateAuthHeader(currentTime, sig, auth);
#ifdef TWEESP32_DEBUG
    Serial.print("auth: ");
    Serial.println(auth);
#endif

    int statusCode = makePostRequest(TWEESP32_TWEETS_ENDPOINT, auth, body);
    if (statusCode > 0)
    {
        skipHeaders();
    }
    unsigned long now = millis();

#ifdef TWEESP32_DEBUG
    Serial.print("status Code");
    Serial.println(statusCode);
#endif

    bool success = false;
    if (statusCode == 200 || statusCode == 201)
    {
        StaticJsonDocument<32> filter;
        filter["data"]["id"] = true;

        StaticJsonDocument<96> doc;

        // Parse JSON object
#ifndef TWEESP32_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, *client, DeserializationOption::Filter(filter));
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream, DeserializationOption::Filter(filter));
#endif
        if (!error)
        {
            const char *data_id = doc["data"]["id"];
            strncpy(lastTweetId, data_id, strlen(data_id));
            lastTweetId[strlen(data_id)] = '\0';
            success = true;
        }
        else
        {
#ifdef TWEESP32_SERIAL_OUTPUT
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
#endif
        }
    }
    else
    {
        parseError();
    }

    closeClient();
    return success;
}

int TweESP32::searchTweets(processTweetSearch searchCallback, char *query, bool includeUsername, char *since_id)
{

    char command[500];
    sprintf(command, searchEndpointAndParams, query);

    if (includeUsername)
    {
        strcat(command, searchIncludeNameParams);
    }

    if (since_id != NULL)
    {
        char sinceBuff[50];
        sprintf(sinceBuff, "&since_id=%s", since_id);
        strcat(command, sinceBuff);
    }

#ifdef TWEESP32_DEBUG
    Serial.println(command);
    printStack();
#endif

    char auth[300];
    sprintf(auth, "Bearer %s", _bearerToken);

#ifdef TWEESP32_DEBUG
    Serial.print("auth: ");
    Serial.println(auth);
#endif

    int statusCode = makeGetRequest(command, auth);
    if (statusCode > 0)
    {
        skipHeaders();
    }
    unsigned long now = millis();

#ifdef TWEESP32_DEBUG
    Serial.print("status Code");
    Serial.println(statusCode);
#endif

    int resultNum = -1;
    if (statusCode == 200)
    {

        DynamicJsonDocument doc(searchWithNameBufferSize);

        // Parse JSON object
#ifndef TWEESP32_PRINT_JSON_PARSE
        DeserializationError error = deserializeJson(doc, *client);
#else
        ReadLoggingStream loggingStream(*client, Serial);
        DeserializationError error = deserializeJson(doc, loggingStream);
#endif
        if (!error)
        {
            TweetSearchResult result;

            int resultCount = doc["meta"]["result_count"];
            for (int i = 0; i < resultCount; i++)
            {
                result.authorId = doc["data"][i]["author_id"].as<const char *>();
                result.tweetId = doc["data"][i]["id"].as<const char *>();
                result.text = doc["data"][i]["text"].as<const char *>();

                if (includeUsername)
                {
                    int usersArraySize = doc["includes"]["users"].size();
                    if (usersArraySize != resultCount)
                    {
                        //We have less user objects than messages, which means multiple message are from the same user
                        // we nee to check the authorID against the user objects to match names

                        for (int j = 0; j < usersArraySize; j++)
                        {
                            const char *user_Id = doc["includes"]["users"][j]["id"];
                            if (strcmp(result.authorId, user_Id) == 0)
                            {
                                result.name = doc["includes"]["users"][j]["name"].as<const char *>();
                                result.username = doc["includes"]["users"][j]["username"].as<const char *>();
                                break;
                            }
                        }
                    }
                    else
                    {
                        result.name = doc["includes"]["users"][i]["name"].as<const char *>();
                        result.username = doc["includes"]["users"][i]["username"].as<const char *>();
                    }
                }

                bool continueCallback = searchCallback(result, i, resultCount);
                // User has decided to end the callbacks
                if (!continueCallback)
                {
                    break;
                }
            }

            resultNum = resultCount;
        }
        else
        {
#ifdef TWEESP32_SERIAL_OUTPUT
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
#endif
        }
    }
    else
    {
        parseError();
    }

    closeClient();
    return resultNum;
}

int TweESP32::getContentLength()
{

    if (client->find("Content-Length:"))
    {
        int contentLength = client->parseInt();
#ifdef TWEESP32_DEBUG
        Serial.print(F("Content-Length: "));
        Serial.println(contentLength);
#endif
        return contentLength;
    }

    return -1;
}

void TweESP32::skipHeaders(bool tossUnexpectedForJSON)
{
    // Skip HTTP headers
    if (!client->find("\r\n\r\n"))
    {
#ifdef TWEESP32_SERIAL_OUTPUT
        Serial.println(F("Invalid response"));
#endif
        return;
    }

    if (tossUnexpectedForJSON)
    {
        // Was getting stray characters between the headers and the body
        // This should toss them away
        while (client->available() && client->peek() != '{')
        {
            char c = 0;
            client->readBytes(&c, 1);
#ifdef TWEESP32_DEBUG
            Serial.print(F("Tossing an unexpected character: "));
            Serial.println(c);
#endif
        }
    }
}

int TweESP32::getHttpStatusCode()
{
    char status[32] = {0};
    client->readBytesUntil('\r', status, sizeof(status));
#ifdef TWEESP32_DEBUG
    Serial.print(F("Status: "));
    Serial.println(status);
#endif

    char *token;
    token = strtok(status, " "); // https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm

#ifdef TWEESP32_DEBUG
    Serial.print(F("HTTP Version: "));
    Serial.println(token);
#endif

    if (token != NULL && (strcmp(token, "HTTP/1.0") == 0 || strcmp(token, "HTTP/1.1") == 0))
    {
        token = strtok(NULL, " ");
        if (token != NULL)
        {
#ifdef TWEESP32_DEBUG
            Serial.print(F("Status Code: "));
            Serial.println(token);
#endif
            return atoi(token);
        }
    }

    return -1;
}

void TweESP32::parseError()
{
    //This method doesn't currently do anything other than print
#ifdef TWEESP32_SERIAL_OUTPUT
    DynamicJsonDocument doc(1000);
    DeserializationError error = deserializeJson(doc, *client);
    if (!error)
    {
        Serial.print(F("getAuthToken error"));
        serializeJson(doc, Serial);
    }
    else
    {
        Serial.print(F("Could not parse error"));
    }
#endif
}

void TweESP32::setBearerToken(const char *bearerToken)
{
    this->_bearerToken = bearerToken;
}

void TweESP32::lateInit(const char *consumerKey, const char *consumerSecret, const char *accessToken, const char *accessTokenSecret)
{
    this->_consumerKey = consumerKey;
    this->_consumerSecret = consumerSecret;
    this->_accessToken = accessToken;
    this->_accessTokenSecret = accessTokenSecret;
    updateSigningKey();
}

void TweESP32::closeClient()
{
    //     if (client->connected())
    //     {
    // #ifdef TWEESP32_DEBUG
    //         Serial.println(F("Closing client"));
    // #endif
    //         client->stop();
    //     }
    client->stop();
}

#ifdef TWEESP32_DEBUG
void TweESP32::printStack()
{
    char stack;
    Serial.print(F("stack size "));
    Serial.println(stack_start - &stack);
}
#endif
