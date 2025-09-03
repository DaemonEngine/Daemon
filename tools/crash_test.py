#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
import traceback
import zipfile

class CrashTest:
    def __init__(self, name):
        self.name = name

    def Begin(self):
        self.status = "PASSED"
        print(f"===RUNNING: {self.name}===")

    def End(self):
        print(f"==={self.status}: {self.name}===")

    def Verify(self, cond, reason=None):
        if not cond:
            if reason is not None:
                print(f"FAILURE: {reason}")
            self.status = "FAILED"
            if GIVE_UP:
                raise Exception("Giving up on first failure")

    def Go(self):
        self.Begin()
        try:
            self.Do()
        except Exception as e:
            traceback.print_exception(e)
            self.status = "FAILED"
        self.End()
        return self.status == "PASSED"

class BreakpadCrashTest(CrashTest):
    def __init__(self, module, engine, tprefix, fault):
        super().__init__(module + "." + fault)
        self.engine = engine
        self.tprefix = tprefix
        self.fault = fault
        self.dir = os.path.join(TEMP_DIR, self.name)
        try:
            shutil.rmtree(self.dir)
        except FileNotFoundError:
            pass
        os.makedirs(self.dir)

    def Do(self):
        vmtype = "0" if SYMBOL_ZIPS else "1"
        print("Running daemon...")
        p = subprocess.run([self.engine,
                            "-set", "vm.sgame.type", vmtype,
                            "-set", "vm.cgame.type", vmtype,
                            "-set", "sv_fps", "1000",
                            "-set", "common.framerate.max", "0",
                            "-set", "client.errorPopup", "0",
                            "-set", "server.private", "2",
                            "-set", "net_enabled", "0",
                            "-set", "common.framerate.max", "0",
                            "-set", "cg_navgenOnLoad", "0",
                            "-homepath", self.dir,
                            *DAEMON_USER_ARGS,
                            *["+devmap plat23"] * (self.tprefix == "sgame."),
                            "+delay 20f echo CRASHTEST_BEGIN",
                            f"+delay 20f {self.tprefix}injectFault", self.fault,
                            "+delay 20f echo CRASHTEST_END",
                            "+delay 40f quit"],
                            stderr=subprocess.PIPE, check=bool(self.tprefix))
        dumps = os.listdir(os.path.join(self.dir, "crashdump"))
        assert len(dumps) == 1, dumps
        dump = os.path.join(self.dir, "crashdump", dumps[0])
        sw_out = os.path.join(TEMP_DIR, self.name + "_stackwalk.log")
        with open(sw_out, "a+") as sw_f:
            print(f"Extracting stack trace to '{sw_out}'...")
            sw_f.truncate()
            subprocess.run(Virtualize([os.path.join(BREAKPAD_DIR, "src/processor/minidump_stackwalk"), dump, SYMBOL_DIR]), check=True, stdout=sw_f, stderr=subprocess.STDOUT)
            sw_f.seek(0)
            sw = sw_f.read()
        TRACE_FUNC = "InjectFaultCmd::Run"
        self.Verify(TRACE_FUNC in sw, "function names not found in trace (did you build with symbols?)")

def Virtualize(cmdline):
    bin, *args = cmdline
    if EXE:
        bin2 = bin.replace("\\", "/")
        if bin2.startswith("//wsl.localhost/"):
            parts = bin2.split("/")
            vm = parts[3]
            path = "/" + "/".join(parts[4:])
            return ["wsl", "-d", vm, "--", path] + args
        if bin.endswith(".py"):
            return [sys.executable] + cmdline
    return cmdline

def ModulePath(module):
    base = {
        "dummyapp" : "dummyapp" + EXE,
        "server": "daemonded" + EXE,
        "ttyclient": "daemon-tty" + EXE,
        "client": "daemon" + EXE,
        "sgame": f"sgame-{NACL_ARCH}.nexe",
        "cgame": f"cgame-{NACL_ARCH}.nexe",
    }[module]
    return os.path.join(GAME_BUILD_DIR, base)

