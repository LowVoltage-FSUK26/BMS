# BQ79600 / BQ79616 Battery Monitoring System

A firmware-level battery monitoring system built on the **BQ79600** (Bridge) and **BQ79616** (Slaves) ICs, communicating with an **STM32** MCU over UART.

---

## Table of Contents

- [System Overview](#system-overview)
- [Communication Interface](#communication-interface)
- [Initialization](#initialization)
  - [Wake-Up Sequence](#wake-up-sequence)
  - [Auto-Addressing](#auto-addressing)
- [Cell Voltage Configuration](#cell-voltage-configuration)
  - [Active Cell Selection](#1-active-cell-selection)
  - [OV/UV Thresholds](#2-ovuv-thresholds)
  - [OV/UV Control Mode](#3-ovuv-control-mode)
  - [OV/UV Status Check](#4-ovuv-status-check)
- [Cell Voltage Reading](#cell-voltage-reading)
- [GPIO & Temperature Configuration](#gpio--temperature-configuration)
  - [OT/UT Thresholds](#1-otut-thresholds)
  - [Comparator Threshold](#2-comparator-threshold)
  - [TSREF Enable](#3-tsref-enable)
  - [GPIO Configuration](#4-gpio-configuration)
  - [OT/UT Control Mode](#5-otut-control-mode)
  - [OT/UT Status Check](#6-otut-status-check)
- [GPIO Voltage Reading](#gpio-voltage-reading)
- [Fault Handling](#fault-handling)
- [System Flow Summary](#system-flow-summary)

---

## System Overview

The system performs battery monitoring using the following hardware:

| Component     | Role                  | Connection              |
|---------------|-----------------------|-------------------------|
| **BQ79600**   | Bridge                | Connected to STM32 MCU  |
| **BQ79616**   | Slaves (×N)           | Daisy-chained to Bridge |

The Bridge interfaces with the MCU on one side and connects to multiple slave devices in a **daisy-chain** topology on the other. Each slave is connected in series to the next.

> **Reference:** Section 7.3.1.3 — SPI/UART Selection | Section 5 — Pin Configuration and Functions

---

## Communication Interface

Two communication methods are supported between the MCU and the Bridge:

- **SPI**
- **UART** ✅ *(selected in this design)*

**Selected Configuration:** UART at **1 Mbps** baud rate.

---

## Initialization

The initialization phase must follow proper hardware setup and covers:

1. Wake-up sequence
2. Auto-addressing
3. Cell voltage configuration
4. GPIO configuration

### Wake-Up Sequence

> **Reference:** Section 7.3.1.2 — Pings

The wake-up procedure is as follows:

1. **Deinitialize UART:**
   ```c
   UART_DeInit();
   ```

2. **Configure the TX pin as GPIO.**

3. **Drive the TX pin LOW for approximately 2.5 ms**
   *(this value is not strict and can be adjusted).*

4. **Reinitialize UART:**
   ```c
   UART_Init();
   ```

After wake-up, communication continues normally using UART frames.

> **Reference:** Section 7.3.2 — Communication

---

### Auto-Addressing

> **Reference:** Section 7.3.2.3.1 — Auto-Addressing

Auto-addressing assigns a unique address to each device on the bus:

- The **Bridge** (BQ79600)
- Each **Slave** (BQ79616)

Addresses are assigned **sequentially starting from 0**.

---

## Cell Voltage Configuration

> **Reference:** Section 9.3.2.1.1 — Cell Voltage Measurements

### 1. Active Cell Selection

Not all 16 cells are required to be active. Configure the number of active cells using:

| Parameter | Value   |
|-----------|---------|
| Register  | `ACTIVE_CELL` |
| Page      | 128     |

---

### 2. OV/UV Thresholds

Set overvoltage and undervoltage protection thresholds:

| Parameter | Value        |
|-----------|--------------|
| Registers | `OV_THRESH`, `UV_THRESH` |
| Page      | 160          |

> **Reference:** Section 9.3.4.1 — OVUV Protectors

---

### 3. OV/UV Control Mode

Configure the operating mode for OV/UV monitoring:

- **Continuous mode**
- **One-shot mode**

| Parameter | Value       |
|-----------|-------------|
| Register  | `OVUV_CTRL` |
| Page      | 161         |

> **Reference:** Section 9.3.4.1.2 — OVUV Control and Status

---

### 4. OV/UV Status Check

Verify that OV/UV monitoring is active:

| Parameter | Value                  |
|-----------|------------------------|
| Register  | `DEV_STAT[OVUV_RUN]`  |
| Page      | 135                    |

---

## Cell Voltage Reading

> **Reference:** Section 9.5.4.6 — ADC Measurement Results (Page 140)

### Data Format

Each measurement is **16-bit**, composed of:
- High byte
- Low byte

### Voltage Calculation

```
Voltage (V) = ADC_Value × VLSB_ADC
```

Where:
- `VLSB_ADC = 0.00019073 V/LSB`

> **Reference:** ADC Resolution (Page 14)

### Function Signature

```c
uint8_t readBoardVoltages(uint8_t  boardNum,
                          uint8_t  numCells,
                          int     *totalV,
                          uint16_t *cellVoltages);
```

### Data Storage

All cell voltage readings from all slave boards are stored in a 2D array:

```c
uint16_t cellVoltages_board[SLAVEBOARDS][16];
```

---

## GPIO & Temperature Configuration

> **Reference:** Section 9.3.2.1.2 — Temperature Measurements

### GPIO Modes

GPIO pins support the following operating modes:

| Mode            | Description                    |
|-----------------|--------------------------------|
| Digital Input   | Standard logic input           |
| Digital Output  | Standard logic output          |
| ADC Only        | Analog measurement only        |
| **ADC + OT/UT** | ✅ Used in this design         |

---

### 1. OT/UT Thresholds

Set over-temperature and under-temperature protection thresholds:

| Parameter | Value          |
|-----------|----------------|
| Register  | `OTUT_THRESH`  |
| Page      | 161            |

> **Reference:** Section 9.3.4.2 — OTUT Protector

---

### 2. Comparator Threshold

| Parameter | Value          |
|-----------|----------------|
| Register  | `OTCB_THRESH`  |

---

### 3. TSREF Enable

Enable the internal reference voltage for temperature sensors:

| Parameter | Value                   |
|-----------|-------------------------|
| Register  | `CONTROL2[TSREF_EN]`   |
| Page      | 132                     |

> **Reference:** Section 9.3.1.6 — TSREF

**Function:** Provides a **5V output** to power temperature sensors. The ground reference is taken from the same slave board.

---

### 4. GPIO Configuration

Set all relevant GPIOs to **ADC + OT/UT** mode using the following registers:

| Register     | Page |
|--------------|------|
| `GPIO_CONF1` | 162  |
| `GPIO_CONF2` | 162  |
| `GPIO_CONF3` | 162  |
| `GPIO_CONF4` | 162  |

> **Reference:** Section 9.5.4.9 — GPIO Configuration

---

### 5. OT/UT Control Mode

Configure the operating mode for OT/UT monitoring:

- **Continuous mode**
- **One-shot mode**

| Parameter | Value       |
|-----------|-------------|
| Register  | `OTUT_CTRL` |
| Page      | 161         |

> **Reference:** Section 9.3.4.2.2 — OTUT Control and Status

---

### 6. OT/UT Status Check

Verify that OT/UT monitoring is active:

| Parameter | Value                  |
|-----------|------------------------|
| Register  | `DEV_STAT[OTUT_RUN]`  |
| Page      | 135                    |

---

## GPIO Voltage Reading

> **Reference:** Section 9.5.4.6 — ADC Measurement Results

### Voltage Calculation

```
Voltage (V) = ADC_Value × VLSB_DIAG
```

Where:
- `VLSB_DIAG = 152.59 µV/LSB`

> **Reference:** Electrical Characteristics (Page 15)

### Function Signature

```c
float readGPIOVoltage(uint8_t   BID,
                      uint8_t   GPIO_NUM,
                      uint16_t *raw_value_ptr);
```

---

## Fault Handling

After completing cell voltage and temperature monitoring, the system must detect and respond to faults.

### Fault Types

| Fault | Description           |
|-------|-----------------------|
| **OV** | Over Voltage         |
| **UV** | Under Voltage        |
| **OT** | Over Temperature     |
| **UT** | Under Temperature    |

### Required Actions

1. **Read** fault status registers
2. **Identify** the fault type
3. **Take appropriate action** based on severity:
   - 🔴 **Shutdown** — critical fault, halt operation
   - 🟡 **Warning** — non-critical, alert the system
   - 📋 **Logging** — record the event for diagnostics

---

## System Flow Summary

The complete system execution flow proceeds as follows:

```
1. Hardware Setup
       ↓
2. Initialization
   ├── Wake-Up Sequence
   └── Auto-Addressing
       ↓
3. Cell Voltage Configuration
   ├── Active Cell Selection
   ├── OV/UV Threshold Setup
   ├── OV/UV Control Mode
   └── Status Verification
       ↓
4. Cell Voltage Reading
       ↓
5. GPIO / Temperature Configuration
   ├── OT/UT Threshold Setup
   ├── Comparator Threshold
   ├── TSREF Enable
   ├── GPIO Mode Configuration
   ├── OT/UT Control Mode
   └── Status Verification
       ↓
6. Temperature Reading
       ↓
7. Fault Detection & Handling
```
