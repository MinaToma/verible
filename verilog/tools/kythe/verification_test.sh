#!/bin/bash
#
# Copyright 2020 Google LLC.
# SPDX-License-Identifier: Apache-2.0
#
# Extract Kythe indexing facts from SystemVerilog code and check the Kythe
# verification expectations from the annotations.

# --- begin runfiles.bash initialization ---
# Copy-pasted from Bazel's Bash runfiles library (tools/bash/runfiles/runfiles.bash).
set -euo pipefail
if [[ ! -d "${RUNFILES_DIR:-/dev/null}" && ! -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  if [[ -f "$TEST_SRCDIR/MANIFEST" ]]; then
    export RUNFILES_MANIFEST_FILE="$TEST_SRCDIR/MANIFEST"
  elif [[ -f "$0.runfiles/MANIFEST" ]]; then
    export RUNFILES_MANIFEST_FILE="$0.runfiles/MANIFEST"
  elif [[ -f "$TEST_SRCDIR/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
    export RUNFILES_DIR="$TEST_SRCDIR"
  fi
fi
if [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
elif [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " \
    "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
else
  echo >&2 "ERROR: cannot find @bazel_tools//tools/bash/runfiles:runfiles.bash"
  exit 1
fi
# --- end runfiles.bash initialization ---

source "$(rlocation "io_bazel/src/test/shell/unittest.bash")" ||
  {
    echo "unittest.bash not found!" >&2
    exit 1
  }
TESTS_DIR="$(rlocation "com_google_verible/verilog/tools/kythe/testdata/more_testdata")" ||
  {
    echo "Can't load the test data!" >&2
    exit 1
  }
VERIBLE_EXTRACTOR_BIN="$(rlocation "com_google_verible/verilog/tools/kythe/verible-verilog-kythe-extractor")" ||
  {
    echo "Can't load the extractor binary!" >&2
    exit 1
  }
KYTHE_VERIFER_BIN="$(rlocation "io_kythe/kythe/cxx/verifier/verifier")" ||
  {
    echo "Can't load the verifier binary!" >&2
    exit 1
  }

function test_single_files() {
  test_count=0
  for verilog_file in $(ls -d "${TESTS_DIR}"/*); do
    if [[ -d "${verilog_file}" ]]; then
      continue
    fi
    test_filename="$(basename "${verilog_file}")"
    test_dir="${TEST_TMPDIR}/${test_filename%.*}"
    mkdir "${test_dir}"
    cp "${verilog_file}" "${test_dir}"
    filelist_path="${test_dir}/filelist"
    echo "${test_filename}" > "${filelist_path}"

    echo "Running Kythe verification test for ${test_filename}" >> "$TEST_log"
    "${VERIBLE_EXTRACTOR_BIN}" --print_kythe_facts_proto "${filelist_path}" > "${test_dir}/entries" ||
      fail "Failed to extract Kythe facts"
    cat "${test_dir}/entries" | "${KYTHE_VERIFER_BIN}" --nocheck_for_singletons "${test_dir}/${test_filename}" >> "$TEST_log" ||
      fail "Verification failed for ${test_filename}"
    test_count=$((${test_count} + 1))
  done
  [[ ${test_count} -gt 0 ]] || fail "No tests are executed!"
}

function test_multi_files() {
  test_count=0
  for test_case_dir in $(ls -d "${TESTS_DIR}"/*); do
    if [[ -f "${test_case_dir}" ]]; then
      continue
    fi
    test_name="$(basename "${test_case_dir}")"
    test_dir="${TEST_TMPDIR}/${test_name}"
    mkdir "${test_dir}"
    cp "${test_case_dir}"/* "${test_dir}/"
    filelist_path="${test_dir}/filelist"
    ls "${test_case_dir}" > "${filelist_path}"

    echo "Running Kythe verification multi file test for ${test_name}" >> "$TEST_log"
    "${VERIBLE_EXTRACTOR_BIN}" --print_kythe_facts_proto "${filelist_path}" > "${test_dir}/entries" ||
      fail "Failed to extract Kythe facts"
    cat "${test_dir}/entries" | "${KYTHE_VERIFER_BIN}" "${test_dir}"/*.* >> "$TEST_log" ||
      fail "Verification failed for ${test_name}"
    test_count=$((${test_count} + 1))
  done
  [[ ${test_count} -gt 0 ]] || fail "No tests are executed!"
}

run_suite "kythe verification tests"
