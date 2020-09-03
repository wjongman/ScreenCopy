# ScreenCopy
A program that can take a snapshot of the screen and either save it to file or copy it to the clipboard.  
It sits in the background and can be activated with a global hotkey.  
When active it shows a sizeable, transparent window that can be accurately positioned by using the mouse or the keyboard arrow-keys.  
Frequently used areas can be saved in a preset list, for quick selection later.  
Autosave features make it easy to capture a sequence of views with a minimum of keystrokes.  




## Features
#### Hotkey
When started for the first time, it will ask to select the hotkey used to activate the screengrabber.

#### AutoSave
When saving a file with autosave enabled it will not prompt for a filename, instead the file will be saved in a predefined location with a predefined filename.

![Autosave](images/autosave.png)

#### Portability
When starting, it will look for the file ScreenCopy.ini in it's program directory, if it is found, it will be used to store settings, else settings will be stored in the registry. 

#### Presets
To be done.

#### Caveat
ScreenCopy, when in the background, has no other user interface then the hotkey.
However when started, it will notice it is already running and will activate the running instance instead.

## Building

#### Prerequisites:

WTL10, the [Windows Template Library](https://sourceforge.net/projects/wtl/).

A Visual Studio 2017 project file is included.
