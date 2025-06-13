#!/bin/bash

SERVICES_DIR="$(dirname "$0")/services"
IGNORE_FILES=("__init__.py" "log.py" "storage.py")

echo "Starting services from directory: $SERVICES_DIR"
for file in "$SERVICES_DIR"/*.py; do
    [ -e "$file" ] || continue
    filename=$(basename "$file")
    skip=false
    for ignore in "${IGNORE_FILES[@]}"; do
        if [[ "$filename" == "$ignore" ]]; then
            skip=true
            break
        fi
    done
    $skip && continue
    echo "Running   $file..."
    python3 "$file" &
done
echo "All services started."
wait