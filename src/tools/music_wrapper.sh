#!/bin/bash
#
# play/pause/stop music wrapper for midi player
#
# build without_mixer
# set environment MUSIC_WRAPPER=path_to/music_wrapper.sh
# set fheroes2.cfg param: playmus command = path_to/music_wrapper.sh
# set executable flag on music_wrapper.sh
#

NAME=$2
# any midi player
CMD=timidity

if [ $2 ]; then
    PID=`ps x | grep ${CMD} | grep ${NAME} | awk '{ print $1 }'`

    case $1 in
	pause)
	if [ ${PID} ]; then
	    kill -STOP ${PID}
	fi
	;;

	cont*)
	if [ ${PID} ]; then
	    kill -CONT ${PID}
	fi
	;;

	stop)
	if [ ${PID} ]; then
	    kill -9 ${PID}
	fi
	;;

	*)
	exec ${CMD} $2
	;;
    esac
else
    exec ${CMD} $1
fi
