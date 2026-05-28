# Developer / LLM Context Documentation for Geekstyles v2 Nixie Clock

This file contains the complete hardware, firmware, and compiler context required for an AI or human developer to understand, maintain, or port this clock's firmware.

## 1. Hardware Overview
*   **Microcontroller:** STM8S003F3 (8-bit MCU, 16MHz internal clock, 8KB Flash, 1KB RAM)
*   **Compiler:** SDCC (Small Device C Compiler) targeting the STM8 (`-mstm8`).
*   **Programming:** Flashed via ST-Link v2 using `stm8flash`.
*   **RTC:** DS3231 connected over I2C.
*   **Tubes:** 6x IN-12 Nixie tubes (Seconds, Minutes, Hours) driven by 8 cascaded SN74HC595 shift registers.

## 2. Shift Register Mapping (64-bit Chain)
The hardware uses eight 8-bit shift registers (64 bits total) to drive the digits.
*   **Latch Pin (STCP):** `PC5`
*   **Data Pin (DS):** `PC7`
*   **Clock Pin (SHCP):** `PD3`
*   **Logic:** Active High (1 = digit ON, 0 = digit OFF).
*   **Bit Mapping Formula:** `Bit Index = (Tube_Index * 10) + Digit`
    *   Tube 0 (Rightmost, Secs Ones): Bits 0-9
    *   Tube 1 (Secs Tens): Bits 10-19
    *   ...
    *   Tube 5 (Leftmost, Hrs Tens): Bits 50-59
    *   Bits 60-63: Unused.

## 3. Tube Brightness & Output Enable (OE) Quirks
A major hardware limitation on the Geekstyles v2 PCB dictates the software architecture:
*   The Output Enable (OE) pins for the shift registers *should* theoretically be tied to `PD2` for hardware PWM dimming.
*   **The Hardware Bug:** On this revision, OE for 5 out of the 6 tubes is hardwired directly to Ground. `PD2` only controls the OE for a subset of the tubes.
*   **The Software Solution:** `PD2` is permanently tied LOW (`sfr_PORTD.ODR.ODR2 = 0`) to keep all tubes unconditionally enabled. Dimming is achieved natively in software. The main `display_digits()` loop runs at ~900Hz. To dim the tubes, we shift out entirely blank data arrays (all 0s) for a certain percentage of those cycles.

## 4. Colons (Neon Dots)
The colons between the tubes are NOT attached to the shift registers. They are raw GPIO pins on `PORTD`.
*   **Left Colon Top Dot:** `PD4` (Active High)
*   **Right Colon Top Dot:** `PD5` (Active High)
*   *Note:* The bottom dots are physically missing or disconnected on this PCB revision. Do not attempt to toggle them.

## 5. RGB Underlighting & Timer Conflicts
The clock features an RGB LED under each tube, driven globally by `PORTC`:
*   **Red:** `PC3`
*   **Blue:** `PC4`
*   **Green:** `PC6`
*   **The Hardware Trap:** We originally tried to use `TIM1` Hardware PWM. While `PC3` and `PC4` mapped cleanly to `TIM1_CH3` and `TIM1_CH4`, `PC6` failed. `PC6` is `SPI_MOSI` by default, and requires the microcontroller's hardware Option Bytes to be set for the Alternate Function `TIM1_CH1`. Because we flashed via `stm8flash` over factory hardware, the Option Bytes were missing.
*   **The Software Solution:** We implemented a 10kHz timer interrupt using `TIM4` (`void TIM4_UPD_OVF_IRQHandler(void) __interrupt(23)`). This ISR generates high-resolution software PWM in the background, completely agnostic to hardware mapping issues and perfectly flicker-free. The LEDs are Active Low (0 = ON).

## 6. Button Debouncing
The board features three physical buttons mapped to `PORTA`:
*   **Right (Brightness):** `PA1`
*   **Middle (Mode):** `PA2`
*   **Left (Set):** `PA3`
*   These buttons suffer from intense hardware bouncing. We implemented an 8-bit shift-register debounce in software (`update_button_state`). A button's state is only considered "changed" if 8 consecutive reads (spanning 8ms) all agree on the new state.

## 7. Master Tick & RTC Architecture
I2C communication with the DS3231 takes a variable amount of time (around 1-2ms), which completely wrecks simple main-loop delay implementations.
*   **Timer Accumulator:** `TIM2` is configured to run at 1MHz (1us per tick). A 16-bit hardware counter continuously increments, overflowing every 65.5ms. The main loop tracks this timer and accumulates 1ms system ticks (`sys_tick`). This guarantees millisecond accuracy even if an I2C transaction stalls for 5ms.
*   **Failover Logic:** The system asks the RTC for the exact time once per second. However, it independently maintains a software shadow clock (`centiseconds`, `seconds`, `minutes`, `hours`). If the I2C read fails or the DS3231 is removed, the software shadow clock continues running off the precision `TIM2` accumulator flawlessly.

## 8. Complete Microcontroller Pin Map
For absolute clarity, here is the complete map of every active pin on the STM8S003F3 for this board:
*   **PORTA:**
    *   `PA1`: Right Button (Brightness/Adjust Up) - Input Pull-up
    *   `PA2`: Middle Button (Mode/Next) - Input Pull-up
    *   `PA3`: Left Button (Set/Enter) - Input Pull-up
