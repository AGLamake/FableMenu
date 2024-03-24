# FableMenu by ermaccer

FableMenu is a plugin for the PC version of Fable: The Lost Chapters, it adds an ImGui powered menu with many features including free camera, object creation, player data manipulation and more!

**Tested only with Steam version!**

# FRC

FRC - Russian Fable Community
It is a group of people involved in developing Fable projects, creating mods for games in the series, and discussing the lore of games.

The team includes coders, artists, musicians and many other interesting personalities.


# Features

- Free camera
- Player morph editor
- Player data editor (Gold, Health, Will, Experience, Position)
- World time control
- World object creation
- World creature creation (also has an option to make some creatures follow and defend player)
- HUD toggle
- Simple slowmotion toggle

and more!

## FRC version Updates

- Added support for using custom translations for the interface and lists of objects and entities.
- The design of the creation tab (objects, creatures) has been changed and brought to a single, dynamically changing view.
- Added the ability to simultaneously create multiple objects/creatures.

# Installation

Download binary files from [Releases](https://github.com/AGLamake/FableMenu/releases) and extract the .zip
archive to root folder of Fable TLC.

Archive breakdown:

 - dinput8.dll - [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/)
 - FableMenu.asi 
 - FableMenu.ini - configuration file

# Issues
- Hiding HUD might make menu disappear when any object is being targeted, simple way to fix is to close menu if open and untick hide HUD option in the pause menu.
- Custom interface translation may be disabled and no longer load. Restarting the game helps avoid the problem.
- The list of objects does not close. A fix will be provided in the next update.

# Usage

Press ~ (default) to open or close menu.

Check .ini file for extra options.


## How to make your own translation

Getting your own translation working involves making changes to the fablemenu_lang.ini, obj.csv and crt.csv files.

fablemenu_lang.ini - is responsible for translating the interface.
Changes made to this file will be automatically applied.
Losing this file will include the original text.

If you want to update the lists of objects and/or creatures, you need to change the obj.csv and crt.csv files.
If you are developing mods, you can also add your own lines to these files so that they can be used in FableMenu.

The format is as follows:
ID, Original_name, Translated_name, Note, Crash, Limited
- ID - identifier
- Original_name - unique name
- Translated_name - translation for beautiful display
- Note - note
- Crash - 1 causes a crash (The trainer will not allow you to create anything with crash=1)
- Limited - 1 spawn is allowed only with limited settings (Used for spawning creatures that do not support factions)

## Acknowledgments

- Jacks (Егор Сычёв) - Ideological mastermind, leader of FRC. Made a great contribution to the translation of the list of objects and entities.
- Ivan Vasiljevich - Contributed to the translation of the list of objects and entities.
- Stanislav Meshkov - Contributed to the translation of the list of objects and entities.
- OdarenkoAs (Андрей Одаренко) - FRC Administrator. Providing advice on some issues related to objects and entities.

