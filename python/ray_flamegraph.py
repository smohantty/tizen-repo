#!/usr/bin/env python3
import shutil
import subprocess
import sys
import shlex
import webbrowser

# Remote (Tizen device) paths
PERF_DATA = "/tmp/perf.data"
PERF_TXT  = "/tmp/perf.script"

# Local output
SVG_FILE  = "/tmp/flamegraph.svg"

def check_prerequisites():
    """Check host requirements: sdb and inferno utilities."""
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

    # perf options: DWARF call graph for full stack traces
    perf_opts = "-F 99 --call-graph dwarf -g -o {data}".format(data=PERF_DATA)

    if mode == "1":
        app = input("Enter app launch command (on device): ").strip()
        record_cmd = f"perf record {perf_opts} -- {app}"

    elif mode == "2":
        # Ask for a unique substring of the executable name
        name = input("Enter process name substring (e.g. rayvision): ").strip()
        # Use [r] trick to avoid matching the grep itself and anchor to end-of-line
        ps_cmd = (
            f"ps -eo pid,comm,args | grep '[{name[0]}]{shlex.quote(name[1:])}$'"
        )
        ps_out = run_sdb(ps_cmd)
        if not ps_out.strip():
            print("No matching process found on device.")
            sys.exit(1)
        # Take the first column (PID) of the first matching line
        pid = ps_out.split()[0]
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
    rec_proc.terminate()
    rec_proc.wait()

    print("Converting perf.data -> perf.script on device…")
    subprocess.check_call(["sdb", "shell", f"perf script -i {PERF_DATA} > {PERF_TXT}"])

    print("Pulling perf.script to host …")
    subprocess.check_call(["sdb", "pull", PERF_TXT, PERF_TXT])

    print("Generating flamegraph.svg …")
    with open(SVG_FILE, "w") as outsvg:
        collapse = subprocess.Popen(
            ["inferno-collapse-perf", PERF_TXT],
            stdout=subprocess.PIPE
        )
        subprocess.check_call(
            ["inferno-flamegraph"],
            stdin=collapse.stdout,
            stdout=outsvg
        )

    print(f"Flamegraph generated: {SVG_FILE}")
    webbrowser.open(f"file://{SVG_FILE}")

if __name__ == "__main__":
    main()
