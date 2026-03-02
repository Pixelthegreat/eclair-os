# Initially run file for Eclair OS shell (/bin/sh) #
echo Welcome to Eclair OS!
echo

if [ "$USER" = "root" ]; then
	PS1="\"[\e[32;1m\$USER@\$PWD\e[0m]\"' # '"
else
	PS1="\"[\e[32;1m\$USER@\$PWD\e[0m]\"' \$ '"
fi

sysinfo -c
