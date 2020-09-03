# ScreenCopy

A program that can take a snapshot of the screen and either save it to file or copy it to the clipboard.  
It sits in the background and can be activated with a global hotkey.  
When active, it shows a sizeable, transparent window that can be accurately positioned by using the mouse or the keyboard arrow-keys.  
Frequently used areas can be saved in a preset list, for quick selection later.  
Autosave features make it easy to capture a sequence of views with a minimum of keystrokes.  



## Prerequisites:

WTL10, the [Windows Template Library](https://sourceforge.net/projects/wtl/).

## Building:

A Visual Studio 2017 project file is included.

## Usage:

#### Hotkey
When started for the first time, it will ask to select the hotkey used to activate the screengrabber.

#### AutoSave
When saving a file with autosave enabled, you will not be prompted for a filename, instead the file will be saved in a predefined location with a predefined filename.  
![Autosave](/images/autosave.png)

#### Portability
When started, it will look for the file ScreenCopy.ini in it's program directory, if it is found, it will be used to store settings, else settings will be stored in the registry. 

#### Presets
Presets to be done.