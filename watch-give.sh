#!/bin/bash

# A script that runs watch to show you any processes running give, excluding
# those involved in the watch command (grep, watch, and sh).
# Useful for noticing if any processes refuse to die.

watch -n 1 'ps -aux | grep give | grep -v -E "watch|sh|grep"'
