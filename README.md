# give-take

A command-line utility to share files between users on MathLAN.

Final project for CSC 213.

# Key concepts

In my project proposal, I specified these three areas:

1. Inter-Process Communication / Networks and Distributed Systems

	This category was meant as a catch-all for whatever kind of inter-process
	communication I ended up using. In retrospect this project leverages network
	communication rather than any other methods in order to be able to work
	between multiple computers, but this was not obvious at the time.

2. Files and File Systems

	Obviously, a program that transfers files has to deal with the file system. I
	did some stuff here that I think is interesting, though! This includes sending
	files in a way that works over the network, and also recursively sending
	directories.

3. Processes

	This project takes advantage of processes to daemonize the giving process, so
	that it is possible to do other things and/or log out after initializing a give.

	It's also an important part of this project to clean up the processes whenever
	it is possible to do so. This means that a give process should not stick
	around indefinitely on the system, which would be a bad thing.

In order to communicate effectively over the network, this project also uses
parallelism with threads in order to respond properly if someone other than the
intended recipient attempts to take the file instead.

# Example usage

First, one user must send the other a file, and the port that has been opened
will be displayed.

```
[heilalmond@even] ~ $ ./give fakeuser file.txt
Server listening on port 50112
```

Then, `heilalmond` is free to log off or do anything else on the system. Anytime in
the future, the user that they sent it to can pick up the file.

If this is happening on the same computer (localhost), it is possible to pick up
the file knowing only the port:

```
[fakeuser@even] ~ $ ./take 50112
(file.txt will appear in their current directory)
```

However, it is also possible to retrieve a file that was originally given from a
different machine. In order to do this, you also need to know the name of the
MathLAN computer (without `.cs.grinnell.edu`):

```
[fakeuser@forsythe] ~ $ ./take even:50112
(file.txt will appear in their current directory)
```

In either of these cases, the file `file.txt` will be loaded into `fakeuser`'s
current directory, as long as it does not already exist. This utility avoids
overwriting existing files, requiring the receiving user to change directories
or fix the conflict before the transfer can go through.

# Detailed usage

## `give` usage

### Give a file to a user

```
give TARGET_USER PATH
```

Parameters are as follows:

- `TARGET_USER` must be another user who exists on this system.

  - That user is also assumed to exist on the system that `take` will be run on,
		which is true on MathLAN but not computer systems in general.

- `PATH` must be a valid path to a regular file (as in, not a symlink or block
		device) or a directory readable by your current user.

  - Permissions will not be preserved, the file will be transferred with the
		default `umask` of the target user.

  - It need not be a file in your current directory, and if there is a path to the
		file that will be truncated when giving. For instance, if you run `give` on
		`/long/path/to/file.png`, it will be transferred as `file.png`.

  - When sending a directory, the program must recursively read the directory
			and store it in memory. Because of this, when opening too many files 

Will print the port being used to the terminal.

### Cancel a give

```
give -c [HOST:]PORT
```

Parameters are as follows:

- `HOST` is an optional network parameter. If used, it will attempt to cancel
a give on a different system than localhost.

  - It should be provided as the name of a MathLAN machine, without the
		`.cs.grinnell.edu` suffix.

  - There must be a colon with no spaces between it and the
		port. For instance, `even:50112` is valid, but `even: 50112` is not.

- `PORT` is a required network parameter. It refers to the port that the initial
	give was hosted on.

Will print to the terminal announcing that the give was cancelled.

## `take` usage

### Take a file that has been given

```
take [HOST:]PORT
```

Parameters are as follows:

- `HOST` is an optional network parameter. If used, it will attempt to take from a
	given MathLAN machine other than localhost.

  - It should be provided as the name of a MathLAN machine, without the
	  `.cs.grinnell.edu` suffix.

  - There must be a colon with no spaces between it and the
  	port. For instance, `even:50112` is valid, but `even: 50112` is not.

- `PORT` is a required network parameter. It specifies the port to attempt to take
	the file through.

Will print to the terminal announcing that the file was received.

# Notes

- The examples in this README assume that the `give` and `take` executables exist
	in the current directory, but they are easier to use if they exist somewhere on
	your `PATH` such as `~/.local/bin/`.

- When hosting a file, `give` accepts requests and checks that they contain the
	correct username (the one that the give was initiated using).

  - This is not a particularly robust method because the target username is
		possible to find by looking at the process with tools like `ps`, so it only
		really protects against the problem of users accidentally typing the wrong
		port when using `take`. It cannot defend against any belligerent attackers.

- The give process will not terminate when a user logs out, but due to system
	processes out of my control it may be killed if it is idle for too long, or
	if the system reboots. Keep this in mind, and do not leave these processes
	running indefinitely.

# Further work / TODO

- [ ] List outgoing gives (from the local machine)

- [ ] List incoming takes (on the local machine)

- [ ] Find if it's possible to list processes on a remote machine

- [ ] Look into encoding ports as letters to make them easier to remember / type

- [ ] Disallow giving to yourself and taking from yourself

- [x] Do not require `TARGET_USER` when cancelling a give
