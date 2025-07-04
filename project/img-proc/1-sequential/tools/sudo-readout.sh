#!/usr/bin/env bash 

TOOLDIR=.

##
# Initialize for each tile 4 C-HEAP standard in/out channels.
# The first pair is reserved for the system application, the second for app stdin/out.
##
if [ "$1" == "-debug" ] ; then
	$TOOLDIR/channel_init
	$TOOLDIR/channel_readall
else
	$TOOLDIR/channel_init -lessinfo
	$TOOLDIR/channel_readall -skippartition0
fi

exit 0

# in case you don't want to read all channels then replace the channel_readall by the loop below
while true
do
	## Tile 0 ##
	# system application
	$TOOLDIR/channel_readline 0 0
	# app 1
	$TOOLDIR/channel_readline 0 1
	# app 2
	$TOOLDIR/channel_readline 0 2
	# app 3
	$TOOLDIR/channel_readline 0 3

	## Tile 1 ##
	# system application
	$TOOLDIR/channel_readline 1 0
	# app 1
	$TOOLDIR/channel_readline 1 1
	# app 2
	$TOOLDIR/channel_readline 1 2
	# app 3
	$TOOLDIR/channel_readline 1 3

	## Tile 2 ##
	# system application
	$TOOLDIR/channel_readline 2 0
	# app 1
	$TOOLDIR/channel_readline 2 1
	# app 2
	$TOOLDIR/channel_readline 2 2
	# app 3
	$TOOLDIR/channel_readline 2 3
done
