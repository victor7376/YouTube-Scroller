# YouTube-Scroller
Marquee scroller which shows Time, date &amp; how many subscribers you have

To make this work, please follow the instructions from Brian Lough YouTube Library:

## Getting a Google Apps API key (Required!)

* Go to [the Google developer dashboard](https://console.developers.google.com) and create a new project
* In the "API & Services" menu, [go to "Credentials"](https://console.developers.google.com/apis/credentials) and create a new API key.
* Head to [the Google API library](https://console.developers.google.com/apis/library), find the ["YouTube Data API v3"](https://console.developers.google.com/apis/library/youtube.googleapis.com), and "Enable" it for your project.
* Make sure the following URL works for you in your browser (change the key at the end!):

https://www.googleapis.com/youtube/v3/channels?part=statistics&id=UCezJOfu7OtqGzd5xrP3q6WA&key=PutYourNewlyGeneratedKeyHere

If everything is working correctly you should see the JSON channel statistics for his YouTube channel!

## This code is a combination

This code is a combination of two main arduino projects.

Main part is from Qrome/marquee-scroller

2nd part is from KiKi's code for a YouTube monitor:

https://www.youtube.com/watch?v=19NfoJHLdIc

It has all been updated to work with the latest libraries of the ESP8266 & ArduinoJson.

If there is any issues please let me know as its still a work in progress project.
