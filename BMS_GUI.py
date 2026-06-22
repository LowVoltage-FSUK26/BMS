import tkinter as tk
from tkinter import ttk, font as tkfont
import serial

# =========================================
# Configuration
# =========================================
NUM_SLAVES = 2
BRIDGE_NAME = "S0"
SLAVE_NAMES = [f"S{i}" for i in range(1, NUM_SLAVES + 1)]
ALL_MODULES = [BRIDGE_NAME] + SLAVE_NAMES

# =========================================
# UART Setup
# =========================================
ser = serial.Serial("COM4", 9600, timeout=1)

# =========================================
# Design Tokens
# =========================================
COLORS = {
    "bg_root":       "#0d1117",
    "bg_card":       "#161b22",
    "bg_card_inner": "#1c2128",
    "bg_cell":       "#21262d",
    "bg_header":     "#0d1117",
    "accent_blue":   "#58a6ff",
    "accent_red":    "#f85149",
    "accent_green":  "#3fb950",
    "accent_yellow": "#e3b341",
    "accent_purple": "#bc8cff",
    "text_primary":  "#e6edf3",
    "text_muted":    "#8b949e",
    "border":        "#30363d",
    "fault_ok":      "#238636",
    "fault_err":     "#da3633",
    "btn_bg":        "#21262d",
    "btn_hover":     "#30363d",
    "btn_border":    "#30363d",
    "separator":     "#21262d",
}

FONTS = {
    "display":   ("Consolas", 22, "bold"),
    "heading":   ("Consolas", 13, "bold"),
    "subhead":   ("Consolas", 11, "bold"),
    "body":      ("Consolas", 10),
    "small":     ("Consolas", 9),
    "cell_val":  ("Consolas", 11, "bold"),
    "cell_lbl":  ("Consolas", 8),
    "mono_lg":   ("Courier New", 14, "bold"),
}

# =========================================
# Data Storage
# =========================================
uart_data = {}
for s in SLAVE_NAMES:
    uart_data[s] = {
        "cells": ",".join(["0000"] * 16),
        "gpio":  ",".join(["0000"] * 4),
        "fault": "00"
    }
uart_data[BRIDGE_NAME] = {"fault": "0"}

# =========================================
# GUI Storage
# =========================================
cell_labels        = {}
gpio_labels        = {}
slave_total_labels = {}
slave_frames       = {}
fault_labels       = {}

# =========================================
# Conversion Helpers
# =========================================
def hex_to_cell_voltage(hex_str):
    return round(int(hex_str, 16) * 0.00019073, 3)

def hex_to_gpio_voltage(hex_str):
    return round(int(hex_str, 16) * 152.59 / 1_000_000, 3)

# =========================================
# Toggle Slave View
# =========================================
def toggle_slave(slave_name):
    frame = slave_frames[slave_name]
    if frame.winfo_ismapped():
        frame.grid_remove()
    else:
        row_index = 4 + list(slave_frames.keys()).index(slave_name)
        frame.grid(row=row_index, column=0, pady=(0, 24), sticky="ew", padx=40)

# =========================================
# Parse UART Line
# =========================================
def parse_uart_line(line):
    parts = line.split("|")
    if len(parts) < 2:
        return
    slave = parts[0]
    if slave not in uart_data:
        return
    if slave != BRIDGE_NAME:
        if len(parts) >= 4:
            uart_data[slave]["cells"] = parts[1]
            uart_data[slave]["gpio"]  = parts[2]
            uart_data[slave]["fault"] = parts[3]
    else:
        if len(parts) >= 2:
            uart_data[slave]["fault"] = parts[1]
    update_gui()

# =========================================
# UART Reader
# =========================================
def read_uart():
    latest_line = None
    while ser.in_waiting:
        try:
            latest_line = ser.readline().decode().strip()
        except Exception:
            pass
    if latest_line:
        parse_uart_line(latest_line)
    root.after(100, read_uart)

