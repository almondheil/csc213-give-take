# give-take

A command-line utility to share files between users on MathLAN.

Final project for CSC 213.

# Building

This project should build on any system with a good unix compiler available.

To build it, just run `make`.

# Commands

When building, you will have two executables -- `give` and `take`. `give` initiates
the sending of files, and `take` completes the transactions that `give` starts.

## Running `give`

TODO

## Running `take`

TODO

# Example usage

Suppose that `alice` wants to send `bob` her copy of `file.md`. Then, alice will
run the following.

```
[alice@even] ~ $ give bob file.md
```

After this command returns, `alice` is free to do whatever she wants--this
includes logging off the system.

At any point afterwards, `bob` may run the following command to complete the
transaction.

```
[bob@even] ~ $ take alice
```

After `bob` completes the transaction, there will not be any leftover processes
wasting resources on the system.