class ModuleCrashTests(CrashTest):
    def __init__(self, module, engine=None):
        super().__init__(module)
        self.engine = engine

    def Do(self):
        module = self.name
        if module == "sgame":
            eng = self.engine or "server"
            tprefix = "sgame."
        elif module == "cgame":
            tprefix = "cgame."
            eng = self.engine or "ttyclient"
        else:
            tprefix = ""
            eng = module
        engine = ModulePath(eng)
        assert os.path.isfile(engine), engine

        if not SYMBOL_ZIPS:
            target = ModulePath(module)
            assert os.path.isfile(target), target
            print(f"Symbolizing '{target}'...")
            subprocess.check_call(Virtualize([os.path.join(BREAKPAD_DIR, "symbolize.py"),
                                              "--symbol-directory", SYMBOL_DIR, target]))

        self.Verify(BreakpadCrashTest(module, engine, tprefix, "segfault").Go())
        if tprefix or not EXE:
            # apparently abort() is caught on Linux but not Windows
            self.Verify(BreakpadCrashTest(module, engine, tprefix, "abort").Go())
        if tprefix:
            self.Verify(BreakpadCrashTest(module, engine, tprefix, "exception").Go())
            self.Verify(BreakpadCrashTest(module, engine, tprefix, "throw").Go())

def ArgParser(usage=None):
    ap = argparse.ArgumentParser(usage=usage)
    ap.add_argument("--breakpad-dir", type=str, default=BREAKPAD_DIR, help=r"Path to Breakpad repo containing built dump_syms and stackwalk binaries. It may be a \\wsl.localhost\ path on Windows hosts in order to symbolize NaCl.")
    ap.add_argument("--give-up", action="store_true", help="Stop after first test failure")
    ap.add_argument("--nacl-arch", type=str, choices=["amd64", "i686", "armhf"], default="amd64") # TODO auto-detect?
    ap.add_argument("module", nargs="*",
        default="server", # bogus default needed due to buggy argparse
        choices=["dummyapp", "server", "ttyclient", "client",
        "cgame", "ttyclient:cgame", "client:cgame",
        "sgame", "server:sgame", "ttyclient:sgame", "client:sgame"])
    return ap

BREAKPAD_DIR = os.path.abspath(os.path.join(
    os.path.dirname(os.path.realpath(__file__)), "../libs/breakpad"))
GAME_BUILD_DIR = '.' # WSL calls rely on relative paths
TEMP_DIR = os.path.join(GAME_BUILD_DIR, "crashtest-tmp")
SYMBOL_DIR = os.path.join(TEMP_DIR, "symbols")

os.makedirs(TEMP_DIR, exist_ok=True)
os.makedirs(SYMBOL_DIR, exist_ok=True)

if os.name == "nt":
    EXE = '.exe'
else:
    EXE = ""

ap = ArgParser(usage=ArgParser().format_usage().rstrip().removeprefix("usage: ")
               + " [--daemon-args ARGS...]")
ap.add_argument("--daemon-args", nargs=argparse.REMAINDER, default=[],
                help="Extra arguments for Daemon (e.g. -pakpath)")
pa = ap.parse_args(sys.argv[1:])
BREAKPAD_DIR = pa.breakpad_dir
GIVE_UP = pa.give_up
DAEMON_USER_ARGS = pa.daemon_args
NACL_ARCH = pa.nacl_arch
SYMBOL_ZIPS = [p for p in os.listdir(GAME_BUILD_DIR) if p.startswith("symbols") and p.endswith(".zip")]
modules = pa.module
if isinstance(modules, str):
    modules = ["server", "ttyclient", "sgame", "cgame"]

if SYMBOL_ZIPS:
    print("Symbol zip(s) detected. Using release validation mode with pre-built symbols")
    for z in SYMBOL_ZIPS:
        with zipfile.ZipFile(z, 'r') as z:
            z.extractall(SYMBOL_DIR)
else:
    print("No symbol zip detected. Using end2end Breakpad tooling test mode with dump_syms")

passed = True
for module in modules:
    passed &= ModuleCrashTests(*module.split(":")[::-1]).Go()
    if not passed and GIVE_UP:
        break
sys.exit(1 - passed)
