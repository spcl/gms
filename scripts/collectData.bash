#! /bin/bash

#############################################################################
# functions
function printHelp() {
    echo "usage: $1 [options]"
    echo "  -d [dir]   directory with input files (default: output)"
    echo "  -o [file]  file to which output is appended (default: data.csv)"
    echo "  -x         overwrite, rather than append"
}

#############################################################################
# set up variables
dataDirectory="output"
dataFile="data.csv"
delete="false"

paramValue="@@#"
programOutput="@@@"

#############################################################################
# start script:
# process input options
while getopts "hd:o:x" opt; do
    case $opt in
    h)
        printHelp "$0"
        exit 0
        ;;
    d )
        dataDirectory="$OPTARG"
        ;;
    o )
        dataFile="$OPTARG"
        ;;
    x )
        delete="true"
        ;;
    *)
        printHelp "$0"
        exit 1
        ;;
    esac
done

# delete old file if necessary
if [ $delete == "true" ]; then
    if [ -f "$dataFile" ]; then
        rm "$dataFile"
    fi
fi

# iterate through all files
for file in "$dataDirectory"/*; do
    # walk through relevant lines of file
    paramLine=""
    while read -r line; do
        # read parameters
        if [[ "$line" == $paramValue* ]]; then
            paramLine=${line#"$paramValue "}
        fi
        # print measurements with above parameters to dataFile
        if [[ "$line" == $programOutput* ]]; then
            cutLine=${line#"$programOutput "}
            echo "$paramLine $cutLine" >> "$dataFile"
        fi
    done < <(grep -e "$paramValue" -e "$programOutput" "$file")
done

echo "Appended data to '$dataFile'"