*   **PORTB:**
    *   `PB4`: I2C SCL (to DS3231)
    *   `PB5`: I2C SDA (to DS3231)
*   **PORTC:**
    *   `PC3`: Red LED (Active Low)
    *   `PC4`: Blue LED (Active Low)
    *   `PC5`: Shift Register Latch (STCP)
    *   `PC6`: Green LED (Active Low)
    *   `PC7`: Shift Register Data (DS)
*   **PORTD:**
    *   `PD2`: Shift Register Output Enable (OE). Hardwired to multiple tubes. Tied permanently LOW in software.
    *   `PD3`: Shift Register Clock (SHCP)
    *   `PD4`: Left Colon Top Dot (Active High)
    *   `PD5`: Right Colon Top Dot (Active High)

## 9. The Data Shifting Protocol (`display_digits`)
Because the STM8 has no hardware DMA for generic shift registers, the 64-bit payload is shifted out manually in a tight C loop.
*   **Interrupt Masking:** Global interrupts MUST be briefly masked (`__asm__("sim")`) before the shift and unmasked (`__asm__("rim")`) after. If the `TIM4` 10kHz interrupt fires *during* the `SHCP` clock pulse, the shift register timings are violated, causing visual glitching across the tubes.
*   **Timing:** The hardware requires distinct latching states. `PC5` (STCP) is held low, data is asserted on `PC7` (DS), `PD3` (SHCP) is pulsed high then low, and finally `PC5` is pulled high to lock the latch.

## 10. User Interface & Anti-Poisoning
*   **Zero-to-Nine Mode:** Nixie tubes suffer from "cathode poisoning" if digits sit unused for weeks. A software mode (toggled via `PA2`) rapidly cycles all tubes from 0 through 9 once a second to exercise all cathodes.
*   **Brightness Control:** Toggled via `PA1`. Modifies the `brightness_percent` variable. Because OE is hard-grounded, the `display_digits` loop conditionally sends a 64-bit array of purely `0`s based on `sys_tick % 10` to native-dim the tubes at 900Hz.

## 11. Compiler Notes & Toolchain
*   **Compiler:** SDCC (Small Device C Compiler)
*   **Build Command:** `sdcc -mstm8 --std-sdcc99 --std-c99 --opt-code-speed -ISTM8_headers/include -c real_clock.c`
*   **Linking:** `sdcc -mstm8 ds3231.rel i2c.rel real_clock.rel -o real_clock.ihx`
*   **Flasher:** `stm8flash`
*   **Flash Command:** `stm8flash -c stlinkv2 -p stm8s003f3 -w real_clock.ihx`
*   **Code Quirks:** Always use `volatile` for variables modified inside ISRs (`r_duty`, `g_duty`, `sys_tick`). Direct register access uses the `sfr_PORTX` structures defined in `STM8_headers/include/STM8S003F3.h` (e.g., `sfr_PORTC.ODR.ODR6 = 0`).

## 12. Unresolved Hardware Mysteries & Unknowns
While the firmware fully controls the clock, there are a few lingering hardware mysteries on the Geekstyles v2 PCB that an AI should be aware of:
1.  **The Bottom Colon Dots:** The top dots of the two colons are successfully mapped to `PD4` and `PD5`. However, the bottom dots never illuminate. It is unknown if they are wired to a hidden GPIO pin we haven't tested, if they are tied to one of the unused shift register bits, or if they are simply physically disconnected/missing from the PCB entirely.
2.  **Unused Shift Register Bits (60-63):** The 8x SN74HC595 cascade provides 64 bits of output. We only use bits 0-59 for the 6 Nixie tubes (6 tubes * 10 digits). It is unknown if bits 60-63 are connected to anything (perhaps the missing colon dots?) or if they are entirely dead-ended on the PCB.
3.  **The Output Enable (OE) Discrepancy:** `PD2` is traced to the OE pin of the shift registers, which theoretically should allow global Hardware PWM dimming. However, on this specific board, the OE pins for 5 of the 6 tubes appear to be permanently hardwired to Ground. We do not know if this is a manufacturing defect specific to this batch, or a deliberate design choice by the creator for the v2 board. (We bypassed this by doing Software PWM dimming via the shift registers).
4.  **Hardware Option Bytes & The Original Repo:** The original IN12v3 documentation explicitly stated: *"Before flashing the firmware, set the option byte AFR0 in the ST Visual Programmer"*. Because we used `stm8flash` on macOS instead of the Windows STVP tool, we never flashed the option bytes. 
    *   **The Dump Confirmation:** We successfully dumped the Option Bytes via `stm8flash -s opt -r dump.bin` and confirmed that `OPT2` was `0x00`. This proved that `AFR0` was never set, meaning `TIM1_CH1` was physically locked out from reaching the `PC6` Green LED.
    *   > [!WARNING]
    >   > **DO NOT FLASH THE OPTION BYTES UNLESS ABSOLUTELY NECESSARY.** Flashing option bytes is extremely risky. If you accidentally overwrite the Read-Out Protection (ROP) byte or disable the Reset pin function, you can permanently and irreversibly brick the STM8 microcontroller. 
    *   **Our Solution:** Rather than risking a bricked chip to fix the hardware PWM, we simply wrote the 10kHz `TIM4` software interrupt to bypass the limitation entirely. This firmware works natively on factory-fresh STM8 chips without requiring dangerous Option Byte modifications!
