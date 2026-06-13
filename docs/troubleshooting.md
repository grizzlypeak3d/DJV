---
title: Troubleshooting
layout: default
nav_order: 14
---

# Troubleshooting

If something isn't working, the log file in the **Documents/DJV** directory is
the first place to look.

If DJV fails to start, run it from the command line to see startup errors:

```
djv -log
```

If the application is misbehaving, try resetting the settings:

- Delete the directory **Documents/DJV**
- Or pass the **-resetSettings** flag on the command line
