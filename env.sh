#!/bin/bash
pwd
env
CONAN_FILE="conanbuildinfo.txt"
OUTPUT_FILE=".env"

if [ -f "$CONAN_FILE" ]; then

    extract_paths() {
        local section=$1
        local in_section=false

        while IFS= read -r line; do
            if [[ "$line" == "[$section]" ]]; then
                in_section=true
                continue
            fi

            if [[ "$line" =~ ^\[.*\]$ ]] && [ "$in_section" = true ]; then
                break
            fi

            if [ "$in_section" = true ] && [[ -n "$line" ]] && [[ ! "$line" =~ ^[[:space:]]*# ]]; then
                echo "$line"
            fi
        done < "$CONAN_FILE"
    }

    INCLUDE_PATHS=$(extract_paths "includedirs")
    LIB_PATHS=$(extract_paths "libdirs")

    if [ -z "$INCLUDE_PATHS" ] && [ -z "$LIB_PATHS" ]; then
        echo "Error: No include or lib paths found in $CONAN_FILE"
        exit 1
    fi
fi

CPATH_VAR=""
LIBRARY_PATH_VAR=""
DYLD_LIBRARY_PATH_VAR=""

if [ -n "$INCLUDE_PATHS" ]; then
    while IFS= read -r path; do
        if [ -n "$path" ] && [ -d "$path" ]; then
            if [ -z "$CPATH_VAR" ]; then
                CPATH_VAR="$path"
            else
                CPATH_VAR="$CPATH_VAR:$path"
            fi
        fi
    done <<< "$INCLUDE_PATHS"
fi

if [ -n "$LIB_PATHS" ]; then
    while IFS= read -r path; do
        if [ -n "$path" ] && [ -d "$path" ]; then
            if [ -z "$LIBRARY_PATH_VAR" ]; then
                LIBRARY_PATH_VAR="$path"
                DYLD_LIBRARY_PATH_VAR="$path"
            else
                LIBRARY_PATH_VAR="$LIBRARY_PATH_VAR:$path"
                DYLD_LIBRARY_PATH_VAR="$DYLD_LIBRARY_PATH_VAR:$path"
            fi
        fi
    done <<< "$LIB_PATHS"
fi

LIBRARY_PATH_VAR="$LIBRARY_PATH_VAR:/opt/homebrew/Cellar/qt@5/5.15.17/plugins/sqldrivers:/opt/homebrew/opt/libpq/lib"
DYLD_LIBRARY_PATH_VAR="$DYLD_LIBRARY_PATH_VAR:/opt/homebrew/Cellar/qt@5/5.15.17/plugins/sqldrivers:/opt/homebrew/opt/libpq/lib"
if [ -n "$CPATH_VAR" ]; then
    export CPATH="$CPATH_VAR"
    echo "Exported CPATH: $CPATH"
fi

if [ -n "$LIBRARY_PATH_VAR" ]; then
    export LIBRARY_PATH="$LIBRARY_PATH_VAR"
    echo "Exported LIBRARY_PATH: $LIBRARY_PATH"
fi

if [ -n "$DYLD_LIBRARY_PATH_VAR" ]; then
    export DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH_VAR"
    echo "Exported DYLD_LIBRARY_PATH: $DYLD_LIBRARY_PATH"
fi

echo "# Generated environment file from $CONAN_FILE" > "$OUTPUT_FILE"
echo "# Generated on: $(date)" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

if [ -n "$CPATH_VAR" ]; then
    echo "CPATH=\"$CPATH_VAR\"" >> "$OUTPUT_FILE"
    echo "Added CPATH with $(echo "$INCLUDE_PATHS" | wc -l | tr -d ' ') include paths"
fi

if [ -n "$LIBRARY_PATH_VAR" ]; then
    echo "LIBRARY_PATH=\"$LIBRARY_PATH_VAR\"" >> "$OUTPUT_FILE"
    echo "DYLD_LIBRARY_PATH=\"$DYLD_LIBRARY_PATH_VAR\"" >> "$OUTPUT_FILE"
    echo "Added LIBRARY_PATH with $(echo "$LIB_PATHS" | wc -l | tr -d ' ') library paths"
fi

echo "QT_PLUGIN_PATH=\"/opt/homebrew/Cellar/qt@5/5.15.17/plugins\"" >> "$OUTPUT_FILE"
echo "QT_DEBUG_PLUGINS=1" >> "$OUTPUT_FILE"
echo "QT_DEBUG=1" >> "$OUTPUT_FILE"

psql -U postgres -c "DROP DATABASE curs3_db;"
psql -U postgres -c "CREATE DATABASE curs3_db;"
psql --username=postgres curs3_db < "/Users/user/Projects/novsu/novsudbcurs/migrations/001_create_database.up.sql"
psql --username=postgres curs3_db < "/Users/user/Projects/novsu/novsudbcurs/migrations/002_teachers_view.up.sql"
psql --username=postgres curs3_db < "/Users/user/Projects/novsu/novsudbcurs/migrations/003_reports.up.sql"
psql --username=postgres curs3_db < "/Users/user/Projects/novsu/novsudbcurs/migrations/00_set_moc.sql"

compiledb make -j10
