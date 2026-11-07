#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/cyd-kiln-native-tests"
CXX="${CXX:-g++}"
mkdir -p "$BUILD_DIR"

COMMON="-std=c++11 -Wall -Wextra -Werror"
INCLUDES="-I$ROOT/shared/protocol -I$ROOT/firmware/controller_s3/src"

$CXX $COMMON $INCLUDES \
  "$ROOT/tests/protocol/test_protocol.cpp" \
  -o "$BUILD_DIR/test_protocol"
"$BUILD_DIR/test_protocol"

$CXX $COMMON $INCLUDES \
  "$ROOT/tests/controller_s3/test_control_core.cpp" \
  "$ROOT/firmware/controller_s3/src/profile_engine.cpp" \
  "$ROOT/firmware/controller_s3/src/zone_controller.cpp" \
  "$ROOT/firmware/controller_s3/src/safety_manager.cpp" \
  "$ROOT/firmware/controller_s3/src/kiln_control_core.cpp" \
  -o "$BUILD_DIR/test_control_core"
"$BUILD_DIR/test_control_core"

$CXX $COMMON $INCLUDES \
  "$ROOT/tests/controller_s3/test_sensor_manager.cpp" \
  "$ROOT/firmware/controller_s3/src/sensor_manager.cpp" \
  -o "$BUILD_DIR/test_sensor_manager"
"$BUILD_DIR/test_sensor_manager"

$CXX $COMMON $INCLUDES \
  "$ROOT/tests/controller_s3/test_zone_mapping.cpp" \
  "$ROOT/firmware/controller_s3/src/zone_mapping.cpp" \
  -o "$BUILD_DIR/test_zone_mapping"
"$BUILD_DIR/test_zone_mapping"

printf '%s\n' "All native kiln controller tests passed."
