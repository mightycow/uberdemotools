# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

This document will cover the structure of **Quake** demos and the basics of UDT's custom parsing API.

Quake Demo Structure
--------------------

###### Packets

**Quake** demos are sequences of message bundles (let's call them packets) that the server sends to the client. It is, in essence, a dump of the client's incoming network stream.  

So, a well-formed demo file/stream is structured like this:
> [packet header] [packet data] [packet header] [packet data] ... [EoS packet header]

A packet header is structured like this:

| Byte count | Meaning                     | 
|:-----------|:----------------------------|
| 4          | Message sequence number     |
| 4          | Packet data length in bytes |

When the message sequence number and the packet length are both -1 (all 64 bits of the header set to 1), you have an *End of Stream* (EoS) packet and no data follows.

###### Packet message types

A packet can contain one or several messages of different types.  
Here are the 3 message types we care about:

| Message type | Frequency                           | Description |
|:-------------|:------------------------------------|:------------|
| Game state   | Only sent when a map is (re-)loaded | Baseline entities + config strings |
| Snapshot     | Sent continuously                   | Delta-encoded entity states + player state |
| Command      | Sent irregularly                    | A command encoded as a string |

In a packet, you either get:

* A single game state message and nothing else
* A single snapshot and any number of command messages

###### The command message

The command messages are encoded in strings. 
Here is an example:
> cs 1 "\g_syncronousClients\0\sv_pure\0\sv_serverid\301783138\timescale\1\fs_game\cpma\sv_cheats\0"

This command tells the client to replace the config string (hence `cs`) at index `1` with what follows in the quotes.

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

###### Config strings encoding multiple variables

The player, system info and server info config strings encode multiple "variables" in a single string.  
The config string is usually created as follows (pseudo-code):
```
cs = string.empty()
foreach variable in variables
  if(loopindex > 0) cs.append(backslash)
  cs.append(variable.name)
  cs.append(backslash)
  cs.append(variable.value)
```

In practice, a player config string looks like this: "n\UnnamedPlayer\t\3\hc\100".  
In this specific example:

* the name (n) is UnnamedPlayer
* the team (t) is 3 (he's a spectator)
* the handicap (hc) is 100

Player config strings *can* start with a separator and *can* end with a separator. However, none of that is guaranteed, so don't ever rely on it to be the case or not. UDT supplies helper functions to deal with that: `udtParseConfigStringValueAsInteger` and `udtParseConfigStringValueAsString`.

###### Config string indices

The index of a config string encoding some specific type of data can change from a protocol version to another.  
To know what index to use, you can use the helper function `udtGetIdConfigStringIndex`.

However, the first 2 config string indices are reserved:
* 0 is always for server info
* 1 is always for system info

###### Config strings and the `cs` command

As we've seen earlier, the gamestate message will define a whole bunch of config string values. However, as the variables encoded in config strings change during the game (scores, player names, etc), the server will need to send commands with updated config strings to the client. For that, the server will send a `cs` command, telling the client to update the config string at a given index to the new supplied value.

###### Config strings and the `bcs0`, `bcs1` and `bcs2` commands

When a config string is deemed too large to be put in a network packet by the Quake server, it gets split up into multiple chunks. (The `b` in `bcs` means big.)

| Command | Description |
|:--------|:------------|
| bcs0    | Start a new config string |
| bcs1    | Append to the config string and expect more |
| bcs2    | Append to the config string (last append) |

UDT handles those commands for you, you only have to concern yourself with the `cs` commands if you want to.

###### Player state and entity state

The entity state is a general-purpose data structure. It can represent players, items, projectiles, events, etc. The fields can have different meanings for different entity state types (the `idEntityStateBase::eType` field).

The player state is used for representing the spectated player only. Because it is a superset of the entity state, it can be converted to an entity state (the `BG_PlayerStateToEntityState` function in the original **Quake 3** source code).

###### Note on the terminology

In the original Quake 3 source code, the message bundles (or packets as I call them) were also called messages. The data structure describing them is called `msg_t`.

Features
--------

The custom parsing API has the following features: 

* No third-party library dependency: the UDT shared library is self-contained
* Parsing support for *all* **Quake 3** and **Quake Live** demo protocols
* The user has full flow control: setting up callbacks (fatal error and message printing) is optional
* The user supplies the input buffers: you can read demo data from any source (hard drive, over the network, etc)
* Access to the raw decompressed data
* No duplication: you don't get the same snapshot, command or entity event more than once
* For each snapshot, you get the list of entities that were added/changed and the numbers of the entities that were removed
* Access to the latest version of all config strings with `udtCuGetConfigString`
* The only config string update command you get is `cs`: `bcs0`, `bcs1` and `bcs2` are dealt with transparently
* Command tokens access: it tokenizes commands and gives you access to the results
* A helper function for string clean-ups, `udtCleanUpString`, that gets rid of Quake 3/Live and OSP color codes
* A helper function, `udtPlayerStateToEntityState`, to convert a player state to an entity state
* Helper functions to parse config string variables:
  * `udtParseConfigStringValueAsInteger`
  * `udtParseConfigStringValueAsString`
* A helper function, `udtGetIdMagicNumber`, to know what magic number Quake uses for a given UDT identifier
* A helper function, `udtGetUDTMagicNumber`, to know what UDT identifier corresponds to a given Quake magic number
* The supported magic number types are:
  * power-up indices
  * life stats indices (stats that reset on respawn)
  * persistent stats indices
  * entity types
  * entity flags
  * entity events
  * config string indices
  * teams
  * game types
  * flag statuses
  * weapons
  * means of death
  * items
  * player movement types

Usage
-----

###### Start-up and shut-down

Call `udtInitLibrary` before any other functions. (One exception: you can call `udtSetCrashHandler` first.)  
Call `udtShutDownLibrary` after every other function.

###### Creating a custom demo parsing context

Call `udtCuCreateContext` to create a new context.  
Call `udtCuDestroyContext` to free all resources of a context.

###### Parsing a demo with your context

In some pseudo-code, here's how you process a demo:

```
udtCuStartParsing(context, protocol);
Packet packet;
  while ( ReadDemoPacket(packet) )
    udtCuParseMessage(context, packet);
```

Sample Applications
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

A Note for Windows developers
-----------------------------

Please note that if you want proper support for Unicode file paths, you will have to change 2 things with respect to the sample applications:

1. The entry point must be `wmain`, not `main`. The `main` entry point on Windows does *not* get UTF-8 file paths!
2. Use another API than `fopen` for dealing with files because it does *not* consider the input as UTF-8.

To deal with Unicode, the recommended course of action is to internally do everything with UTF-8 and only convert from/to UTF-16 when dealing with Windows-specific stuff. No extra steps needed on Linux. That's what UDT does for the library and all command-line tools.

Additional Resources
--------------------

* Article: [The **Quake 3** Networking Model](http://trac.bookofhook.com/bookofhook/trac.cgi/wiki/Quake3Networking) by *Brian Hook*, who worked on **Quake 3**
* Article: [**Quake 3** Source Code Review: The Network Model](http://fabiensanglard.net/quake3/network.php) by *Fabien Sanglard*  
Note that Huffman compression wasn't used originally. It was introduced with protocol 66 (*.dm_66 demo files).
* Source Code: [**Quake 3**](https://github.com/id-Software/Quake-III-Arena) by *id Software*
* Source Code: [**WolfcamQL**](https://github.com/brugal/wolfcamql) by *Angelo 'brugal' Cano*
