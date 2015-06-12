

# Concurrency: multi-process and multi-thread access #

<strong>whefs currently has only the most basic support for concurrency! Multiple processes can acquire locks (see below), but multiple threads which access the same EFS <em>will</em> cause corruption or other failure at some point!</strong>

That said...

The library is concerned with concurrency at two levels:

  * Multiple processes access an EFS.
  * Multiple threads within a process.

The former can be addressed using POSIX fcntl() locks. The latter is a whole other can of worms.

Some skeleton code is in place to support fcntl() locks, but there is still much work to do here, and much of it is awaiting other refactoring to take place (off of which the locking framework will be built).

Work on multi-threading support is still far, far away. The fcntl() locks are considered more important for this particular case, and threading scares the hell out of me, so that feature is going to have to wait.

## Why do we need concurrency support? ##

First, file/record-level locks can keep multiple (cooperating) processes from hosing each other while they work on an EFS file. Threading support is of course necessary for modern applications in general.

One of the more interesting uses for file-level locks is that we could use whefs as a data store for CGI applications, in the same way that sqlite3 acts as an encapsulated data store for a [Fossil repository](http://fossil-scm.org). This bypasses a number of problems related to file ownership and access rights when running over CGI. This level of concurrency would also allow us to safely implement SWIG-based wrappers for languages like PHP. A PHP wrapper could get the same benefits as a CGI apps - a safe data store which isn't going to have incorrect access rights when new files are created via the web interface.

## File locking (new as of 20090322, commit `#a373df92f8`) ##

The engine currently supports only the most basic file locking:

  * When an EFS is opened read/write, the engine will open the file (if possible), then wait until it can get a write lock on the whole file.
  * For read-only mode, it locks the whole file with a read lock. Multiple reader processes can use the filesystem safely.
  * The locking is only advisory, relying on <tt>fcntl()</tt> to do the dirty work. Bugs in whefs could cause a lock to be violated.

There needs to be more granularity in this (record- or pseudofile-level locking, instead of container-level), but that is going to take a large effort and a larger amount of testing to ensure that it works as advertised. It also raises questions such as "how do we do an 'ls' if one of the inodes is write-locked at the time?"

## Specific problems vis-a-vis threading ##

On the surface it would seem that we can lock a set of EFS operations using a mechanism like this:

```
whefs_thread_lock_fs( fs ); // waits for lock
...
whefs_thread_unlock_fs( fs );
```

Other than being problematic to handle correct vis-a-vis error handling, and unwieldy for client-side use, it theoretically has a more fatal flaw: the i/o layer is device-independent. Unless the underlying i/o device has thread-specific values for its current position (and possibly other mutable data), i'm not certain that we can guaranty the consistency of the underlying data store in the face of access via multiple threads. i don't have a concrete example to show. It's possible that one thread repositions the internal cursor, such that another thread would read from or write to the wrong position, effectively corrupting the FS.

i'm trying to figure out how to hide the locking from the client, but i can't think of a good way which would allow any sort of guaranty on the consistency of the storage's position pointer.