Version: 0.8.0.0

This program depends on DirectX 9.0c as well as the June 2010 DirectX update. 
If the program fails to run with an error like "D3DX9_43.dll not found" then you
should install the latest DirectX runtime from Microsoft. It's usually the top 
link shown when visiting the DirectX section of the Microsoft download 
center. (This shouldn't be an issue if using the installer.)

--------------------------------------------------------------------------------

Options:

Options can be specified on the command line or in the configuration file,
by default conrep.cfg. In the configuration file, options can be set like:

font_name = Lucida Console
font_size = 10

On the command line they are set like so: conrep --font_name="Courier New"

There are three options that can only be used on the command line:
cfgfile         - Specifies which config file to use. If no file is given
                  then the program first looks for "conrep.cfg" in the 
                  users %APPDATA%/Conrep directory. If that isn't found then
                  it checks for "conrep.cfg" in the EXE directory.
adjust          - When used from a conrep console window specifies that the
                  following command line options are intended to modify the
                  existing window. Options marked with * in the next section can
                  all be modified with --adjust.
help            - Displays option descriptions.

The other options can be set either on the command line or from the config file:
shell           - The command to start the console with. Default value is
                  "cmd.exe".
args            - Arguments to pass to the shell. Default is "".
font_name       * The name of the starting font for the console. Default value
                  is "Courier New" 
font_size       * The size of the starting font for the console. Default value 
                  is "10"
rows            * The number of rows for the console. Ignored if maximize is 
                  set to true. Default value is "24".
columns         * The number of columns for the console. Ignored if maximize is 
                  set to true. Default value is "80".
maximize        * Whether the console window should be created so that the 
                  window occupies the entire desktop window (not including the 
                  taskbar, etc.). Use --maximize or --maximize=true to 
                  maximize the window use --maximize=false with --adjust to 
                  change to a normal window mode. Default value is "false"
snap_distance   * Distance from the screen border in pixels that will cause the
                  console window to snap against the edge. Default value is "10"
gutter_size     * Size of the interior border of the console in pixels. Default
                  value is "2".
extended_chars  * Whether or not to display extended characters in the console.
                  This is necessary for some console applications such as many
                  rogue-likes, but doesn't work well with all fonts. Use
                  --extended_chars or --extended_chars=false to display 
                  extended characters, anything else does not. Default value is 
                  "false". This setting can be changed on the fly for via the 
                  right click popup menu in addition to with --adjust.
intensify       * If enabled, maps all non-black foreground colors to the high
                  intensity versions. If not, foreground colors are displayed
                  normally. Use --intensify or --intensify=true to enable and
                  --intensify=false to disable.
zorder          * If set to "top" the console window will be always on top. If 
                  set to "bottom", the window will always be on bottom, just 
                  above the desktop. Otherwise the window will obey normal 
                  window Z order rules. This setting can be changed on the fly 
                  via the right click popup menu.

--------------------------------------------------------------------------------

Known bugs:

* The program cannot handle wallpapers that are too big (6000x4000 known to 
  fail).
* Extended characters may not display correctly in all fonts
* Redraw artifacts when the windows is dragged or gains/loses focus on Vista
  machines with Aero enabled
* Program does not interact properly with console programs that use multiple 
  screen buffers or SetConsoleMode() with ENABLE_MOUSE_INPUT
* Image scaling doesn't match Windows' scaling artifacts when small wallpapers
  are blown up (for example a pixelated 20x20 wallpaper set to Fill).