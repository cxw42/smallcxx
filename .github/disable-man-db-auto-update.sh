#!/bin/bash
# .github/disable-man-db-auto-update.sh: disable man-db auto-updates of
# the index.  That way, when we install packages in CI, we don't have to wait
# for the manpages to update.  Run as root.
#
# This has to be in a script, not run as part of an `sh -c '...'` line.
# I think this is because /usr/share/debconf/confmodule expects "$0" to be
# a filename, but I'm not sure.

rm -f /var/lib/man-db/auto-update

lib='/usr/share/debconf/confmodule'
if [[ ! -e "$lib" || ! -r "$lib" ]]; then
    echo "No $lib --- skipping"
    exit 0
fi

. "$lib"

# Print the old value
# db_get man-db/auto-update
# echo "$RET"

db_set man-db/auto-update false
echo "Result of setting man-db/auto-update: [$RET]"

# Print the new value
#db_get man-db/auto-update
#echo "$RET"
