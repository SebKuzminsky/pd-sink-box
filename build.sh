#!/bin/bash
set -e


PARALLEL=$(getconf _NPROCESSORS_ONLN)

while [[ -v 1 ]]; do
    case "$1" in

        "--parallel" | "-j")
            shift
            PARALLEL="$1"
            shift
            ;;

        "--verbose"|"-v")
            VERBOSE="VERBOSE=1"
            shift
            ;;

        *)
            echo "unknown argument '$1'"
            exit 1
            ;;

    esac
done


SRC_DIR="firmware"
#for PICO_BOARD in pico pico_w waveshare_rp2040_zero ; do
for PICO_BOARD in pico ; do
    BUILD_DIR="build_${PICO_BOARD}"
    MAKE_ARGS=(
        -C"${BUILD_DIR}"
        -j"${PARALLEL}"
    )
    if [[ -v VERBOSE ]]; then
        MAKE_ARGS[${#MAKE_ARGS[@]}+1]="${VERBOSE}"
    fi

    echo "building firmware for Pico board '${PICO_BOARD}'"

    cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" -D "PICO_BOARD=${PICO_BOARD}"
    make "${MAKE_ARGS[@]}"
done
