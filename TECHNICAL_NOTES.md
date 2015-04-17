[UDT](https://github.com/mightycow/uberdemotools) - Technical Notes
===================================================================

<<<<<<< HEAD
'C+11' - a subset of C++11
--------------------------

The C++ parts of **UDT** are written in C++11 but since it uses a subset of C++11, I'll call it *C+11* just because I can. For **UDT**, here are some of the basic rules:
=======
*C+11* - a subset of C++11
--------------------------

The C++ parts of UDT are written in C++11 but since it uses a subset of C++11, I'll call it *C+11* just because I can. For UDT, here are some of the basic rules:

>>>>>>> develop
* No C++ exceptions
* Because C++ exceptions are disabled, the STL is strictly avoided and so are operator new and delete.
* No RTTI
* No multiple inheritance, no virtual inheritance
* Virtual functions are avoided when possible.
* No third-party dependencies: only C run-time library functions and core OS functions allowed
* Always prefer POD types and use arrays as the default data structure unless the data access patterns truly don't favor sequential access.
* Templates outside of container classes shouldn't really be used except for trivial cases and/or static functions.
<<<<<<< HEAD
* The one feature I enabled C++11 compilation for is the *override* keyword. Hard to believe we had to wait that long to have that keyword.

Here are some general reasons why not to use C++ exceptions (they don't all apply to **UDT**):
=======
* The one feature I enabled C++11 compilation for is the override keyword. Hard to believe we had to wait that long to have that keyword.

Here are some general reasons why not to use C++ exceptions (they don't all apply to UDT):

>>>>>>> develop
* You have to live with reduced performance.
* The error-handling model isn't very good.
* Not using C++ exceptions is a must if you want people to be able to use your shared libraries from as many other languages as possible.
* Exceptions are not necessarily available on all platforms (just like longjmp/setjmp aren't on all platforms if you write C code).
* Writing exception-safe C++ code is very tricky anyway (much more than you'd think).

.NET Framework 4.0 vs 3.5
-------------------------

<<<<<<< HEAD
Because *Windows 7* ships with *.NET Framework 3.5*, it makes sense to really try to make sure your .NET application can run on version 3.5 if you don't have to sacrifice too much for it.

Unfortunately, the GUI library *WPF* (which the **UDT** GUI uses) has terrible font rendering in *.NET Framework 3.5* but can look decent in version 4.0.
=======
Because **Windows 7** ships with **.NET Framework 3.5**, it makes sense to really try to make sure your .NET application can run on version 3.5 if you don't have to sacrifice too much for it.

Unfortunately, the GUI library **WPF** (which the UDT GUI uses) has terrible font rendering in **.NET Framework 3.5** but can look decent in version 4.0.
>>>>>>> develop

New Huffman decoder and encoder
-------------------------------

<<<<<<< HEAD
The *Quake 3* and *Quake Live* demo files don't define the Huffman tree to use. Instead, they always use the one tree *id Software* defined in the *Quake 3* source code.
=======
The **Quake 3** and **Quake Live** demo files don't define the Huffman tree to use. Instead, they always use the one tree *id Software* defined in the **Quake 3** source code.
>>>>>>> develop

Observations about *id*'s Huffman code:
- The symbol length is 8 bits.
- The maximum code length is 11 bits: long enough that a single look-up table would be too large for proper cache use but short enough that you can make a decoder that requires a maximum of 2 look-ups.
- Their code did things the naïve way: literally traversing a binary tree (probably because they were in a hurry). This is really bad decoder-wise because the amount of memory reads and cache misses is high.
- The tree is non-canonical, which prevents some decoder optimizations.
- The tree used doesn't seem that well suited to their demo data in general: 72% of all encoded symbols use codes of length <= 8 bits (data gathered on 1 GB of real demos). But since that tree is implicitly defined, that's not something we can improve on.

###### The new encoder
Since the symbol length is 8 bits, all you need for the encoder is a single look-up table with 256 entries (the symbol is the index) where each entry contains the code word and its bit length. The new implementation is thus trivial and substanitally faster.

###### The new decoder
The decoder basically works in 2 steps, where step 2 only is required when the code length is >= 8 (which is the less frequent case):
<<<<<<< HEAD
=======

>>>>>>> develop
1. The first step is to read 8 bits from the bit stream into byte variable *Cstart* and read from the first look-up (using *Cstart* as an index) what the code word length is. If it's 0, its is unknown and we must read 3 more bits in step 2. If not 0, we can find the symbol with a simple look-up (using *Cstart* as an index).
2. We read 3 more bits from the bit stream into byte variable *Cend*. We look up the second-level table index (using *Cstart* as an index) into variable *LongCodeIndex*. We now look up the code length and symbol (using *LongCodeIndex* + *Cend* as an index).

You might notice that the data in some of the look-ups can be packed together or kept split. The decision really is based on 2 simple factors in practise:
<<<<<<< HEAD
=======

>>>>>>> develop
1. Data that is always accessed together should be bundled together and data that isn't should be kept separate for performance reasons. The idea is to keep the most frequently accessed data in the cache.
2. Keeping data in separate tables can be simpler because of pointer arithmetic. Not a good engineering argument for the best case, but sometimes simplicity (and/or lack of time) trumps others concerns.

To recap:
<<<<<<< HEAD
=======

>>>>>>> develop
- First-level data: 256 code lengths (0 if not known), 256 symbols, 256 second-level indices (only used when the code length is 0)  
Indexing: bits 0-7 from the current position in the bit stream
- Second-level data: 8N code lengths, 8N symbols (where N is the number of code words whose length is > 8)  
Indexing: second-level index + bits 8-10 from the current position in the bit stream

###### Results
<<<<<<< HEAD
After replacing *id*'s code with the new Huffman decoder and encoder, the **total** run-time performance for demo analysis and cutting has **more than doubled**. It is therefore easy to infer the following 2 statements:
1. Huffman decode was a very important fraction of the total cost of reading a demo.
2. The new code that does that is much faster.

Very brief overview of what's in a *Quake 3* or *Quake Live* demo
-----------------------------------------------------------------
=======
After replacing *id*'s code with the new Huffman decoder and encoder, the **total run-time** for demo analysis and cutting is **less than half** what it was before the change. It is therefore easy to infer the following 2 statements:

1. Huffman decode was a very important fraction of the total cost of reading a demo.
2. The new code that does that is much faster.

Very brief overview of what's in a **Quake 3** or **Quake Live** demo
---------------------------------------------------------------------
>>>>>>> develop

Quake demos are sequences of messages that the server sends to the client. It is, in essence, a dump of the client's incoming network stream.

Here are the 3 message types we care about:

| Message type | Frequency                           | Description |
|:-------------|:------------------------------------|:-------------
| Game state   | Only sent when a map is (re-)loaded | Baseline entities + config strings |
| Snapshot     | Sent continuously                   | Delta-encoded entity states |
| Command      | Sent irregularly                    | A command encoded as a string |

<<<<<<< HEAD
Command string example:  
```
cs N "\key1\value1\key2\value2"
```
=======
Command string example:
> cs N "\key1\value1\key2\value2"

>>>>>>> develop
This command tells the client to replace the config string at index N with what follows in the quotes.

| Game state types  | Description |
|:------------------|:-------------
| Baseline entities | The initial value entities at given indices should have
| Config strings    | Generic null-terminated C strings encoding various state (match start time, score, player names, map name, etc)

There are many specifics for things such as message and command sequence numbers, how there are entity events and event entities, the way Huffman compression is used, the per-field delta-encoding of player states and entity states, etc.
I will not cover those things because this is not a primer nor a guide about Quake demo parsing. I'll just say that if the devil's in the details, then that protocol is definitely from hell.

However, I want to make clear the following 2 points:
<<<<<<< HEAD
1. Cutting a demo isn't just copying the sequence of messages with the timestamps you care about. Because the data is delta-encoded, it would simply not work. Now you know why *demo cutting* isn't almost instant.
2. The data the client receives isn't enough to *fully* simulate the game on the client: it's only enough to display it properly.
   Example #1: The client receives separate information from the server that a player was fragged and that the score changed.  
   Example #2: When a rocket is shot, the rocket entity doesn't tell you who shot the rocket. And when a rocket explodes, the entity representing the explosion doesn't tell you what rocket entity is linked.
=======

1. Cutting a demo isn't just copying the sequence of messages with the timestamps you care about. Because the data is delta-encoded, it would simply not work. Now you know why *demo cutting* isn't almost instant.
2. The data the client receives isn't enough to *fully* simulate the game on the client: it's only enough to display it properly.
   Example #1: The client receives separate information from the server that a player was fragged and that the score changed.  
   Example #2: When a rocket is shot, the rocket entity doesn't tell you who shot the rocket. And when a rocket explodes, the entity representing the explosion doesn't tell you what rocket entity is linked nor who shot the rocket.
>>>>>>> develop

Memory allocation
-----------------

<<<<<<< HEAD
All memory allocations are done with malloc/free or the OS functions for virtual memory (VirtualAlloc/VirtualFree on Windows, mmap/munmp/mprotect on Linux). Because classes with non-trivial constructors and destructors are used, operator placement new is used. Manual destructor calls are used in udtVMScopedStackAllocator only.
* malloc is only used for creating core library instances (udtParserContext, udtParserContextGroup) and a few other places
* virtual memory OS functions are used for pretty much everything else (i.e. the vast majority of all allocations) through udtVMLinearAllocator, a virtual-memory backed linear allocator

I really like the memory allocation systems games use where they allocate a big block of memory at start-up and then never ask the OS to allocate more memory. Every memory allocation is a failure point of the allocation and not having to worry about it because all failure points disappeared is great. Unfortunately, desktop applications can't really aggressively commit huge chunks of memory like that and play nice with the rest of the applications running on the system. One way to use a similar approach without actually committing a large amount of physical memory is to reserve memory address space and commit physical pages when needed. Pretty much everything in **UDT** is stored into arrays, and they all use the same virtual-memory backed linear allocator.

When running a batch job, **UDT** will reserve all the memory address space up front to avoid crashing during jobs.
The amount of address space reserved is:
> file_count * ( bytes_needed(parser) + sum( bytes_needed(analyzers) ) )

Here are a few things **UDT** does to reduce memory address space consumption as well as physical memory waste:
=======
All memory allocations are done with malloc/free or the OS functions for virtual memory (VirtualAlloc/VirtualFree on Windows, mmap/munmp/mprotect on Linux). Because classes with non-trivial constructors and destructors are used, operator placement new is used. Most manual destructor calls are done through `udtVMScopedStackAllocator`, but there are some other places where they had to be used.

* malloc is only used for creating core library instances (`udtParserContext`, `udtParserContextGroup`) and in a few other places.
* Virtual memory OS functions are used for pretty much everything else (i.e. the vast majority of all allocations) through `udtVMLinearAllocator`, a virtual-memory backed linear allocator.

I really like the memory allocation systems games use where they allocate a big block of memory at start-up and then never ask the OS to allocate more memory. Every memory allocation is a failure point of the application and not having to worry about it because all failure points disappeared is great. Unfortunately, desktop applications can't really aggressively commit huge chunks of memory like that and play nice with the rest of the applications running on the system. One way to use a similar approach without actually committing a large amount of physical memory is to reserve memory address space and commit physical pages when needed. Pretty much everything in UDT is stored into arrays, and they all use the same virtual-memory backed linear allocator.

When running a batch job, UDT will try to make as many of the memory address space reservations up front to avoid crashing during jobs.
The amount of address space reserved is thus roughly:
> file_count * ( bytes_needed(parser) + sum( bytes_needed(analyzers) ) )

Here are a few things UDT does to reduce memory address space consumption as well as physical memory waste:

>>>>>>> develop
* When running batch jobs, the GUI application will split the file list in smaller file lists for the library to process.
* Only one parser instance is used per thread.
* Only one analyzer instance is used per thread (the output data from analysis gets stored in the same array, not matter how many demos are processed).

Unlike standard array implementations, an array backed by virtual memory where enough memory is reserved offers highly interesting properties:
<<<<<<< HEAD
=======

>>>>>>> develop
* When the array grows, you only have to commit physical pages. You never need to do this thing where you allocate new memory, copy the data from the old buffer to the new buffer and free the old buffer.
* The start address of the array is always the same.
* Because the commit granularity is 4 KB (the size of a physical memory page), the maximum amount of wasted memory per linear allocator is 4095 bytes.
* The more data you write to the array, the less relative memory waste you have!
* As a bonus, you know how much virtual and physical memory you use and you know how much physical memory you're wasting.

The amount of physical memory wasted is:
> 100 * (1 - used_bytes / committed_bytes)

Multi-threading
---------------

When it came to threading for improving batch processing performance, I was confronted with the 2 choices:
<<<<<<< HEAD
1. Let the library's user create handle thread creation and clean-up, distribute work across the threads etc.
2. Have the library handle all those things so that both the GUI and command-line tools could leverage the same logic and so could other users.
The important thing is that option #2 doesn't really have to preclude option #1 from being available. If the user wants its own logic for creating and releasing thread resources, assigning demos to threads etc, they can by simply calling the same functions and setting the maximum thread argument to 1.

Here's how it works in **UDT**:
=======

1. Let the library's user create handle thread creation and clean-up, distribute work across the threads etc.
2. Have the library handle all those things so that both the GUI and command-line tools could leverage the same logic and so could other users.

The important thing is that option #2 doesn't really have to preclude option #1 from being available. If the user wants its own logic for creating and releasing thread resources, assigning demos to threads etc, they can by simply calling the same functions and setting the maximum thread argument to 1.

Here's how it works in UDT:

>>>>>>> develop
* Every function is synchronous: it returns when it's done with its task or something failed before that could happen.
* The library decides how many threads to launch based on the following data to make sure we don't spawn more threads than necessary:
  * The user's supplied maximum thread count
  * The amount of CPU cores available
  * The amount of demos to process
  * The amount of data (demo file sizes) to process
* If the final thread count decided is 1, all the work is done in the thread of the original function call.
* If the final thread count decided is greater than 1, all the work is done in new threads and the thread of the original function call will join on those new threads.

Note that for crash handling in C#, there is an annoying problem when using unmanaged code that creates its own threads: you can't catch exceptions of unmanaged threads.  
<<<<<<< HEAD
So in practise, if there is a crash in *UDT.dll*:
=======
So in practise, if there is a crash in `UDT.dll`:

>>>>>>> develop
* If the final thread count was 1: the exception gets caught and the application can close with the custom handler and hopefully write down a useful stack trace of the related C# thread.
* If the final thread count was greater than 1: the exception does not get caught and Windows will use its own handler.

Memory allocation tracking
--------------------------

<<<<<<< HEAD
The **UDT** code base has a pretty simple way of tracking memory allocators without using any synchronization primitive and with only 1 extra memory allocation per thread:
=======
The UDT code base has a pretty simple way of tracking memory allocators without using any synchronization primitive and with only 1 extra memory allocation per thread:

>>>>>>> develop
* Every memory allocator contains a node of an intrusive doubly linked list data structure (the node contains 2 pointers only).
* There's a doubly linked list of memory allocators per thread.
* Accessing that list is done through TLS (thread-local storage).
* Every time a memory allocator is constructed, it is added to the thread's list.
* Every time a memory allocator is destructed, it is removed from the thread's list.
* The doubly linked list is lazily allocated and created on first access (1 *malloc* call per thread).
* The doubly linked list is freed once per thread (1 *free* call per thread when the API function is done).
* When each job thread is done, it traverses its own allocator list and sums up the stats in its data return slot.
* When the API function is done waiting on the job threads, it sums up all the stats from the other threads with those of its own thread.
