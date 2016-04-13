[UDT](https://github.com/mightycow/uberdemotools) - Technical Notes
===================================================================

*C+11* - a subset of C++11
--------------------------

The C++ parts of UDT are written in C++11 but since it uses a subset of C++11, I'll call it *C+11* just because I can. For UDT, here are some of the basic rules:

* No C++ exceptions
* Because C++ exceptions are disabled, the STL is strictly avoided and so are operator new and delete.
* No RTTI
* No multiple inheritance, no virtual inheritance
* Virtual functions are avoided when the functions are called a lot.
* No third-party dependencies: only C run-time library functions and core OS functions allowed
* Always prefer POD types and use arrays as the default data structure unless the data access patterns truly don't favor sequential access.
* Templates outside of container classes shouldn't really be used except for trivial cases and/or static functions.
* The one feature I enabled C++11 compilation for originally is the override keyword. Hard to believe we had to wait that long to have that language feature.

Here are some general reasons why not to use C++ exceptions (they don't all apply to UDT):

* You have to live with reduced performance.
* The error-handling model isn't very good.
* Not using C++ exceptions is a must if you want people to be able to use your shared libraries from as many other languages as possible.
* Exceptions are not necessarily available on all platforms (just like longjmp/setjmp aren't on all platforms if you write C code).
* Writing exception-safe C++ code is very tricky anyway (much more than you'd think).

.NET Framework 4.0 vs 3.5
-------------------------

Because **Windows 7** ships with **.NET Framework 3.5**, it makes sense to really try to make sure your .NET application can run on version 3.5 if you don't have to sacrifice too much for it.

Unfortunately, the GUI library **WPF** (which the UDT GUI uses) has terrible font rendering in **.NET Framework 3.5** but can look decent in version 4.0.

New Huffman decoder and encoder
-------------------------------

The **Quake 3** and **Quake Live** demo files don't define the Huffman tree to use. Instead, they always use the one tree *id Software* defined in the **Quake 3** source code.

Observations about *id*'s Huffman code:
- The symbol length is 8 bits: a single look-up table with 256 entries is enough for the encoder.
- The maximum code length is 11 bits: a single look-up table with 2048 entries is enough for the decoder, which is reasonable.
- Their code did things the naïve way: literally traversing a binary tree (probably because they were in a hurry). This is really bad decoder-wise because the amount of memory reads and cache misses is high.
- The tree is non-canonical, which prevents some decoder optimizations. Fortunately, the maximum code length is short so that doesn't get in the way.
- The tree used doesn't seem that well suited to their demo data in general: 72% of all encoded symbols use codes of length <= 8 bits (data gathered on 1 GB of real demos). Since the Huffman tree is implicitly defined, we can't improve the compression rate while retaining compatibility.

|         | Entries | Index With | Entry Data (16 bits) |
|:--------|--------:|:----------:|----------------------|
| Encoder | 256     | Symbol     | code word (12 bits)<br>code word length (4 bits) |
| Decoder | 2048    | Code Word  | symbol (8 bits)<br>code word length (8 bits)     |

Previously, the UDT decoder used a 2-phase look-up system. You can read up on it in earlier versions of this document.  
While it is using more memory, the single look-up system is both simpler and faster.

Very brief overview of what's in a **Quake 3** or **Quake Live** demo
---------------------------------------------------------------------

Quake demos are sequences of messages that the server sends to the client. It is, in essence, a dump of the client's incoming network stream.

Here are the 3 message types we care about:

| Message type | Frequency                           | Description |
|:-------------|:------------------------------------|:-------------
| Game state   | Only sent when a map is (re-)loaded | Baseline entities + config strings |
| Snapshot     | Sent continuously                   | Delta-encoded entity states |
| Command      | Sent irregularly                    | A command encoded as a string |

Command string example:
> cs N "\key1\value1\key2\value2"

This command tells the client to replace the config string at index N with what follows in the quotes.

| Game state types  | Description |
|:------------------|:-------------
| Baseline entities | The initial value entities at given indices should have
| Config strings    | Generic null-terminated C strings encoding various state (match start time, score, player names, map name, etc)

There are many specifics for things such as message and command sequence numbers, how there are entity events and event entities, the way Huffman compression is used, the per-field delta-encoding of player states and entity states, etc.
I will not cover those things because this is not a primer nor a guide about Quake demo parsing. I'll just say that if the devil's in the details, then that protocol is definitely from hell.

However, I want to insist on the following 2 points:

1. Cutting a demo isn't just copying the sequence of messages with the timestamps you care about. Because the data is delta-encoded, it would simply not work. Now you know why *demo cutting* isn't almost instant.
2. The data the client receives isn't enough to *fully* simulate the game on the client: it's only enough to display it properly.
   Example #1: The client receives separate information from the server that a player was fragged and that the score changed.  
   Example #2: When a rocket is shot, the rocket entity doesn't tell you who shot the rocket. And when a rocket explodes, the entity representing the explosion doesn't tell you what rocket entity is linked nor who shot the rocket.

Memory allocation
-----------------

All memory allocations are done with malloc/free or the OS functions for virtual memory (VirtualAlloc/VirtualFree on Windows, mmap/munmp/mprotect on Linux). Because classes with non-trivial constructors and destructors are used, operator placement new is used. Most manual constructor and destructor calls are done through `udtVMScopedStackAllocator`, but there are some other places where they had to be used.

* malloc is only used for creating core library instances (`udtParserContext`, `udtParserContextGroup`) and in a few other places.
* Virtual memory OS functions are used for pretty much everything else (i.e. the vast majority of all allocations) through `udtVMLinearAllocator`, a virtual-memory backed linear allocator.

Pretty much everything in UDT is stored into arrays (`udtVMArray`), and they all use the same virtual-memory backed linear allocator (`udtVMLinearAllocator`).

Here are a few things UDT does to reduce memory address space consumption as well as physical memory waste:

* When running batch jobs, the GUI application will split the file list in smaller file lists for the library to process.
* Only one parser instance is used per thread.
* Only one analyzer instance is used per thread and per analyzer type (the output data from analysis gets stored in the same array(s), no matter how many demos are processed).
* That is, if an analysis job runs on 4 threads with 8 analyzers, only 32 analyzer instances will be created in total, no matter how many files get processed.

Here are some of the interesting properties of arrays when using the virtual memory system:

* When growing and relocation isn't needed, you just commit the amount of memory pages needed.
* Because the commit granularity is 4 KB (the size of a physical memory page), the maximum amount of wasted physical memory per linear allocator is 4095 bytes.
* When growing and relocation is needed, the new address space can be a fair amount bigger than the old one to make relocations rare, but the max. physical waste remains low.
* The more data you write to the array, the less relative memory waste you have. (Which is why analyzer arrays hold results for all the demos processed in the same thread.)
* You know how many pages are reserved, how many pages are commited and how many bytes are used. Therefore, you know exactly how much physical memory you're wasting and you know how your systems behave.

The amount of physical memory wasted is:
> 100 * (1 - used_bytes / committed_bytes)

Multi-threading
---------------

When it came to threading for improving batch processing performance, I was confronted with the following 2 choices:

1. Let the library's user handle thread creation and clean-up, distribute work across the threads etc.
2. Have the library handle all those things so that both the GUI and command-line tools could leverage the same logic and so could other users.

The important thing is that option #2 doesn't really have to preclude option #1 from being available. If the user wants its own logic for creating and releasing thread resources, assigning demos to threads etc, they can by simply calling the same functions and setting the maximum thread count option to 1.

Here's how it works in UDT:

* Every function is synchronous: it returns when it's done with its task or something failed before that could happen.
* The library decides how many threads to launch based on the following data to make sure we don't spawn more threads than necessary:
  * The user's supplied maximum thread count
  * The amount of CPU cores available
  * The amount of demos to process
  * The amount of data (demo file sizes) to process
* If the final thread count decided is 1, all the work is done in the thread of the original function call.
* If the final thread count decided is greater than 1, all the work is done in new threads and the thread of the original function call will join (i.e. wait for) the new threads.

Note that for crash handling in C#, there is an annoying problem when using unmanaged code that creates its own threads: you can't catch exceptions of unmanaged threads.  
I have therefore re-implemented the logic for work distribution, thread creation/joining and progress report in C# so that all crashes can be caught.

Memory allocation tracking
--------------------------

The UDT code base has a pretty simple way of tracking memory allocators without using any synchronization primitive and with only 1 extra memory allocation per thread:

* Every memory allocator (instances of `udtVMLinearAllocator`) contains a node of an *intrusive* linked list data structure.
* There's a linked list of memory allocators per thread.
* Accessing that list is done through TLS (thread-local storage).
* Every time a memory allocator is constructed, it is added to the thread's list.
* Every time a memory allocator is destructed, it is removed from the thread's list.
* The linked list is lazily allocated and created on first access (1 *malloc* call per thread).
* The linked list is freed once per thread (1 *free* call per thread when the API function is done).
* When each job thread is done, it traverses its own allocator list and sums up the stats in its data return slot.
* When the API function is done waiting on the job threads, it sums up all the stats from the other threads with those of its own thread.
