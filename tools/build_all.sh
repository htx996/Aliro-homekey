#!/usr/bin/env bash
#
# Build and package every release target, from inside the esp-matter container.
#
#     docker run --rm -v "$PWD":/work espressif/esp-matter:latest_idf_v5.5.4 \
#         bash -lc '. "$IDF_PATH/export.sh" >/dev/null 2>&1
#                   . "$ESP_MATTER_PATH/export.sh" >/dev/null 2>&1
#                   /work/tools/build_all.sh'
#
# Each target gets its own build directory, so a rebuild of one does not throw
# away the others.
set -uo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

# The job limit is load-bearing, not a preference.
#
# connectedhomeip's C++ runs to roughly 2 GB per compiler process with full
# debug info. Give ninja its default width on a container with 8 GB and GCC
# dies inside dwarf2out -- as an assembler "file table slot is already
# occupied" error, or a straight internal compiler error, on a different file
# each run, so it reads like a toolchain that has come loose rather than a
# machine that is out of memory. Three of those cost an afternoon before the
# pattern was obvious.
#
# idf.py has no -j; it shells out to cmake --build, which reads this.
export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-2}"

targets=("${@:-esp32 esp32c3 esp32s3}")
read -r -a targets <<< "${targets[*]}"
rc=0

for t in "${targets[@]}"; do
    bdir="build-$t"
    echo "=================== $t ==================="
    date -u +'  start %H:%M:%S UTC'

    if ! idf.py -B "$bdir" -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.matter" \
                -D SDKCONFIG="$bdir/sdkconfig" set-target "$t" > "/tmp/$t.cfg.log" 2>&1; then
        echo "  CONFIGURE FAILED"; tail -25 "/tmp/$t.cfg.log"; rc=1; continue
    fi

    if ! idf.py -B "$bdir" -D SDKCONFIG="$bdir/sdkconfig" build > "/tmp/$t.log" 2>&1; then
        echo "  BUILD FAILED"
        grep -E "error:|internal compiler error" "/tmp/$t.log" | head -8
        echo "  (an internal compiler error here usually means memory, not code -- retry, or lower CMAKE_BUILD_PARALLEL_LEVEL)"
        rc=1; continue
    fi

    # Two settings that silently produce wrong firmware if they do not take.
    grep -Fqx 'chip_persist_subscriptions = true' "$bdir/esp-idf/chip/args.gn" \
        && echo "  subscriptions : ok" || { echo "  subscriptions : MISSING"; rc=1; }
    grep -q '#define CONFIG_ALIRO_NFC_ECP_BEACON 1' "$bdir/config/sdkconfig.h" \
        && echo "  ECP beacon    : on" || echo "  ECP beacon    : off"

    app=$(stat -c%s "$bdir/aliro_homekey.bin"); slot=$((0x1E0000))
    echo "  app $((app/1024)) KB / slot $((slot/1024)) KB / headroom $(((slot-app)/1024)) KB"
    [ "$app" -lt "$slot" ] || { echo "  DOES NOT FIT AN OTA SLOT"; rc=1; continue; }

    # package_firmware.sh reads ./build, so point it at this target's output.
    rm -rf build && cp -a "$bdir" build
    ./tools/package_firmware.sh "$t" > /dev/null && echo "  packaged      : ok"
    date -u +'  done  %H:%M:%S UTC'
done

echo "=================== artifacts ==================="
ls -l firmware/ 2>/dev/null
exit $rc
