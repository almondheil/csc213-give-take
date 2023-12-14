# give-take

A command-line utility to share files and directories between users on MathLAN.

Final project for CSC 213, Fall 2023.

# Installing

First, clone this repo and `cd` into it:

```
git clone https://github.com/almondheil/csc213-give-take.git
cd csc213-give-take
```

Then, build it. The build should not require any dependencies not already on
MathLAN, and you will see a couple warnings about unused functions--just ignore
these for now :)

```
make
```

This will create the executables needed. To be able to use them no matter what
directory you are in, it can be helpful to move them somewhere in your `$PATH`.
A common location is `~/.local/bin/`.

# Key concepts

In my project proposal, I specified these three areas:

1. Inter-Process Communication / Networks and Distributed Systems

	This category was meant as a catch-all for whatever kind of inter-process
	communication I ended up using. In retrospect this project leverages network
	communication instead of the other options I had considered, but this wasn't
	clear to me at the time.

2. Files and File Systems

	Obviously, a program that transfers files has to deal with the file system. I
	did some stuff here that I think is interesting, though! In particular, I was
	able to represent actual directory structures in a network-friendly way, and
	log pending gives so their status can be checked.

3. Processes

	This project takes advantage of processes to daemonize the giving process, so
	that it is possible to do other things and/or log out after initializing a
	give. It's also an important part of this project to clean up the processes
	whenever it is possible to do so. This means that a give process should not
	stick around indefinitely on the system, which would be a bad thing.

In order to communicate effectively over the network, this project also uses
parallelism with threads in order to respond properly if someone other than the
intended recipient attempts to take the file instead. This was not in my initial
proposal, but I figure I might as well list it here.

# Example usage

## Between users on one machine

First, one user sends the other a file.

```
[userA@even] ~ $ give userB projectdir/
Server listening on port 50112
```

Then, the other user can take the file once they are told the correct port to use:

```
[userB@even] ~ $ take 50112
Successfully took projectdir
```

## Between users on multiple machines

Just like on one machine, one user initializes the give.

```
[userA@even] ~ $ give userB projectdir/
Server listening on port 50112
```

The only extra info needed to take the file is the name of the machine the transaction was started from.

```
[userB@loeb] ~ $ take even:50112
Successfully took projectdir
```

## When there is a conflicting filename

Suppose userA wants to send their `.bashrc` to userB:

```
[userA@even] ~ $ give userB ~/.bashrc
Server listening on port 60703
```

However, when userB tries to take the file they realize they already have a `.bashrc`!

```
[userB@noyce] ~ $ take even:60703
Refusing to overwrite existing file ./.bashrc
```

Luckily, they can just repeat the command, but specifiy a new name for the file to be saved under.

```
[userB@noyce] ~ $ take even:60703 userA-bashrc
Successfully took userA-bashrc
```

## Checking status

The user who initialized a give can check the status of any outgoing gives which have not yet been closed.

```
[userA@even] ~ $ give userB projectdir/
Server listening on port 57599
[userA@even] ~ $ give userB otherprojectdir/
Server listening on port 40629
[userA@even] ~ $ give --status
projectdir
  To: userB
  Server: even:57599
  Directory: /home/userA
  Time: 2023-12-14 10:06:49
otherprojectdir
  To: heilalmond
  Server: even:40629
  Directory: /home/userA
  Time: 2023-12-14 10:06:53
```

## Cancelling a mistake

Suppose that from the previous example, userA didn't really mean to send `projectdir`, but `otherprojectdir` instead.
To fix this mistake (even if they closed the terminal) they can refer to the information printed out by the status command in order to cancel. For instance,

```
[userA@even] ~ $ give -c 57599
Successfully cancelled give.
```

This can also be done from a different machine:

```
[userA@loeb] ~ $ give -c even:40629
Successfully cancelled give.
```

# Detailed usage

## `give` usage

The `give` command has the most power, being able to create, cancel, and list
transfers.

### Give a file or directory to a user

```
give TARGET_USER PATH
```

On success, this command prints the port in use to the terminal.

Parameters are as follows:

- `TARGET_USER` must be another user who exists on this system.

  - That user is also assumed to exist on the system that `take` will be run on,
		which is true on MathLAN but not computer systems in general.

- `PATH` must be a valid path to a regular file (as in, not a symlink or block
		device) or a directory readable by your current user.

  - Permissions will not be preserved, the file or directory will be transferred with the
		default `umask` of the target user.

  - The path does not need to be in your current directory. If you send a file
			with the path `/path/to/file.png`, the name of the file when sent will
			simply be `file.png`.

  - There is a limit on the total size of files that can be sent. If you
			encounter this limit, you could try compressing them somehow. It is also
			possible to manually increase the limit, but the set limit of 256MB is in
			place because network operations tend to take too long past that limit.

### Cancel a give

```
give -c [HOST:]PORT
```

On success, this command prints that the give was cancelled.

Parameters are as follows:

- `HOST` is an optional network parameter. If used, it will attempt to cancel
a give on a different system than localhost.

  - It should be provided as the name of a MathLAN machine, without the
		`.cs.grinnell.edu` suffix.

  - There must be a colon with no spaces between it and the
		port. For instance, `even:50112` is valid, but `even: 50112` is not.

- `PORT` is a required network parameter. It refers to the port that the initial
	give was hosted on.

### List pending gives

```
give --status
```

This command prints out a list of any pending gives, or nothing if there are
none. It does not take any parameters to customize the behavior.

## `take` usage

### Take a file or directory that has been given

```
take [HOST:]PORT [NAME]
```

On success, this command will print that the file or directory was successfully taken.

Parameters are as follows:

- `HOST` is an optional network parameter. If used, it will attempt to take from a
	given MathLAN machine other than localhost.

  - It should be provided as the name of a MathLAN machine, without the
	  `.cs.grinnell.edu` suffix.

  - There must be a colon with no spaces between it and the
  	port. For instance, `even:50112` is valid, but `even: 50112` is not.

- `PORT` is a required network parameter. It specifies the port to attempt to take
	the file or directory through.

- `NAME` is an optional parameter. If provided, it modifies the name of the
		received file or directory to itself. Otherwise, it will default to whatever
		name the file had when it was given.

# Notes

- The examples in this README assume that the `give` and `take` executables exist
	in the current directory, but they are easier to use if they exist somewhere on
	your `PATH` such as `~/.local/bin/`.

- When hosting a file or directory, `give` accepts requests for the data
		and checks that they contain the username specified when the give was first
		initiated.

  - This is not a particularly robust method because the target username is
		possible to find by looking at the process with tools like `ps`, so it only
		really protects against the problem of users accidentally typing the wrong
		port when using `take`. It cannot defend against any belligerent attackers.

- The give process will not terminate when a user logs out, but due to system
	processes it may eventually be terminated. Keep this in mind, and don't expect
	process to run indefinitely. However, I believe things will be fine on the
	timespan of a few days.
