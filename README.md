# YTopen

YTopen is a lightweight Windows tray utility for quick YouTube search and channel opening using a global hotkey.

## What it does

- on first run the program creates configuration storage in `.data/config.yml`
- shows a setup console to choose browser or custom browser path
- after setup the app runs in the Windows system tray
- it registers the hotkey `Win+Y`
- pressing `Win+Y` opens the console window
- in the console you can use one command format:
  - `y <query>` to search for a YouTube video
  - `y c <channel>` or `y channel <channel>` to search for a YouTube channel
- when a search command runs, YTopen opens the browser and hides the console back into the tray
- app auto-adds itself to Windows startup using the user startup folder

## Implementation details

- `main.c` is the application entrypoint
- `src/utils.h` declares helper functions and config access
- `src/scripts/utils.c` implements:
  - tray icon and hotkey registration with WinAPI
  - console show/hide behavior and WM_CLOSE interception
  - browser selection and config file management
  - YouTube URL generation with UTF-8 encoding for Russian text
  - loading `assets/icon.ico` or `assets/icon.jpg` for the tray icon
- the config file is stored under `.data/config.yml`
- startup setup copies `YTopen.exe` into the Windows startup folder

## Commands

- `y <query>`
- `y c <channel>`
- `y channel <channel>`
- `edit` to change browser setup
- `exit` to close the console

## Notes

- put `assets/icon.jpg` or `assets/icon.ico` in the `assets` folder to change the tray icon
- the app hides the console instead of closing it when the console window is closed
- the console help text is shown only once per session
