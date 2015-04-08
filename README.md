# [UDT](https://github.com/mightycow/uberdemotools) - Uber Demo Tools

**UDT** is a set of tools for analyzing and cutting *Quake 3* (.dm_68) and *Quake Live* (.dm_73 and .dm_90) demo files.

The main features are:
- Extracting and displaying information (examples: map name, player names and teams, game mode, etc)
- Cutting by time: creating a new demo file that is a time sub-range of the original demo file for easier processing or demo viewing
- Splitting demos: given a demo with multiple map changes, create a new demo file per map change (plus one for the stuff before the first map change)
- Cutting by patterns: given rules defined by the user, find matching events and cut demos around the times of said events

What's in the project?
----------------------

The project is currently comprised of 3 parts:  
1. A shared library, UDT_DLL, with a C interface, written in C++. Supported OSes: Windows, Linux.
2. A set of command-line tools, UDT_cutter and UDT_splitter, written in C++. Supported OSes: Windows, Linux.
3. A GUI application, UDT_GUI, written in C#. Supported OSes: Windows only (requires the .NET Framework).

Overview of the binaries

| Project name | Type        | Language | Windows | Linux | Dependencies | Description |
|:-------------|:------------|:--------:|:-------:|:-----:|:------------:|:-------------
| UDT_DLL      | Library     | C++      | X       | X     |              | Shared library that does the actual cutting and analysis work |
| UDT_cutter   | Application | C++      | X       | X     |              | Command-line application for cutting demos by time or chat patterns (*Cut by Chat*) |
| UDT_splitter | Application | C++      | X       | X     |              | Command-line application for splitting demos with at least one map change into individual demos with no map changes |
| UDT_GUI      | Application | C#       | X       |       | [.NET Framework 4.0 Client Profile](http://www.microsoft.com/en-us/download/details.aspx?id=24872) | GUI application for demo analysis, information display, cutting by time or various patterns |

Installation
------------

No installation is required for any of the binaries.  
**UDT_DLL**, **UDT_cutter** and **UDT_splitter** have no third-party dependencies.

**UDT_GUI** requires [*.NET Framework 4.0 Client Profile*](http://www.microsoft.com/en-us/download/details.aspx?id=24872) at a minimum to run.  
If you have *Windows 8* or later, then you should have it pre-installed with the OS unless you changed system settings.

Pre-compiled binaries are hosted here for your convenience: [official binaries](http://giant.pourri.ch/snif.php?path=UDT/)

How did UDT come to be?
-----------------------

When I started work on my first fragmovie, I had a bunch of properly sorted and named demos with cool frags but I realized I had a lot more cool frags scattered among a huge amount of demo files.

Those others demos were not renamed (time-stamp, type of frags) nor sorted. I was definitely not going to watch them, so I had to find another way.

Whenever I had made a cool frag, I was using the same exact chat message every time because I had a chat bind for said message, so all I did was press a key. I then realized that the chat message is a marker for cool frags. If I could find when those messages were printed, I could find the cool frags! Thus, the first *Cut by Pattern* feature was born: *Cut by Chat*.

Typical UDT GUI usage scenario
------------------------------

The typical usage scenario for **UDT** is the one for which the interface has been optimized: *Cut by Chat*.

It supposes the following:  
1. You have a chat bind you use when you make cool frags or something funny/unusual/movie-worthy happens. Example: you have /bind space "say HAHA! YOU ARE DEAD!" and you press space when something cool happens.
2. You have configured **UDT**'s *Global Chat* rules under the *Patterns* tab and the general stuff under the *Settings* tab

After your gaming session...  
1. Drag'n'drop the new demos onto **UDT**.
2. Select *Patterns*, then *Global Chat*.
3. Select all demos (can click the *Demo List* list box and press Ctrl+A), then click click *Cut!*.
4. You now have a cut demo for each cool thing that happened in the folder you specified.
5. Review the cut demos to decide what you keep, rename and move those you wish to keep to the appropriate folder.

With that workflow, you minimize the amount of work needed to find and keep what's worthy after your play sessions. Less work for the players, more cool stuff for the movie-makers.

[Technical Notes](https://github.com/mightycow/uberdemotools/blob/master/TECHNICAL_NOTES.md)
-------

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
* gaiia
* JackBender
* Naper
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
