# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

UDT is a set of tools for analyzing, cutting and modifying **Quake 3** and **Quake Live** demo files.

The main features are:

- Extracting and displaying information (examples: map name, player names and teams, game mode, etc)
- Cutting by time: creating a new demo file that is a time sub-range of the original demo file for easier processing or demo viewing
- Splitting demos: given a demo with multiple map changes, create a new demo file per map change (plus one for the stuff before the first map change)
- Cutting by patterns: given rules defined by the user, find matching events and cut demos around the times of said events
- Time shifting demos: shifting the non-first-person players back in time (a sort of anti-lag)
- Merging demos: given multiple demos from the same match recorded by different players, create a new demo with more complete information

Official UDT binaries
---------------------

[Grab them here](http://giant.pourri.ch/snif.php?path=UDT/).

What's in the project?
----------------------

The project is currently comprised of 3 parts:

1. A shared library, `UDT_DLL`, with a C interface, written in C++. Supported OSes: Windows, Linux
2. A set of command-line tools, `UDT_cutter`, `UDT_splitter`, `UDT_timeshifter`, `UDT_merger` and `UDT_json`, written in C++. Supported OSes: Windows, Linux
3. A GUI application, `UDT_GUI`, written in C#. Supported OSes: Windows only (requires the .NET Framework)

Overview of the binaries

| Project name    | Type/Language      | Platforms     | Dependencies  | Description  |
|:----------------|:------------------:|:-------------:|:-------------:|:------------:|
| UDT_DLL         | Library<br>C++     | Windows Linux |  | Shared library that does the actual cutting and analysis work |
| UDT_cutter      | Application<br>C++ | Windows Linux |  | Command-line application for cutting demos by time or chat patterns (*Cut by Chat*) |
| UDT_splitter    | Application<br>C++ | Windows Linux |  | Command-line application for splitting demos with at least one map change into individual demos with no map changes |
| UDT_timeshifter | Application<br>C++ | Windows Linux |  | Command-line application for shifting the non-first-person players back in time (a sort of anti-lag) |
| UDT_merger      | Application<br>C++ | Windows Linux |  | Command-line application for merging multiple demos into one |
| UDT_json        | Application<br>C++ | Windows Linux |  | Command-line application for exporting analysis data to JSON files |
| UDT_GUI         | Application<br>C#  | Windows       | [.NET Framework 4.0 Client Profile](http://www.microsoft.com/en-us/download/details.aspx?id=24872) | GUI application for demo analysis, information display, cutting by time or various patterns, time-shifting and merging |

Supported demo formats
----------------------

Read-only demos can be analyzed.  
Read/write demos can be analyzed, modified and cut.

| Demo file extension | Game version | Support level |
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
`UDT_DLL`, `UDT_cutter`, `UDT_splitter`, `UDT_timeshifter`, `UDT_merger` and `UDT_json`` have no third-party dependencies.

`UDT_GUI` requires [**.NET Framework 4.0 Client Profile**](http://www.microsoft.com/en-us/download/details.aspx?id=24872) at a minimum to run.  
If you have **Windows 8** or later, then you should have it pre-installed with the OS unless you changed system settings.

How did UDT come to be?
-----------------------

When I started work on my first fragmovie, I had a bunch of properly sorted and named demos with cool frags but I realized I had a lot more cool frags scattered among a huge amount of demo files.

Those others demos were not renamed (time-stamp, type of frags) nor sorted. I was definitely not going to watch them, so I had to find another way.

Whenever I had made a cool frag, I was using the same exact chat message every time because I had a chat bind for said message, so all I did was press a key. I then realized that the chat message is a marker for cool frags. If I could find when those messages were printed, I could find the cool frags! Thus, the first `Cut by Pattern` feature was born: `Cut by Chat`.

Typical UDT GUI usage scenario
------------------------------

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

Time formats
------------

* All timestamps in UDT are server times, not match or warm-up times. The time you see on the Quake "clock" is not the server time.
* The syntax `$(x)` means format the variable `x` into a string.
* The syntax `$02(x)` means format the variable `x` with with maximum 2 digits and add leading zeores if necessary to get 2 digits.
* The time format used everywhere in the GUI for read-only data is `$(minute):$02(second)`.
* The time formats accepted for input ate `$(minute):$(second)` and `$(total_seconds)`. Any amount of leading zeroes is acceptable. Example: "107" is the same as "1:47".
* For file names, the format is `$(minute)$02(second)` because using `:` in file names is not valid. Example: time "1:07" becomes "107".

Cutting by patterns
-------------------

Here are the patterns you can look for in demos:

| Pattern          | Player<sup>[1]</sup>   | Description |
|:-----------------|:-----------------------|:------------|
| Chat             | All                    | String patterns in chat messages |
| Frag sequences   | Selected               | Sequences of frags happening within a certain duration |
| Mid-air frags    | Selected               | Rocket and BFG frags where the victim was airborne |
| Multi-frag rails | Selected               | Railgun frags killing 2 or more players |
| Flag captures    | Selected               | Flag runs: the player picks up the flag and captures it |
| Flick rails      | Selected               | Railgun frags where the attacker's view angles changed very fast right before the killing shot |

1. To which player(s) is the pattern matching applied to?
2. `Selected` &mdash; see the `Player Selection` rules in the `Cut by Patterns` tab

Technical Notes
---------------

The technical notes have their own page [here](https://github.com/mightycow/uberdemotools/blob/master/TECHNICAL_NOTES.md).

Contact
-------

GitHub user: [mightycow](https://github.com/mightycow)  
GitHub project page: [uberdemotools](https://github.com/mightycow/uberdemotools)

myT @ QuakeNet on IRC  
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
* pakao
* Sab0o
* santile
* Terifire

License
-------

The entire source code in this release is covered by the GPL.  
See [COPYING.txt](https://github.com/mightycow/uberdemotools/blob/master/UDT_DLL/COPYING.txt) for the GNU GENERAL PUBLIC LICENSE.

Uber Demo Tools (UDT) is Copyright (C) 2011-2015 Gian 'myT' Schellenbaum.  
It is based on the Quake III Arena source code and the Challenge Quake 3 source code.

The Quake III Arena source code is Copyright (C) 1999-2005 Id Software, Inc.  
The Challenge Quake 3 source code is Copyright (C) 2006-2009 Kevin H 'arQon' Blenkinsopp.
