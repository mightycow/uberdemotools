# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

Here you'll learn the principles and basics of UDT's custom parsing API.

How is a Quake demo structured?
-------------------------------

###### Packets

Quake demos are sequences of message bundles (let's call them packets) that the server sends to the client. It is, in essence, a dump of the client's incoming network stream.  

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

Player config strings *can* start with a separator and *can* end with a separator. However, none of that is guaranteed, so **don't ever rely on it** to be the case or not.  
UDT supplies helper functions to deal with that: `udtParseConfigStringValueAsInteger` and `udtParseConfigStringValueAsString`.

###### Config strings and the `cs` command

As we've seen earlier, the gamestate message will define a whole bunch of config string values.  
However, as the variables encoded in config strings change during the game (scores, player names, etc), the config strings will need to be updated as well.  
For that, the server will send a `cs` command, telling the client to update the config string at a given index to the new supplied value.

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

In the original Quake 3 source code, the message bundles (or packets as I call them) were also called messages.  
The data structure describing them is called `msg_t`.

What problems does it solve for me?
-----------------------------------

The API will take care of the following for you: 

1. It handles all the differences between protocol versions when reading in the packets and messages.
2. It deals with Huffman compression and delta-decoding the entities, only giving you raw decompressed data.
3. It deals with duplication so you don't get the same snapshots, commands or entity events twice.
4. It stores the config string values and updates them upon processing the `cs` and `bcs2` commands. You can access the latest version of any config string with `udtCuGetConfigString`.

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

Please note that if you want proper support for Unicode file paths, you will have to change 2 things with respect to the sample applications:

1. The entry point must be `wmain`, not `main`. The `main` entry point on Windows does *not* get UTF-8 file paths!
2. Use another API than `fopen` for dealing with files because it does *not* consider the input as UTF-8.

To deal with Unicode, the recommended course of action is to internally do everything with UTF-8 and only convert from/to UTF-16 when dealing with Windows-specific stuff. No extra steps needed on Linux.  
That's what UDT does for the library and all command-line tools.
