#!/usr/bin/env python3

import os
import subprocess
import sys
import traceback

DAEMON_DIR = '/unv/Unvanquished/daemon'
BREAKPAD_DIR = '/unv/Unvanquished/daemon/libs/breakpad'
GAME_BUILD_DIR = '.'
ARCH = 'amd64'

SYMBOLIZE = os.path.join(BREAKPAD_DIR, "symbolize.py")
STACKWALK = os.path.join(BREAKPAD_DIR, "src/processor/minidump_stackwalk")

DAEMON_TTYCLIENT = os.path.join(GAME_BUILD_DIR, 'daemon-tty')
DAEMON_SERVER = os.path.join(GAME_BUILD_DIR, 'daemonded')
TEMP_DIR = os.path.join(GAME_BUILD_DIR, "crashtest-tmp")
os.makedirs(TEMP_DIR, exist_ok=True)
SYMBOL_DIR = os.path.join(TEMP_DIR, "symbols")
os.makedirs(SYMBOL_DIR, exist_ok=True)
HOMEPATH = os.path.join(TEMP_DIR, "homepath")



assert os.path.isfile(DAEMON_SERVER)
assert os.path.isfile(STACKWALK)


class CrashTest:
    def __init__(self, name):
        self.name = name

    def Begin(self):
        self.status = "PASSED"
        print(f"===RUNNING: {self.name}===")

    def End(self):
        print(f"==={self.status}: {self.name}===")

    def Verify(self, cond, reason):
        if not cond:
            print(f"FAILURE: {reason}")
            self.status = "FAILED"

    def Go(self):
        self.Begin()
        try:
            self.Do()
        except Exception as e:
            traceback.print_exception(e)
            self.status = "FAILED"
        self.End()
        return self.status == "PASSED"

class NaclCrashTest(CrashTest):
    def __init__(self, engine, tprefix, fault):
        super().__init__(f"nacl.{fault}")
        self.engine = engine
        self.tprefix = tprefix
        self.fault = fault

    def Do(self):
        print("Running daemon...")
        p = subprocess.run([self.engine, "-set", "vm.sgame.type", "1",
                            "-set", "vm.cgame.type", "1",
                        #"-set", "logs.level.fs", "warning",
                        "-set", "sv_fps", "1000",
                        "-set", "common.framerate.max", "0",
                        #"-set", "server.private", "2",
                        #"-set", "sv_networkScope", "0",
                        "-set", "net_enabled", "0",
                        "-set", "common.framerate.max", "0",
                        "-pakpath", "/unv/Unvanquished/pkg",
                        #"-homepath", HOMEPATH,
                        #"-pakpath", os.path.join(DAEMON_DIR, "pkg"),
                        #"-set", "fs_basepak", "testdata",
                        "+devmap plat23",
                        "+delay 20f echo CRASHTEST_BEGIN",
                        f"+delay 20f {self.tprefix}injectFault", self.fault,
                        "+delay 20f echo CRASHTEST_END",
                        "+delay 40f quit"],
        stderr=subprocess.PIPE,      check=False)
        log = [s.strip() for s in p.stderr.decode("utf8").splitlines()]
        i = log.index("CRASHTEST_BEGIN")
        #j = log.index("CRASHTEST_END")
        j = len(log) - 1
        # TODO expected vs. actual Warn's
        DUMP_PREFIX = "Wrote crash dump to "
        dumps = [l for l in log if l.startswith(DUMP_PREFIX)]
        assert len(dumps) == 1, "Daemon log contains 1 crash dump"
        dump = dumps[0][len(DUMP_PREFIX):]
        sw_out = os.path.join(TEMP_DIR, self.tprefix + self.fault + ".stackwalk.log")
        with open(sw_out, "a+") as sw_f:
            print(f"Extracting stack trace to '{sw_out}'...")
            sw_f.truncate()
            subprocess.run([STACKWALK, dump, SYMBOL_DIR], check=True, stdout=sw_f, stderr=subprocess.STDOUT)
            sw_f.seek(0)
            sw = sw_f.read()
        TRACE_FUNC = "InjectFaultCmd::Run"
        self.Verify(TRACE_FUNC in sw, "function names not found in trace (did you build with symbols?)")

def DoModule(module):
    try:
        engine = DAEMON_TTYCLIENT
        if module == "sgame":
            target = os.path.join(GAME_BUILD_DIR, f"sgame-{ARCH}.nexe")
            tprefix = "sgame."
        elif module == "cgame":
            target = os.path.join(GAME_BUILD_DIR, f"cgame-{ARCH}.nexe")
            tprefix = "cgame."
        elif module == "server":
            tprefix = ""
            engine = target = DAEMON_SERVER

        assert os.path.isfile(target), target
        print(f"Symbolizing '{target}'...")
        subprocess.check_call([sys.executable, SYMBOLIZE, "--symbol-directory", SYMBOL_DIR, target])

        return (True
            & NaclCrashTest(DAEMON_TTYCLIENT, tprefix, "exception").Go()
            & NaclCrashTest(DAEMON_TTYCLIENT, tprefix, "throw").Go()
            & NaclCrashTest(DAEMON_TTYCLIENT, tprefix, "abort").Go()
            & NaclCrashTest(DAEMON_TTYCLIENT, tprefix, "segfault").Go())
    except Exception:
        raise
        return False

#passed = DoModule("sgame") & DoModule("cgame")
passed = DoModule("server")
sys.exit(1 - passed)