# =========================================
# Update GUI
# =========================================
def update_gui():
    pack_total = 0
    for slave in uart_data:
        if slave != BRIDGE_NAME:
            hex_cells   = uart_data[slave]["cells"].split(",")
            cell_values = [hex_to_cell_voltage(h) for h in hex_cells]
            slave_total = sum(cell_values)
            pack_total += slave_total

            slave_total_labels[slave].config(
                text=f"▸  {slave}  {slave_total:.3f} V"
            )
            for i in range(16):
                v = cell_values[i]
                color = (COLORS["accent_red"] if v < 2.8 or v > 4.2
                         else COLORS["accent_yellow"] if v < 3.0
                         else COLORS["accent_green"])
                cell_labels[slave][i].config(
                    text=f"C{i+1:02d}\n{v:.3f}V",
                    fg=color
                )
            hex_gpio   = uart_data[slave]["gpio"].split(",")
            gpio_values = [hex_to_gpio_voltage(h) for h in hex_gpio]
            for i, value in enumerate(gpio_values):
                color = (COLORS["accent_red"] if value < 0.5 or value > 3.3
                         else COLORS["accent_green"])
                gpio_labels[slave][i].config(
                    text=f"GPIO{i+1}  {value:.3f} V",
                    bg=color
                )

        fault_hex = uart_data[slave]["fault"]
        try:
            fault_bits = bin(int(fault_hex, 16))[2:].zfill(
                8 if slave != BRIDGE_NAME else 4
            )
        except Exception:
            return

        fault_names = (
            ["FAULT_COMM", "FAULT_REG", "FAULT_SYS", "FAULT_PWR"]
            if slave == BRIDGE_NAME else
            ["FAULT_PROT", "FAULT_COMP_ADC", "FAULT_OTP", "FAULT_COMM",
             "FAULT_OTUT", "FAULT_OVUV", "FAULT_SYS", "FAULT_PWR"]
        )
        for i, name in enumerate(fault_names):
            state = fault_bits[i]
            bg = COLORS["fault_err"] if state == "1" else COLORS["fault_ok"]
            symbol = "● ACTIVE" if state == "1" else "○ OK"
            fault_labels[slave][i].config(
                text=f"{name}\n{symbol}",
                bg=bg
            )

    pack_total_label.config(
        text=f"⚡  PACK TOTAL  {pack_total:.3f} V"
    )

# =========================================
# Utility: Rounded-look card frame
# =========================================
def make_card(parent, **kwargs):
    outer = tk.Frame(parent, bg=COLORS["border"], padx=1, pady=1)
    inner = tk.Frame(outer, bg=kwargs.get("bg", COLORS["bg_card"]),
                     padx=kwargs.get("padx", 16),
                     pady=kwargs.get("pady", 12))
    inner.pack(fill="both", expand=True)
    return outer, inner

# =========================================
# Main Window
# =========================================
root = tk.Tk()
root.title("BMS Monitor  ·  UART Dashboard")
root.configure(bg=COLORS["bg_root"])

WIN_W, WIN_H = 1200, 700
sw = root.winfo_screenwidth()
sh = root.winfo_screenheight()
root.geometry(f"{WIN_W}x{WIN_H}+{(sw-WIN_W)//2}+{(sh-WIN_H)//2}")
root.minsize(900, 500)

# =========================================
# Styles
# =========================================
style = ttk.Style()
style.theme_use("clam")
style.configure("TFrame",      background=COLORS["bg_root"])
style.configure("Inner.TFrame", background=COLORS["bg_card"])
style.configure("TLabelframe",
                background=COLORS["bg_card_inner"],
                foreground=COLORS["text_primary"],
                bordercolor=COLORS["border"],
                relief="flat")
style.configure("TLabelframe.Label",
                background=COLORS["bg_card_inner"],
                foreground=COLORS["accent_blue"],
                font=FONTS["subhead"])
style.configure("TButton",
                background=COLORS["btn_bg"],
                foreground=COLORS["text_primary"],
                font=FONTS["body"],
                borderwidth=1,
                relief="flat",
                padding=(12, 6))
style.map("TButton",
          background=[("active", COLORS["btn_hover"])],
          foreground=[("active", COLORS["accent_blue"])])

# =========================================
# Root Layout: header + scrollable body
# =========================================
root.rowconfigure(1, weight=1)
root.columnconfigure(0, weight=1)

# ── Header ──────────────────────────────
header = tk.Frame(root, bg=COLORS["bg_header"], height=64)
header.grid(row=0, column=0, sticky="ew")
header.columnconfigure(0, weight=1)
header.grid_propagate(False)

tk.Label(
    header,
    text="⚡  BMS MONITOR",
    font=FONTS["display"],
    fg=COLORS["accent_blue"],
    bg=COLORS["bg_header"]
).grid(row=0, column=0, sticky="w", padx=32, pady=14)

tk.Frame(root, bg=COLORS["border"], height=1).grid(
    row=0, column=0, sticky="sew"
)

# ── Scrollable canvas ───────────────────
scroll_outer = tk.Frame(root, bg=COLORS["bg_root"])
scroll_outer.grid(row=1, column=0, sticky="nsew")
scroll_outer.rowconfigure(0, weight=1)
scroll_outer.columnconfigure(0, weight=1)

