Keep an additional "Steam" entry in your taskbar.
The taskbar entry will automatically disappear once the Steam client was shut down.
This should help on desktops where the tray icon is not present.
The entry will always stay minimized. Clicking on the entry will simply start
the `steam` command which triggers the main window to reappear since only one
instance of the Steam client is allowed.

`libsteam_api.so` needs to be in the library search path or next to the binary.
The program will use this library to check for a running instance of Steam.

Either start `steam_taskbar` after Steam was launched or start it with `steam_taskbar --idle`
or the symlink `steam_taskbar_idle` in which case the entry will stay in the taskbar
until Steam was launched. After Steam was launched it will behave as usual and close
once Steam was shut down.

To make the tray icon disappear you can apply the patch `app-indicator.patch` to
the libappindicator sources and then add the library to `LD_LIBRARY_PATH`.
The library must be build as 32 bit.

Note: this is similar to setting `export STEAM_FRAME_FORCE_CLOSE=0` before launching Steam

See also https://github.com/darealshinji/steamwm
