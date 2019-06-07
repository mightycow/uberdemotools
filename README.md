# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

UDT is a set of tools for analyzing, cutting, converting, modifying and viewing **Quake 3** and **Quake Live** demo files.

The main features are:

- Extracting and displaying information (examples: map name, player names and teams, game mode, etc)
- Cutting by time: creating a new demo file that is a time sub-range of the original demo file for easier processing or demo viewing
- Splitting demos: given a demo with multiple gamestates (happens on map change/reload), create a new demo file per gamestate
- Cutting by patterns: given rules defined by the user, find matching events and cut demos around the times of said events
- Searching for patterns: given rules defined by the user, find matching events and display the results with the option to apply cuts later
- Time-shifting demos: shifting the non-first-person players back in time (a sort of anti-lag)
- Merging demos: given multiple demos from the same match recorded by different players, create a new demo with more complete information
- Converting demos: convert demos to a different protocol version
- Viewing demos with the 2D demo viewer (top-down view)

Official Releases
-----------------

Permanent links to the latest builds

| OS      | GUI | Command-Line Tools | 2D Viewer |
|:--------|:---:|:------------------:|:---------:|
| Windows | [x64](http://myt.playmorepromode.com/udt/redirections/windows_gui_x64.html) - [x86](http://myt.playmorepromode.com/udt/redirections/windows_gui_x86.html) | [x64](http://myt.playmorepromode.com/udt/redirections/windows_console_x64.html) - [x86](http://myt.playmorepromode.com/udt/redirections/windows_console_x86.html) | [x64](http://myt.playmorepromode.com/udt/redirections/windows_viewer_x64.html) - [x86](http://myt.playmorepromode.com/udt/redirections/windows_viewer_x86.html) |
| Linux   | Not Available    | [x64](http://myt.playmorepromode.com/udt/redirections/linux_console_x64.html) - [x86](http://myt.playmorepromode.com/udt/redirections/linux_console_x86.html) | [x64](http://myt.playmorepromode.com/udt/redirections/linux_viewer_x64.html) - [x86](http://myt.playmorepromode.com/udt/redirections/linux_viewer_x86.html) |

Alternatively, you can browse [this folder](http://myt.playmorepromode.com/udt/) where you can also find older releases.  
The zip files are Windows releases.  
The tar.bz2 files are Linux releases.

Project Overview
----------------

The project is currently comprised of 4 parts:

1. A shared library, `UDT_DLL`, with a C89 compatible interface, written in C++. Supported OSes: Windows, Linux
2. A set of command-line tools, written in C++. Supported OSes: Windows, Linux
3. A GUI application, `UDT_GUI`, written in C#. Supported OSes: Windows only (requires the .NET Framework)
4. A 2D demo viewer, `UDT_viewer`, written in C++. Supported OSes: Windows, Linux

Overview of the binaries

| Project name    | Type/Language      | Platforms     | Dependencies  | Description  |
|:----------------|:------------------:|:-------------:|:-------------:|:------------:|
| UDT_DLL         | Library<br>C++     | Windows Linux |  | Shared library that does the actual cutting and analysis work |
| UDT_cutter      | Application<br>C++ | Windows Linux |  | Command-line application for cutting demos by time, chat patterns (*Cut by Chat*) or matches (*Cut by Match*) |
| UDT_splitter    | Application<br>C++ | Windows Linux |  | Command-line application for splitting demos with at least one map change into individual demos with no map changes |
| UDT_timeshifter | Application<br>C++ | Windows Linux |  | Command-line application for shifting the non-first-person players back in time (a sort of anti-lag) |
| UDT_merger      | Application<br>C++ | Windows Linux |  | Command-line application for merging multiple demos into one |
| UDT_json        | Application<br>C++ | Windows Linux |  | Command-line application for exporting analysis data to JSON files (one per demo file) |
| UDT_captures    | Application<br>C++ | Windows Linux |  | Command-line application for exporting a sorted list of all flag captures from the demo recorder to a single JSON file |
| UDT_converter   | Application<br>C++ | Windows Linux |  | Command-line application for converting demos to a different protocol version |
| UDT_GUI         | Application<br>C#  | Windows       | [.NET Framework 4.0 Client Profile](http://www.microsoft.com/en-us/download/details.aspx?id=24872) | GUI application for demo analysis, information display, cutting by time or various patterns, time-shifting, merging, conversions, etc |
| UDT_viewer      | Application<br>C++ | Windows Linux | Windows:<br>Direct3D 11<br>Linux:<br>GLFW 3.0+ | A 2D demo viewer that can generate heat maps |

Supported Formats
-----------------

Read-only demos can be analyzed.  
Read/write demos can be analyzed, modified and cut.

| File extension | Game version | Support level |
|:------|:------------------|:-----------|
| dm3   | Quake 3 1.11-1.17 | Read-only  |
| dm_48 | Quake 3 1.27      | Read-only  |
| dm_66 | Quake 3 1.29-1.30 | Read/write |
| dm_67 | Quake 3 1.31      | Read/write |
| dm_68 | Quake 3 1.32      | Read/write |
| dm_73 | Quake Live        | Read/write |
| dm_90 | Quake Live        | Read/write |
| dm_91 | Quake Live        | Read/write |

Installation
------------

No installation is required for any of the binaries.  
The command-line tools have no third-party dependencies.

For Windows users: `UDT_GUI`, `UDT_cutter` and `UDT_viewer` need to be able to read and write to config files right next to them.  
It is therefore recommended to put all the binaries in a new folder with read and write access (i.e. not in `Program Files` nor `Program Files (x86)`).

`UDT_GUI` requires [**.NET Framework 4.0 Client Profile**](http://www.microsoft.com/en-us/download/details.aspx?id=24872) at a minimum to run.  
If you have **Windows 8** or later, then you should have it pre-installed with the OS unless you changed system settings.

`UDT_viewer` requires Direct3D 11 on Windows and GLFW 3 on Linux.

Project Origin
--------------

When I started work on my first fragmovie, I had a bunch of properly sorted and named demos with cool frags but I realized I had a lot more cool frags scattered among a huge amount of demo files.

Those others demos were not renamed (time-stamp, type of frags) nor sorted. I was definitely not going to watch them, so I had to find another way.

Whenever I had made a cool frag, I was using the same exact chat message every time because I had a chat bind for said message, so all I did was press a key. I then realized that the chat message is a marker for cool frags. If I could find when those messages were printed, I could find the cool frags! Thus, the first `Cut by Pattern` feature was born: `Cut by Chat`.

Typical GUI Usage Scenario
--------------------------

The typical day-to-day usage scenario for UDT is the one for which the interface has been optimized: `Cut by Chat`.

It supposes the following:

1. You have a chat bind you use when you make cool frags or something funny/unusual/movie-worthy happens. Example: you have `bind space "say HAHA! YOU ARE DEAD!"` in your Quake config and you press space when something cool happens.
2. You have configured UDT's `Chat` rules under the `Patterns` tab and the general stuff under the `Settings` tab.

After your gaming session...

1. Drag'n'drop the new demos onto UDT.
2. Select `Patterns`, then `Chat`.
3. Select all demos (can click the `Demo List` list box and press Ctrl+A), then click click `Cut!`.
4. You now have a cut demo for each cool thing that happened in the folder you specified.
5. Review the cut demos to decide what you keep, rename and move those you wish to keep to the appropriate folder.

With that workflow, you minimize the amount of work needed to find and keep what's worthy after your play sessions. Less work for the players, more cool stuff for the movie-makers.

Time Formats
------------

* All timestamps in UDT are server times, not match or warm-up times. The time you see on the Quake "clock" is not the server time.
* The syntax `$(x)` means format the variable `x` into a string.
* The syntax `$02(x)` means format the variable `x` with with at least 2 digits (add leading zeroes if necessary to get 2 digits).
* The time format used everywhere in the GUI for read-only data is `$(minute):$02(second)`.
* The time formats accepted for input ate `$(minute):$(second)` and `$(total_seconds)`. Any amount of leading zeroes is acceptable. Example: "107" is the same as "1:47".
* For file names, the format is `$(minute)$02(second)` because using `:` in file names is not valid. Example: time "1:07" becomes "107".
* In other words, the last 2 digits of a file name timestamp are always the seconds while the ones prior are the minutes.

Pattern Search
--------------

Here are the patterns you can look for in demos:

| Pattern          | Player<sup>[1]</sup>   | Description |
|:-----------------|:-----------------------|:------------|
| Chat             | All                    | String patterns in chat messages |
| Frag sequences   | Selected               | Sequences of frags happening within a certain duration |
| Mid-air frags    | Selected               | Rocket and BFG frags where the victim was airborne |
| Multi-frag rails | Selected               | Railgun frags killing 2 or more players |
| Flag captures    | Selected               | Flag runs: the player picks up the flag and captures it |
| Flick rails      | Selected               | Railgun frags where the attacker's view angles changed very fast right before the killing shot |
| Matches          | None                   | Each match, from pre-match count-down start to post-match intermission (scoreboard screen) end |

1. To which player(s) is the pattern matching applied to?
2. `Selected` &mdash; see the `Player Selection` rules in the `Pattern Search` tab

Maximum Thread Count
--------------------

UDT GUI and most of the command-line tools expose a `Maximum Thread Count` option which, as the name implies, *only* acts as an upper bound.  
The actual thread count used by UDT for processing a job is based on the number of files, total byte count, CPU core count and the `Maximum Thread Count` the user specified.  
Please note that given the way UDT works, augmenting the thread count will only yield a performance increase when reading the demos from an SSD.  
If you have a standard hard drive and not an SSD, make sure to leave the `Maximum Thread Count` to 1.

Here are the throughputs (in MB/s), for each thread count, when parsing with all analyzers enabled:

| Machine Config         | 1    | 2     | 3     | 4     | 5     | 6     | 7     | 8                  |
|:-----------------------|-----:|------:|------:|------:|------:|------:|------:|-------------------:|
| i7 2600K<sup>[1]</sup> | 51   | 101   | 141   | 169   |       |       |       |                    |
| i7 3770K<sup>[2]</sup> | 60   | 120   | 167   | 216   |       |       |       |                    |
| i7 5960X<sup>[3]</sup> | 61   | 122   | 180   | 235   | 306   | 347   | 382   | 400+<sup>[4]</sup> |

1) Intel Core i7 2600K CPU (4 cores) + Intel X25-M SSD  
2) Intel Core i7 3770K CPU (4 cores) + Samsung 850 EVO SSD  
3) Intel Core i7 5960X CPU (8 cores) + Samsung 850 EVO SSD  
4) The SSD seems to freeze for a bit thus slowing things down

