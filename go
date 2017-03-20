#!/bin/bash
APPS_PATH="~/popcorn-apps"
APP="mt"

LOAD=1
MSG_LAYER_MOD="msg_loopback.ko"
MSG_LAYER_NAME="loopback"

EXEC=1

case `uname -m` in
x86_64)
	ARCH="x86_64"
	;;
aarch64)
	ARCH="arm64"
	;;
esac

while :
do
	case $1 in
	lo*)
		MSG_LAYER_MOD="msg_loopback.ko"
		MSG_LAYER_NAME="loopback"
		EXEC=0
		shift
		;;
	so*)
		MSG_LAYER_MOD="msg_socket.ko"
		MSG_LAYER_NAME="socket"
		EXEC=0
		shift
		;;
	*)
		if [ -n "$1" ]; then
			APP="$1"
			if [[ $EXEC -eq 0 ]]; then
				LOAD=1
			else
				LOAD=0
			fi
			shift
		else
			break;
		fi
	esac
done

EXEC_BIN="$APP"

if [[ $LOAD -eq 1 ]]; then
	echo "Load message layer over $MSG_LAYER_NAME"
	sudo insmod ~/share/msg_layer/$ARCH/$MSG_LAYER_MOD
fi

if [[ $EXEC -eq 1 ]]; then
	echo "Execute app $APPS_PATH/$APP"
	exec $APPS_PATH/$APP
fi
