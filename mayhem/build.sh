#!/usr/bin/env bash
#
# mayhem/build.sh — Linux build of WechatExporter's portable core (WechatExporter/core) + the fuzz
# harnesses + the functional self-test. Upstream ships only Xcode/MSVC projects, so this compiles the
# core sources directly. Excluded: IDeviceBackup.cpp (live-device libimobiledevice I/O),
# DownloadPool.cpp (includes a Windows-only OSDef.h), Utils_silk.cpp / Utils_audio.cpp (silk SDK /
# lame / opencore-amrnb voice transcoding, not packaged), Utils_md5.cpp (Win32/Apple only; replaced
# by mayhem/compat/Utils_md5_linux.cpp over OpenSSL). The silk/amr/mp3 functions MessageParser and
# AsyncTask reference are abort() stubs in mayhem/compat/Utils_audio_unavailable.cpp (only reached
# outside text mode; fuzz_message_parser runs in text mode).
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"

cd "$SRC"
CORE="$SRC/WechatExporter/core"
OUT="$SRC/mayhem/build"
mkdir -p "$OUT"

CORE_SRCS="AsyncExecutor AsyncTask Downloader Exporter FileSystem ITunesParser MessageParser RawMessage
  ResManager TaskManager Template Updater Utils Utils_protobuf Utils_thread Utils_xml WechatParser XmlParser"
# Upstream relies on transitive includes of the Apple/MSVC standard libraries; force the standard
# headers it uses, and map the BSD st_mtimespec field to glibc's st_mtim.
PORT_FLAGS="-std=c++14 -include cstdint -include string -include cstring -include functional
  -include algorithm -include arpa/inet.h -Dst_mtimespec=st_mtim -w"
INCS="-I$CORE -I/usr/include/libxml2 -I/usr/include/jsoncpp"
LIBS="-lxml2 -lsqlite3 -ljsoncpp -lplist-2.0 -lprotobuf -lcurl -luuid -lcrypto -lz -lpthread -lm"

# UBSan relaxations (ASan and every other UBSan check stay on and halting):
#  * alignment, everywhere: the Manifest.mbdb/MMKV readers load big-endian integers through
#    misaligned casts (e.g. ITunesParser.cpp `*((uint32_t *)(m_fixedData + 18))`) on EVERY record,
#    which is benign on x86 and would make every valid backup "crash".
#  * null, in RawMessage.cpp only: RawMessage::mergeFile takes `&buffer[0]` of an empty vector for a
#    0-byte file (a null reference binding that is never dereferenced, size 0 is passed along).
#    libFuzzer always runs the empty input first, so LoginInfo2 could never iterate otherwise.
extra_flags() {
  case "$2" in *-fsanitize=*undefined*) ;; *) return 0 ;; esac
  printf '%s' "-fno-sanitize=alignment"
  [ "$1" = RawMessage ] && printf ' %s' "-fno-sanitize=null"
  return 0
}

# build_lib <dir> <cflags...>: compile the core into <dir>/libwecore.a (incremental: make-style
# timestamp check so a re-run on a built tree only recompiles what changed).
build_lib() {
  local dir="$1"; shift
  local flags="$*"
  mkdir -p "$dir"
  local objs=() pids=()
  for s in $CORE_SRCS; do
    objs+=("$dir/$s.o")
    if [ ! -f "$dir/$s.o" ] || [ "$CORE/$s.cpp" -nt "$dir/$s.o" ] || [ -n "$(find "$CORE" -name '*.h' -newer "$dir/$s.o" -print -quit)" ]; then
      $CXX $flags $(extra_flags "$s" "$flags") $PORT_FLAGS $INCS -c "$CORE/$s.cpp" -o "$dir/$s.o" &
      pids+=($!)
      if [ "${#pids[@]}" -ge "$MAYHEM_JOBS" ]; then wait "${pids[0]}"; pids=("${pids[@]:1}"); fi
    fi
  done
  for p in "${pids[@]}"; do wait "$p"; done
  $CXX $flags $PORT_FLAGS $INCS -c "$SRC/mayhem/compat/Utils_md5_linux.cpp" -o "$dir/Utils_md5_linux.o"
  $CXX $flags $PORT_FLAGS $INCS -c "$SRC/mayhem/compat/Utils_audio_unavailable.cpp" -o "$dir/Utils_audio_unavailable.o"
  $CC $flags -include string.h -c "$CORE/md5.c" -o "$dir/md5.o"
  objs+=("$dir/Utils_md5_linux.o" "$dir/Utils_audio_unavailable.o" "$dir/md5.o")
  rm -f "$dir/libwecore.a"
  ar rcs "$dir/libwecore.a" "${objs[@]}"
}

# 1) Sanitized + DWARF-3 core for the fuzzers.
# -fsanitize=fuzzer-no-link: SanCov edges in the core itself, not only in the harness TU.
build_lib "$OUT/fuzz" $SANITIZER_FLAGS $DEBUG_FLAGS -O1 -fsanitize=fuzzer-no-link

$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OUT/lsan_off.o"

# 2) Each harness twice: libFuzzer binary + run-once standalone reproducer.
for h in fuzz_template fuzz_xmlparser fuzz_logininfo2 fuzz_mmkv fuzz_itunes_mbdb fuzz_itunes_manifestdb fuzz_info_plist fuzz_wcdb_contacts \
         fuzz_message_parser; do
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS -O1 -fsanitize=fuzzer-no-link $PORT_FLAGS $INCS -I"$SRC/mayhem/harnesses" \
    -c "$SRC/mayhem/harnesses/$h.cpp" -o "$OUT/$h.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OUT/$h.o" "$OUT/lsan_off.o" \
    "$OUT/fuzz/libwecore.a" $LIBS -o "$SRC/mayhem/build/$h"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link "$OUT/$h.o" "$OUT/standalone_main.o" "$OUT/lsan_off.o" \
    "$OUT/fuzz/libwecore.a" $LIBS -o "$SRC/mayhem/build/$h-standalone"
done

# 3) Functional self-test with normal flags (no sanitizers), run by mayhem/test.sh.
build_lib "$OUT/test" -O1 $COVERAGE_FLAGS
$CXX -O1 $COVERAGE_FLAGS $PORT_FLAGS $INCS -c "$SRC/mayhem/selftest/core_selftest.cpp" -o "$OUT/core_selftest.o"
$CXX $COVERAGE_FLAGS "$OUT/core_selftest.o" "$OUT/test/libwecore.a" $LIBS -o "$OUT/core_selftest"
