"""
Standalone test harness for BMS_GUI.py.

Feeds the dashboard synthetic UART lines instead of opening a real COM
port, so it can be exercised end-to-end (cells, GPIO temps, TSREF,
faults) without a board or TTL adapter attached. Delete this file once
real hardware is available — it's a dev convenience, not part of the
shipped GUI.

Run:  python test_gui_simulator.py
"""
import random
import runpy
import time

GPIO_LSB = 0.00015259  # matches VLSB_GPIO / GPIO_LSB in the real code
CELL_LSB = 0.00019073  # matches VOLT_CONV / hex_to_cell_voltage
NUM_BOARDS = 32  # matches NUM_SLAVES in BMS_GUI.py


class FakeSerial:
    """Drop-in replacement for serial.Serial that manufactures believable
    S1..S32 frames in the 5-field wire format on a ~50ms cadence, matching
    the firmware's own Send_GUI_Reading() timing. Cycles through every
    board (including two-digit numbers like S10/S23/S32) so the GUI's
    board-number parsing gets exercised end-to-end."""

    def __init__(self, *args, **kwargs):
        self._pending = []
        self._last_push = 0.0
        self._next_board = 1

    @property
    def in_waiting(self):
        now = time.time()
        if now - self._last_push >= 0.05:
            self._last_push = now
            self._pending.append(self._make_line())
        return len(self._pending)

    def readline(self):
        if self._pending:
            return (self._pending.pop(0) + "\n").encode()
        return b""

    def _make_line(self):
        board = self._next_board
        self._next_board = self._next_board + 1 if self._next_board < NUM_BOARDS else 1

        cells = [f"{int(random.uniform(3.55, 3.85) / CELL_LSB):04X}" for _ in range(16)]

        v_tsref = 3.0
        tsref_hex = f"{int(v_tsref / GPIO_LSB):04X}"

        # Vgpio/Vtsref ratio chosen so R_ntc lands roughly in a 15-35C band
        # via the real R_ntc = R_PULLUP * Vgpio / (Vtsref - Vgpio) formula.
        gpios = []
        for _ in range(5):
            v_gpio = random.uniform(0.28, 0.55) * v_tsref
            gpios.append(f"{int(v_gpio / GPIO_LSB):04X}")

        fault = "00"
        return f"S{board}|{','.join(cells)}|{','.join(gpios)}|{tsref_hex}|{fault}"


# Patch pyserial before BMS_GUI.py's module-level
# `ser = serial.Serial("COM4", 9600, timeout=1)` line ever runs.
import serial
serial.Serial = FakeSerial

runpy.run_path("BMS_GUI.py", run_name="__main__")
