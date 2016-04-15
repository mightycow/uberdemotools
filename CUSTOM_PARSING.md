# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

Here you'll learn the principles and basics of UDT's custom parsing API.

How is a Quake demo structured?
-------------------------------

###### Packets

Quake demos are sequences of message bundles (let's call them packets) that the server sends to the client. It is, in essence, a dump of the client's incoming network stream.  

So, a demo file/stream looks like this:
> [packet] [packet] [packet] ... [EoS packet]

A normal packet is structured like this:

| Byte count | Purpose                              | 
|:-----------|:-------------------------------------|
| 4          | message sequence number: `seq`       |
| 4          | packet buffer length in bytes: `len` |
| `len`      | the raw packet data                  |

When the message sequence number and the packet length are both -1, you have an *End of Stream* (EoS) packet and no data follows.

###### Packet message types

A packet can contain one or several messages of different types.  
Here are the 3 message types we care about:

| Message type | Frequency                           | Description |
|:-------------|:------------------------------------|:------------|
| Game state   | Only sent when a map is (re-)loaded | Baseline entities + config strings |
| Snapshot     | Sent continuously                   | Delta-encoded entity states |
| Command      | Sent irregularly                    | A command encoded as a string |

In a packet, you either get:

* A single game state message and nothing else
* A single snapshot and any number of command messages

###### The command message

The command messages are encoded in strings. 
Here is an example:
> cs N "\key1\value1\key2\value2"

This command tells the client to replace the config string (hence `cs`) at index `N` with what follows in the quotes.

###### The gamestate message

| Game state data types | Description |
|:----------------------|:------------|
| Baseline entities     | The initial value entities at given indices should have |
| Config strings        | Generic null-terminated C strings encoding various state (match start time, score, player names, map name, etc) |

###### The snapshot message

| Snapshot data types       | Description |
|:--------------------------|:------------|
| Player state              | An idPlayerState* for the player being spectated |
| Delta-coded entity states | Delta-encoded instances of idEntityState* |

What problems does it solve for me?
-----------------------------------

The API will take care of the following for you: 

1. It handles all the differences between protocol versions when reading in the packets and messages.
2. It deals with Huffman compression and delta-decoding the entities, only giving you raw decompressed data.
3. It deals with duplication so you don't get the same snapshots and command messages twice.

How do I use it?
----------------

###### Start-up and shut-down

Call `udtInitLibrary` before any other functions.  
Call `udtShutDownLibrary` after every other function.

###### Creating a custom demo parsing context

Call `udtCuCreateContext` to create a context.  
Call `udtCuDestroyContext` to free the resources of a context.

###### Parsing a demo with your context

In some pseudo-code, here's how you process a demo:

```
udtCuStartParsing(context, protocol);
Packet packet;
  while ( ReadDemoPacket(packet) )
    udtCuParseMessage(context, packet);
```

Sample applications
-------------------

###### A multi-frag rail cutter [hosted here](https://github.com/mightycow/uberdemotools/blob/develop/UDT_DLL/src/apps/tut_multi_rail.cpp)

The application

1. detects multi-frag rail shots (multiple kills with a single rail shot) from the player recording the demo
2. creates a new cut demo for each instance of that event

It shows off how to analyze snapshot messages.

###### A player join/leave/rename events printer [hosted here](https://github.com/mightycow/uberdemotools/blob/develop/UDT_DLL/src/apps/tut_players.cpp)

The application 

1. lists all the players connected at a given game state
2. prints player join/leave/rename events

It shows off how to analyze gamestate and command messages.

A note for Windows developers
-----------------------------

Please note that if you want proper support for Unicode file paths, you will have to change 2 things wrt to the sample applications:

1. The entry point must be `wmain`, not `main`. The `main` entry point on Windows does *not* get UTF-8 file paths!
2. Use another API than `fopen` for dealing with files because it does *not* consider the input as UTF-8.

To deal with Unicode, the recommended course of action is to internally do everything with UTF-8 and only convert from/to UTF-16 when dealing with Windows-specific stuff. No extra steps needed on Linux.  
That's what UDT does for the library and all command-line tools.