Build date: April 18, 2016.

While the performance increase isn't perfectly linear, the benefits are far from negligible for users with SSDs.

Technical Notes
---------------

The technical notes have their own page [here](https://github.com/mightycow/uberdemotools/blob/develop/TECHNICAL_NOTES.md).

Building from Source
--------------------

The guide for all supported OS and compiler combinations is [here](https://github.com/mightycow/uberdemotools/blob/develop/BUILD.md).

Custom Parsing API
------------------

The developer's guide to the custom parsing API is [here](https://github.com/mightycow/uberdemotools/blob/develop/CUSTOM_PARSING.md).

Contact
-------

GitHub user: [mightycow](https://github.com/mightycow)  
GitHub project page: [uberdemotools](https://github.com/mightycow/uberdemotools)

myT @ [Discord](https://discord.me/CPMA)
myT @ [ESR](http://esreality.com/?a=users&user_id=37287)

Official ESR forum thread: [UDT @ ESR](http://www.esreality.com/post/2691563/uberdemotools/)  
No account is required to post comments.

Thanks
------

In alphabetical order:
* AsphyxEvents
* cra
* Danmer
* gaiia
* JackBender
* Naper
* oranjemetal
* pakao
* Sab0o
* santile
* Terifire

License
-------

The entire source code in this release is covered by the GPL.  
See [COPYING.txt](https://github.com/mightycow/uberdemotools/blob/master/UDT_DLL/COPYING.txt) for the GNU GENERAL PUBLIC LICENSE.

Uber Demo Tools (UDT) is Copyright (C) 2011-2018 Gian 'myT' Schellenbaum.  
It is based on the Quake III Arena source code and the Challenge Quake 3 source code.

The Quake III Arena source code is Copyright (C) 1999-2005 Id Software, Inc.  
The Challenge Quake 3 source code is Copyright (C) 2006-2009 Kevin H 'arQon' Blenkinsopp.
