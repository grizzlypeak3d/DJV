# Troubleshooting

If something isn't working, the log is the first place to look. **Tools > System Log** shows it and has a **Copy** button; the file itself is in the application support directory, and **Help > System Information** shows the path.

If DJV fails to start, run it from the command line to see startup errors:

```
djv -log
```

If the application is misbehaving, try resetting the settings:

- Delete the settings file in the application support directory; **Help > System Information** shows the path
- Or pass the **-resetSettings** flag on the command line

If the window is blank or drawn wrongly after choosing another **Renderer** in the **Graphics** section of the **Settings** tool, start DJV with OpenGL, which draws whatever is set, and change the setting back:

```
djv -renderer OpenGL
```

The log and **Help > System Information** say what is drawing, and with which graphics device and driver.

## "Open with" on Windows

If choosing DJV in the **Open with** dialog brings the dialog straight back instead of opening the file, the entry Windows keeps for DJV is naming a version that has been removed. Windows writes that entry itself the first time a file is opened with DJV and never revisits it, so it outlives the install it was written for. Clearing it lets the entry the installer writes take over:

```
powershell -NoProfile -Command "Remove-Item -Recurse -Force 'HKCU:\Software\Classes\Applications\djv.exe'"
```

Then choose DJV from **Open with** again. DJV 3.7.0 and later puts these entries right itself each time it starts, including the ones **Always use this app** leaves, so starting DJV once from the Start menu is usually enough. The command is for when it is not.
