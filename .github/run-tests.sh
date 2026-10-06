#!/bin/bash
# Boot xv6 in QEMU, run syscalltests and usertests -q, and fail if
# either reports a failure. Used by CI; also works locally.
set -u

log=qemu.log
rm -f in "$log"
mkfifo in
make qemu CPUS=3 < in > "$log" 2>&1 &
qemu=$!
exec 3> in

finish() {
  kill $qemu 2>/dev/null
  pkill -f qemu-system-riscv64 2>/dev/null
  rm -f in
  cat "$log"
  exit "$1"
}

# wait_for PATTERN SECONDS: wait until the log matches PATTERN.
wait_for() {
  for _ in $(seq "$2"); do
    grep -aqE "$1" "$log" && return 0
    sleep 1
  done
  echo "timed out waiting for: $1"
  finish 1
}

wait_for 'init: starting sh' 120
echo syscalltests >&3
wait_for 'SYSCALLTESTS (PASSED|FAILED)' 120
grep -aq 'ALL SYSCALLTESTS PASSED' "$log" || finish 1

echo 'usertests -q' >&3
wait_for 'ALL TESTS PASSED|FAILED' 1200
grep -aq 'ALL TESTS PASSED' "$log" || finish 1
finish 0
