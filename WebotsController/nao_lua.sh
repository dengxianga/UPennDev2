#!/bin/sh

# On Linux, need to verify that xterm is not setgid
# Otherwise, LD_LIBRARY_PATH gets unset in xterm

COMPUTER=`uname`
export COMPUTER

#PLAYER_ID=$1
PLAYER_ID=1
export PLAYER_ID
#TEAM_ID=$2
TEAM_ID=18
export TEAM_ID

PLATFORM=webots
export PLATFORM

#exec xterm -e "lua -l controller"
#exec luajit -l controller start.lua
exec lua -l controller start.lua