canvas = tk.Canvas(scroll_outer, bg=COLORS["bg_root"],
                   highlightthickness=0, bd=0)
scrollbar = tk.Scrollbar(scroll_outer, orient="vertical",
                         command=canvas.yview)

canvas.configure(yscrollcommand=scrollbar.set)
canvas.grid(row=0, column=0, sticky="nsew")
scrollbar.grid(row=0, column=1, sticky="ns")

# Mouse-wheel scrolling
def _on_mousewheel(event):
    canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
canvas.bind_all("<MouseWheel>", _on_mousewheel)

# Inner scrollable frame — centred in canvas
scrollable_frame = tk.Frame(canvas, bg=COLORS["bg_root"])
canvas_window = canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")

def _on_frame_configure(event):
    canvas.configure(scrollregion=canvas.bbox("all"))
    # Keep the inner frame at least as wide as the canvas
    min_w = event.width if event.width > 1 else WIN_W
    canvas.itemconfig(canvas_window, width=max(min_w, scrollable_frame.winfo_reqwidth()))

scrollable_frame.bind("<Configure>", _on_frame_configure)

def _on_canvas_resize(event):
    canvas.itemconfig(canvas_window,
                      width=max(event.width, scrollable_frame.winfo_reqwidth()))
canvas.bind("<Configure>", _on_canvas_resize)

# Configure scrollable_frame columns so children center
scrollable_frame.columnconfigure(0, weight=1)

# =========================================
# ── Section: Pack + Slave Totals ─────────
# =========================================
totals_outer, totals_inner = make_card(
    scrollable_frame, bg=COLORS["bg_card"], padx=32, pady=20
)
totals_outer.grid(row=0, column=0, sticky="ew", padx=40, pady=(28, 0))
totals_inner.columnconfigure(0, weight=1)

pack_total_label = tk.Label(
    totals_inner,
    text="⚡  PACK TOTAL  0.000 V",
    font=FONTS["display"],
    fg=COLORS["accent_red"],
    bg=COLORS["bg_card"]
)
pack_total_label.grid(row=0, column=0, pady=(4, 16))

# Divider
tk.Frame(totals_inner, bg=COLORS["border"], height=1).grid(
    row=1, column=0, sticky="ew", padx=8, pady=(0, 14)
)

slave_row_frame = tk.Frame(totals_inner, bg=COLORS["bg_card"])
slave_row_frame.grid(row=2, column=0)

for i, s in enumerate(SLAVE_NAMES):
    slave_row_frame.columnconfigure(i, weight=1, minsize=200)
    pill = tk.Frame(slave_row_frame, bg=COLORS["bg_cell"], padx=16, pady=8)
    pill.grid(row=0, column=i, padx=10)
    lbl = tk.Label(
        pill,
        text=f"▸  {s}  0.000 V",
        font=FONTS["heading"],
        fg=COLORS["accent_blue"],
        bg=COLORS["bg_cell"]
    )
    lbl.pack()
    slave_total_labels[s] = lbl

# =========================================
# ── Section: Toggle Buttons ──────────────
# =========================================
btn_outer, btn_inner = make_card(
    scrollable_frame, bg=COLORS["bg_card"], padx=24, pady=14
)
btn_outer.grid(row=1, column=0, sticky="ew", padx=40, pady=(16, 0))
btn_inner.columnconfigure(0, weight=1)

tk.Label(
    btn_inner,
    text="M O D U L E   V I S I B I L I T Y",
    font=FONTS["small"],
    fg=COLORS["text_muted"],
    bg=COLORS["bg_card"],
).grid(row=0, column=0, pady=(0, 10))

btn_row = tk.Frame(btn_inner, bg=COLORS["bg_card"])
btn_row.grid(row=1, column=0)

for i, s in enumerate(ALL_MODULES):
    btn_row.columnconfigure(i, weight=1)
    ttk.Button(
        btn_row,
        text=f"Toggle  {s}",
        command=lambda name=s: toggle_slave(name),
        width=16,
        style="TButton"
    ).grid(row=0, column=i, padx=6)

