# give-take

A command-line utility to share files between users on MathLAN.

Final project for CSC 213.

# Example usage

First, one user must send the other a file:

```
[user1@computer] ~ $ ./give user2 file.txt
```

Then, `user1` is free to log off or do anything else on the system. Anytime in
the future, the user that they sent it to can pick up the file again.

```
[user2@computer] ~ $ ./take user1
```

This will put `file.txt` into the user's current directory, and clean up any
parts of the process that were running in the background.

# Detailed usage

## `give` usage

Usage: `./give TARGET_USER FILENAME`

`TARGET_USER` must be another user who exists on this system.

`FILENAME` must be a valid path to a file that is readable by your current user.

## `take` usage

Usage: `./take SOURCE_USER`

`SOURCE_USER` must be the username of another user who exists on this system.

# Notes

The examples in this README assume that the `give` and `take` executables exist
in the current directory, but they are easier to use if they exist somewhere on
your `PATH` such as `~/.local/bin/`.
