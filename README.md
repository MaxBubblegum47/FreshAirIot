# FreshAirIot
### DISCLAIMER
This README file is still under developing, I suggest the reader to use the pdf report that I have written 😙

## Introduction
The air pollution is becoming more and more dangerous for the public health. Is important to avoid breathing (https://www.who.int/news-room/fact-sheets/detail/ambient-(outdoor)-air-quality-and-health) unhealthy air. FreshAirIot aims to check the air's quality of the area in which the user lives and suggest if is the case, or not, to open the window. The esp boards are scattered inside of the house and outside (front yard/back yard, windows) and they collect data about the environment. Once they have enough data they suggest the user what to do. There are 3 type of notifications that the final user receive:
- telegram notification
- specific rgb light that are visible on the board (green = open the window, yellow = you may consider to open the window, red = do not open the window)
- specific music theme that are played by the board that are related with color of the rgb light (zelda's theme = green, pacman's theme = yellow, doom's theme = red)

For what concerns the telegram notification there is a telegram bot that is always available and can provide the user some information about the current weather condition and forecasting up to 2 days.
The project is built using two types of boards:
- ESP 8266
- ESP 32

All the sensors are available online and I suggest the reader to use Aliexpress website, to get the cheaper price. As I said in the \textbf{Prerequisites} section, you need to install some additional libraries, but this will be more clear in the Software chapter.
