#!/bin/sh
# Start arm-none-eabi-gdb on the game's .elf and connect it to Azahar's GDB
# stub (start Azahar first with ./runAzahar.sh gdb).
#
# Usage: ./runGdb.sh [<file.elf>]
#   <file.elf>  which .elf's symbols to load (default: the project's build)
#
# Env overrides: GDB_PORT (default 24689, matches the Azahar config)

DIR=$(cd "$(dirname "$0")" && pwd)
BASENAME=3DS-C-MazeMaker

export PATH="${DEVKITARM:-/opt/devkitpro/devkitARM}/bin:$PATH"
GDB_PORT="${GDB_PORT:-24689}"

ELF="$DIR/$BASENAME.elf"

case "${1:-}" in
	"")     ;;
	*.elf)  ELF="$1" ;;
	*)
		echo "Usage: $0 [<file.elf>]" >&2
		exit 1
		;;
esac

if [ ! -f "$ELF" ]; then
	echo "error: $ELF not found (run make first)" >&2
	exit 1
fi

if ! command -v arm-none-eabi-gdb >/dev/null 2>&1; then
	echo "error: arm-none-eabi-gdb not found (is devkitARM installed at \$DEVKITARM?)" >&2
	exit 1
fi

echo "Loading $ELF, connecting to localhost:$GDB_PORT"
exec arm-none-eabi-gdb "$ELF" -ex "target remote localhost:$GDB_PORT"
