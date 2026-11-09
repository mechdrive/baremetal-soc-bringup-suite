"""
soc_inspect.py - GDB Python Extension for Silicon Bringup Introspection
Author: Poonam Singh
"""

import gdb

class SoCInspect(gdb.Command):
    """Inspect Silicon Bringup peripheral register maps and test suite results."""

    def __init__(self):
        super(SoCInspect, self).__init__("soc-inspect", gdb.COMMAND_USER)

    def invoke(self, arg, from_tty):
        print("==================================================================")
        print("          SILICON BRINGUP GDB INTROSPECTION CONSOLE               ")
        print("==================================================================")
        inferior = gdb.selected_inferior()

        # Read Bringup Stats from memory if symbol available
        try:
            stats = gdb.parse_and_eval("s_stats")
            print(f"Total Tests Run   : {stats['total_tests']}")
            print(f"Passed Diagnostics: {stats['passed_tests']}")
            print(f"Failed Diagnostics: {stats['failed_tests']}")
            print(f"Total Duration    : {stats['total_duration_us']} us")
        except Exception:
            print("[INFO] Target s_stats symbol not currently in scope.")

        print("==================================================================")

SoCInspect()
print("[GDB-EXT] Loaded 'soc-inspect' command successfully.")

