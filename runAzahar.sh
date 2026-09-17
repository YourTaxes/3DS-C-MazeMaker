#!/bin/sh
# Launch Azahar from the terminal and stream its log (including the game's
# debug prints, logged under Debug.Emulated) here.
#
# Azahar is started through `open` (running the binary directly loses camera
# emulation etc.), which detaches it from this terminal, so the log is
# followed from azahar_log.txt instead. Ctrl-C stops following and quits Azahar.
#
# Usage: ./runAzahar.sh [<file.3dsx>] [gdb]
#   <file.3dsx>  which .3dsx to run (default: the project's build)
#   gdb          launch with -g <port> so Azahar starts its GDB stub and
#                pauses the game at boot until ./runGdb.sh connects.
#                Without it the stub is off.
#
# Env overrides: AZAHAR_APP (path to Azahar.app), AZAHAR_CONFIG (qt-config.ini),
#                AZAHAR_LOG (azahar_log.txt), GDB_PORT (default 24689)

DIR=$(cd "$(dirname "$0")" && pwd)
BASENAME=3DS-C-MazeMaker

ROM="$DIR/$BASENAME.3dsx"
GDB=false

for arg in "$@"; do
	case "$arg" in
		*.3dsx) ROM="$arg" ;;
		gdb)    GDB=true ;;
		*)
			echo "Usage: $0 [<file.3dsx>] [gdb]" >&2
			exit 1
			;;
	esac
done

if [ ! -f "$ROM" ]; then
	echo "error: $ROM not found (run make first)" >&2
	exit 1
fi

# find the Azahar bundle
if [ -z "$AZAHAR_APP" ]; then
	for candidate in "/Applications/Azahar.app" "$HOME/Downloads/azahar-macos-universal-2126.1.1/Azahar.app"; do
		if [ -d "$candidate" ]; then
			AZAHAR_APP="$candidate"
			break
		fi
	done
fi
if [ ! -x "$AZAHAR_APP/Contents/MacOS/azahar" ]; then
	echo "error: Azahar not found (set AZAHAR_APP=/path/to/Azahar.app)" >&2
	exit 1
fi

# The GDB stub is driven by the -g <port> command-line flag: it both enables
# the stub and makes Azahar pause the game at boot (the ini's use_gdbstub
# setting only gives an attach-while-running stub, with no pause). Keep the
# ini setting off so a plain run never starts the stub on its own. Azahar
# rewrites the ini on exit, so this has to happen while it isn't running.
AZAHAR_CONFIG="${AZAHAR_CONFIG:-$HOME/Library/Application Support/Azahar/config/qt-config.ini}"
if [ -f "$AZAHAR_CONFIG" ]; then
	sed -i '' \
		-e 's/^use_gdbstub=.*/use_gdbstub=false/' \
		-e 's/^use_gdbstub\\default=.*/use_gdbstub\\default=false/' \
		"$AZAHAR_CONFIG"
else
	echo "warning: $AZAHAR_CONFIG not found, leaving GDB stub setting alone" >&2
fi

GDB_PORT="${GDB_PORT:-24689}"
GDB_ARGS=""
if [ "$GDB" = true ]; then
	# short form only: this Azahar build exits silently when given --gdbport
	GDB_ARGS="-g $GDB_PORT"
	echo "GDB stub on port $GDB_PORT: Azahar will pause at boot until ./runGdb.sh connects"
fi

AZAHAR_LOG="${AZAHAR_LOG:-$HOME/Library/Application Support/Azahar/log/azahar_log.txt}"

# Azahar rotates the log on startup (azahar_log.txt -> azahar_log.old.txt),
# so remember the current file's inode and wait for a fresh one before
# tailing, otherwise we'd print the previous run's log first.
old_inode=$(stat -f %i "$AZAHAR_LOG" 2>/dev/null)

echo "Running $ROM"
# the ROM must be the last argument - Azahar takes the final non-flag arg as the game path
open -a "$AZAHAR_APP" --args $GDB_ARGS "$ROM" || exit 1

i=0
while [ "$(stat -f %i "$AZAHAR_LOG" 2>/dev/null)" = "$old_inode" ] && [ $i -lt 100 ]; do
	sleep 0.1
	i=$((i + 1))
done

# Ctrl-C: stop tailing and quit Azahar
trap 'echo; osascript -e "quit app \"Azahar\"" >/dev/null 2>&1; exit 0' INT TERM

tail -n +1 -F "$AZAHAR_LOG"
