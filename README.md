# Geekstyles v2 Nixie Clock Firmware
## Getting Started

**Clone the repository with submodules:**
```bash
git clone --recursive https://github.com/siliverst/IN12v3clock.git
git submodule update --init --recursive
```


## Compiling and Flashing
You will need to install **SDCC (Small Device C Compiler)** to compile this firmware.

1. Compile the firmware by simply running:
```bash
make
```
This will generate a `real_clock.ihx` file.

2. To flash the firmware to your clock, you will need an **ST‑Link v2** programmer and the `stm8flash` tool. Clone and build `stm8flash` yourself:
```bash
git clone https://github.com/vdudouyt/stm8flash.git
cd stm8flash
make
```
Once `stm8flash` is built, connect your ST‑Link v2 to the clock's SWIM header and run:
```bash
./stm8flash -c stlinkv2 -p stm8s003f3 -w ../real_clock.ihx
```

## Hardware Configuration
*   **Microcontroller:** STM8S003F3
*   **Driver Architecture:** 8x SN74HC595 shift registers in a 64-bit chain.
*   **Pins Used:**
    *   `PC5`: Latch (STCP)
    *   `PC7`: Data (DS)
    *   `PD3`: Clock (SHCP)
*   **Logic:** Active High (1 turns on a digit).

## Digit Mapping
The shift register chain maps digits sequentially without any scrambling! The formula to find the bit index for any digit is simply:
`Bit Index = (Tube_Index * 10) + Digit`

*   **Tube 0 (Rightmost, Seconds Ones):** Bits 0 - 9
*   **Tube 1 (Seconds Tens):** Bits 10 - 19
*   **Tube 2 (Minutes Ones):** Bits 20 - 29
*   **Tube 3 (Minutes Tens):** Bits 30 - 39
*   **Tube 4 (Hours Ones):** Bits 40 - 49
*   **Tube 5 (Leftmost, Hours Tens):** Bits 50 - 59
*   **Bits 60 - 63:** Unused / Blank.

## Tube Brightness (The Output Enable Issue)
Initially, we attempted to use Hardware PWM on the `PD2` pin (which controls the Output Enable for the shift registers) to dim the tubes. However, we discovered that on the Geekstyles v2 PCB, the OE pins for 5 out of the 6 tubes are physically hardwired to Ground! 
*   **Solution:** We permanently pulled `PD2` low (Ground) to enable all tubes, and implemented **Software PWM** inside the main `display_digits` loop. By shifting out blank digits for a fraction of the 900Hz main loop, we achieved smooth dimming across all 6 tubes natively in software.

## Button Inputs
The Geekstyles v2 board features 3 buttons mapped to **PORTA**:
*   `PA1` (Right Button)
*   `PA2` (Middle Button)
*   `PA3` (Left Button)
*   **Debounce Logic:** Because of extreme hardware bouncing, we implemented an 8-bit shift-register debounce in software, requiring 8 consecutive identical reads before registering a state change.

## Colons
The colons are completely independent from the shift-register chain and are controlled by raw GPIO pins on **PORTD**:
*   `PD4`: Left Colon Top Dot
*   `PD5`: Right Colon Top Dot
*   **Hardware Limitation:** Testing confirmed that the bottom dots are either missing or completely disconnected on this specific PCB revision. They are currently configured to be permanently ON.

## RGB Underlighting
The tubes feature RGB underlighting controlled by **PORTC**:
*   `PC3`: Red
*   `PC6`: Green
*   `PC4`: Blue
*   **The Hardware PWM Trap:** We attempted to use the 16-bit `TIM1` Hardware PWM to drive the LEDs. Red and Blue worked flawlessly, but Green was completely dead. We discovered that `PC6` is mapped to `SPI_MOSI` by default in the STM8 hardware. To use `TIM1_CH1` on `PC6`, the "Alternate Function" Option Bytes must be flashed. Since they were not set on this board, Hardware PWM was impossible.
*   **Solution:** We bypassed the hardware limitations entirely by writing a 10kHz Software PWM engine that runs flawlessly inside the `TIM4` hardware interrupt vector (`TIM4_UPD_OVF_IRQHandler`). This ensures completely jitter-free, high-resolution color control independent of the main loop. (We later simplified this to an instant solid-color cycle at the user's request).

## Time Tracking & RTC Resiliency
The system uses a DS3231 RTC module over I2C.
*   Because I2C reads take time and can block the loop, we migrated the 1ms master system tick to `TIM2`. 
*   `TIM2` runs a continuous 16-bit 1MHz hardware counter. Its 65.5ms overflow buffer safely absorbs any I2C or code jitter.
*   **Failover Mode:** We implemented a robust software-failover mechanism. If the DS3231 RTC chip is removed, damaged, or misses a deadline, the firmware detects it and automatically forces the clock to continue ticking forward using its internal `TIM2` measurements.


## Credits & Acknowledgements
*   **Original Hardware:** Based on the Geekstyles IN12/IN14 Nixie Clocks.
*   **Althrone Repository:** https://github.com/althrone
*   **IN12v3 Repository:** https://github.com/siliverst/IN12v3clock
*   **Firmware Rewrite & Reverse Engineering:** Fully reverse-engineered, optimized, and rewritten by Antigravity AI.
*   **I2C & DS3231 Libraries:** Built upon the standard STM8 libraries for I2C and RTC communication.
