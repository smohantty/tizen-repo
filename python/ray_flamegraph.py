#!/usr/bin/env python3
import shutil
import subprocess
import sys
import shlex
import os
import webbrowser

# Remote paths (on Tizen device)
PERF_DATA = "/tmp/perf.data"
PERF_TXT  = "/tmp/perf.script"

# Local output file
SVG_FILE  = "/tmp/flamegraph.svg"

def check_prerequisites():
    """Ensure sdb and inferno tools exist on the HOST."""
    missing = []
    if not shutil.which("sdb"):
        missing.append((
            "sdb",
            "Install the Tizen SDK (provides `sdb`).\n"
            "  Download: https://developer.tizen.org/development/sdk-download"
        ))
    if not shutil.which("inferno-flamegraph") or not shutil.which("inferno-collapse-perf"):
        missing.append((
            "inferno",
            "Install Rust inferno tools:\n"
            "  cargo install inferno"
        ))
    if missing:
        print("Missing prerequisites:\n")
        for name, hint in missing:
            print(f"  - {name} not found. {hint}\n")
        sys.exit(1)

def run_sdb(cmd):
    """Run a command inside the Tizen device shell and capture output."""
    return subprocess.check_output(["sdb", "shell", cmd], text=True)

def main():
    check_prerequisites()

    print("\nSelect recording mode:")
    print(" 1) Spawn app under perf on device")
    print(" 2) Attach to a running app by PID/name on device")
    print(" 3) System-wide (-a) recording on device")
    mode = input("Choice [1/2/3]: ").strip()

    # Common perf options: 99 Hz sampling, DWARF call graph
    perf_opts = "-F 99 --call-graph dwarf -g -o {data}".format(data=PERF_DATA)

    if mode == "1":
        app = input("Enter app launch command (on device): ").strip()
        record_cmd = f"perf record {perf_opts} -- {app}"
    elif mode == "2":
        name = input("Enter process name substring: ").strip()
        ps_out = run_sdb(f"ps -ef | grep {shlex.quote(name)} | grep -v grep")
        if not ps_out.strip():
            print("No matching process found on device.")
            sys.exit(1)
        pid = ps_out.split()[1]
        print(f"Found PID {pid}")
        record_cmd = f"perf record {perf_opts} -p {pid}"
    elif mode == "3":
        record_cmd = f"perf record {perf_opts} -a"
    else:
        print("Invalid choice.")
        sys.exit(1)

    print("\nStarting perf on device…")
    print("Press Enter to stop recording.")
    rec_proc = subprocess.Popen(["sdb", "shell", record_cmd], stdin=subprocess.PIPE)

    input()  # Wait until user presses Enter
    rec_proc.terminate()       # send SIGTERM to perf
    rec_proc.wait()

    print("Converting perf.data -> perf.script on device…")
    subprocess.check_call(["sdb", "shell", f"perf script -i {PERF_DATA} > {PERF_TXT}"])

    print("Pulling perf.script to host …")
    subprocess.check_call(["sdb", "pull", PERF_TXT, PERF_TXT])

    print("Generating flamegraph.svg …")
    with open(SVG_FILE, "w") as outsvg:
        collapse = subprocess.Popen(
            ["inferno-collapse-perf", PERF_TXT],
            stdout=subprocess.PIPE)
        subprocess.check_call(
            ["inferno-flamegraph"],
            stdin=collapse.stdout,
            stdout=outsvg)

    print(f"Flamegraph generated: {SVG_FILE}")
    webbrowser.open(f"file://{SVG_FILE}")

if __name__ == "__main__":
    main()
