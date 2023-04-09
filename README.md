# Nervos-Doc-Search-Bot
Nervos Doc Search Telegram Bot made for the 2023 Nervos Doc-a-thon

The general idea was to create a customised search engine targeting known sources of Nervos related knowledge. 
This customised search engine can be found at https://nervosdocs.bit.cc 

Additionally I've created a Telegram bot which utilises this custom search engine and provides the title, snippet and link of the top 10 results of your query in a private message between you and the bot. This Telegram bot is written in Arduino (C/C++) and runs on an esp32s3 microcontroller. The Telegram bot can be found at @NervosDocBot in Telegram. 

Several Additional commands which leverage the GitHub REST API have been added to the bot. List and Show RFCS, System Script, Production Script, Miscellaneous Script and Reference. These commands can be accessed by typing '/' in private message with the bot.
