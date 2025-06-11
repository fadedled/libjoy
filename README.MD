# libjoy

This library for the GameCube and Wii facilitates communication with N64 controllers and accessories, the controllers must be connected to the system via a passthrough adapter. This is built on top of the serial interface and is intended to partially handle the PAD library (see below).

## Usage

1. Copy the "lib" and "include" folders into your "portlibs" folder.

2. Add "-ljoy" right before "-libogc" to your LIBS variable.

3. Replace PAD_Init() with JOY_Init(). The JOY_Read() function gets the state of both GC and N64 controllers, no need to use PAD_Read().

4. To use the polling callback, you can use JOY_SetPollingCallback() and call JOY_Read() from there.

5. To initialize a connected Pak, you call JOY_InitPak() which returns the type of Pak detected. If JOY_PAK_TYPE_UNKNOW is returned, then init failed (this should be called once but should not be an issue if it's called multiple times). To check if a Pak is connected but not initialized, call JOY_GetPakType() and check if it returns a JOY_PAK_TYPE_UNKNOWN.

6. Only Rumble Pak is supported for now; you can call JOY_RumbleCtrl() to activate it after initialization.

## Resources

 - [Console Protocols](https://sites.google.com/site/consoleprotocols/home/nintendo-joy-bus-documentation)
 - [Joybus Protocol](https://n64brew.dev/wiki/Joybus_Protocol)