# =========================================
# ── Section: Slave Frames ────────────────
# =========================================
for row_idx, s in enumerate(ALL_MODULES):
    frame = ttk.LabelFrame(
        scrollable_frame,
        text=f"  {s}  DETAILS",
        padding=(20, 14)
    )
    slave_frames[s] = frame

    cell_labels[s]  = []
    gpio_labels[s]  = []
    fault_labels[s] = []

    # Center content inside LabelFrame
    frame.columnconfigure(0, weight=1)

    if s != BRIDGE_NAME:
        # ── Cell grid ──────────────────────
        cells_wrapper = tk.Frame(frame, bg=COLORS["bg_card_inner"])
        cells_wrapper.grid(row=0, column=0, pady=(0, 14))

        for i in range(16):
            col, row = i % 8, i // 8      # 8 per row → 2 rows of 8
            lbl = tk.Label(
                cells_wrapper,
                text=f"C{i+1:02d}\n0.000V",
                width=8,
                height=3,
                bg=COLORS["bg_cell"],
                fg=COLORS["accent_green"],
                font=FONTS["cell_val"],
                relief="flat",
                bd=0,
                highlightbackground=COLORS["border"],
                highlightthickness=1
            )
            lbl.grid(row=row, column=col, padx=3, pady=3)
            cell_labels[s].append(lbl)

        # ── GPIO ───────────────────────────
        gpio_outer = ttk.LabelFrame(frame, text="  ANALOG GPIO", padding=(12, 8))
        gpio_outer.grid(row=1, column=0, sticky="ew", pady=(0, 12))
        gpio_outer.columnconfigure(tuple(range(4)), weight=1)

        gpio_row = tk.Frame(gpio_outer, bg=COLORS["bg_card_inner"])
        gpio_row.grid(row=0, column=0, columnspan=4)
        for g in range(4):
            lbl = tk.Label(
                gpio_row,
                text=f"GPIO{g+1}  0.000 V",
                width=16,
                font=FONTS["body"],
                bg=COLORS["fault_ok"],
                fg="white",
                padx=8, pady=4,
                relief="flat"
            )
            lbl.grid(row=0, column=g, padx=5)
            gpio_labels[s].append(lbl)

        # ── Faults ─────────────────────────
        fault_outer = ttk.LabelFrame(frame, text="  FAULT SUMMARY", padding=(12, 8))
        fault_outer.grid(row=2, column=0, sticky="ew")

        fault_names_slave = [
            "FAULT_PROT", "FAULT_COMP_ADC", "FAULT_OTP", "FAULT_COMM",
            "FAULT_OTUT", "FAULT_OVUV",     "FAULT_SYS", "FAULT_PWR"
        ]
        fault_row = tk.Frame(fault_outer, bg=COLORS["bg_card_inner"])
        fault_row.grid(row=0, column=0, columnspan=8)

        for i, name in enumerate(fault_names_slave):
            lbl = tk.Label(
                fault_row,
                text=f"{name}\n○ OK",
                width=14,
                font=FONTS["small"],
                bg=COLORS["fault_ok"],
                fg="white",
                padx=4, pady=5,
                relief="flat"
            )
            lbl.grid(row=0, column=i, padx=3)
            fault_labels[s].append(lbl)

    else:
        # ── Bridge faults only ─────────────
        fault_outer = ttk.LabelFrame(frame, text="  FAULT SUMMARY", padding=(12, 8))
        fault_outer.grid(row=0, column=0, sticky="ew")

        fault_names_bridge = ["FAULT_COMM", "FAULT_REG", "FAULT_SYS", "FAULT_PWR"]
        fault_row = tk.Frame(fault_outer, bg=COLORS["bg_card_inner"])
        fault_row.grid(row=0, column=0, columnspan=4)

        for i, name in enumerate(fault_names_bridge):
            lbl = tk.Label(
                fault_row,
                text=f"{name}\n○ OK",
                width=16,
                font=FONTS["body"],
                bg=COLORS["fault_ok"],
                fg="white",
                padx=4, pady=6,
                relief="flat"
            )
            lbl.grid(row=0, column=i, padx=5)
            fault_labels[s].append(lbl)

# =========================================
# Status bar
# =========================================
status_bar = tk.Frame(root, bg=COLORS["border"], height=28)
status_bar.grid(row=2, column=0, sticky="ew")
status_bar.columnconfigure(0, weight=1)
status_bar.grid_propagate(False)

tk.Label(
    status_bar,
    text="● LIVE  ·  COM5  ·  115200 baud",
    font=FONTS["small"],
    fg=COLORS["accent_green"],
    bg=COLORS["border"]
).grid(row=0, column=0, sticky="w", padx=16)

tk.Label(
    status_bar,
    text="BMS Monitor v2.0",
    font=FONTS["small"],
    fg=COLORS["text_muted"],
    bg=COLORS["border"]
).grid(row=0, column=1, sticky="e", padx=16)

# =========================================
# Start UART Reading
# =========================================
read_uart()
root.mainloop()