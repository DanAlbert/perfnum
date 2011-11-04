CS 311: Project 3 
=================
Interprocess Communication and Synchronization 
----------------------------------------------
Due Friday, 2 December 2011

This project will require that you computer perfect numbers. These perfect
numbers are numbers such that the sum of their proper divisors is equal to the
number itself. For instance, the proper divisors of 6 are 1, 2, and 3, which sum
to 6.

You will write 3 related programs to manage, report, and compute results.
=========================================================================
`compute`'s job is to compute perfect numbers. It tests all numbers beginning
from its starting point, subject to the constraints below. There may be more
than one copy of compute running simultaneously.

`manage`'s job is to maintain the results of compute. It also keeps track of the
active compute processes, so that it can signal them to terminate.

`report`'s job is to report on the perfect numbers found, the number tested, and
the processes currently computing. If invoked with the "-k" switch, it also is
used to inform the manage process to shut down computation.


You will write code for three different methods of IPC.
=======================================================
Pipes connecting the processes, such that results can be shared and starting
points indicated. In this case, manage should be the parent process. It will
provide the computes with their starting points, obtain the results from each
compute, and pass them to report when asked for, all via communication over
pipes.

As described above, there is a single process hierarchy. manage will take the
number of computes to spawn and the max value to be tested as command line
arguments.

Using shared memory, each process will be invoked independently, and communicate
all data via shared memory. This will require synchronization constructs to be
used in order to avoid race conditions.

The shared memory segment should contain the following data:
------------------------------------------------------------
A bit map large enough to contain the number of bits corresponding to the max
value to be tested. If a bit is off it indicates the corresponding integer has
not been tested.

An array of integers of length 20 to contain the perfect numbers found.

An array of "process" structures of length 20, to summarize data on the
currently active compute processes. This structure should contain the pid, the
number of perfect numbers found, the number of candidates tested, and the number
of candidates not tested. compute should never test a number already marked in
the bitmap.

`compute` processes are responsible for updating the bitmap, as well as their
own process statistics. However, because of the possible conflicts, you must
make use of a synchronization construct in order to avoid race conditions. This
includes updating the process array, updating the bitmap, and updating the list
of perfect numbers found.

Each `compute` will simply test the next untested number in the bitmap,
immediately marking it to indicate that no other compute should test it.

`report` will also make use of the shared memory segment in order to print out
the results.

Using sockets for communication, in a client server model. In this case, as in
the pipes case, `manage` will be the "master" process, which in this case means
the server. Unlike the other 2 methods, you will be able to launch `compute` on
any networked device. `compute` and `manage` behave as in the pipes method, but
all communication will be via the sockets, rather than pipes.

In this case, `compute` must be given the hostname and port number for `manage`
on the command line, and `report` will also include information on the hostname
for each `compute` process. For each compute, manage will send it a range to
evaluate, rather than simply a starting point. At any time, manage should be
able to obtain information on the status of a given compute process, and should
know when that process has terminated.

All processes should terminate cleanly on `INTR`, `QUIT`, and `HANGUP` signals.
For compute processes this means they delete their process entry from the shared
memory segment (if necessary) and then terminate. For manager, it means sending
an `INTR` signal to all the running computes, sleeping 5 seconds, and then
removing the shared memory segment, followed by termination. When the -k is flag
is used on report, report sends an `INTR` to manage to force the same shutdown
procedure.

For shared memory, remember that POSIX shared memory will be used,
rather than SYSV. Please ensure that you use a unique identifier for your
segment.