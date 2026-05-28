#include "STM8S003F3.h"
#include "ds3231.h"

void delay_ms(uint16_t ms) {
    uint32_t i;
    for(i=0; i<((uint32_t)ms * 285); i++) {
        __asm__("nop");
    }
}

void delay_us(uint8_t us) {
    uint8_t i;
    for(i=0; i<us*4; i++) {
        __asm__("nop");
    }
}

void set_any_pin(uint8_t pin, uint8_t val) {
    switch(pin) {
        case 0: sfr_PORTA.ODR.ODR1 = val; break;
        case 1: sfr_PORTA.ODR.ODR2 = val; break;
        case 2: sfr_PORTA.ODR.ODR3 = val; break;
        case 3: sfr_PORTC.ODR.ODR3 = val; break;
        case 4: sfr_PORTC.ODR.ODR4 = val; break;
        case 5: sfr_PORTC.ODR.ODR5 = val; break;
        case 6: sfr_PORTC.ODR.ODR6 = val; break;
        case 7: sfr_PORTC.ODR.ODR7 = val; break;
        case 8: sfr_PORTD.ODR.ODR1 = val; break;
        case 9: sfr_PORTD.ODR.ODR2 = val; break;
        case 10: sfr_PORTD.ODR.ODR3 = val; break;
        case 11: sfr_PORTD.ODR.ODR4 = val; break;
        case 12: sfr_PORTD.ODR.ODR5 = val; break;
        case 13: sfr_PORTD.ODR.ODR6 = val; break;
        case 14: sfr_PORTB.ODR.ODR4 = val; break;
        case 15: sfr_PORTB.ODR.ODR5 = val; break;
    }
}

void shift_data(uint8_t *data) {
    uint8_t r, b, t;
    uint8_t c_pin = 10; // PD3 (Clock)
    uint8_t l_pin = 5;  // PC5 (Latch)
    uint8_t d_pin = 7;  // PC7 (Data)
    
    // Shift 8 full bytes (64 bits)
    for (r = 0; r < 8; r++){
        t = *data++;
        for (b = 0; b < 8; b++) {
            uint8_t bit = (t & 0x80) ? 1 : 0; 
            
            set_any_pin(d_pin, bit);
            delay_us(5);
            set_any_pin(c_pin, 1);
            delay_us(5);
            t <<= 1;
            set_any_pin(c_pin, 0);
            delay_us(5);
        }
    }
    
    set_any_pin(l_pin, 1);
    delay_us(10);
    set_any_pin(l_pin, 0);
}

void display_digits(uint8_t *digits) {
    uint8_t data[8];
    uint8_t i;
    
    for (i = 0; i < 8; i++) data[i] = 0;
    
    // IN14v2 exact mapping (digits array: [0]=s2, [1]=s1, [2]=m2, [3]=m1, [4]=h2, [5]=h1)
    for (i = 0; i < 6; i++) {
        uint8_t digit = digits[i];
        if (digit > 9) continue;
        
        uint8_t bitIndex = (i * 10) + digit;
        
        uint8_t byteIdx = 7 - (bitIndex / 8);
        uint8_t bitPos = bitIndex % 8;
        
        data[byteIdx] |= (1 << bitPos);
    }
    
    shift_data(data);
}

volatile uint8_t r_duty = 100;
volatile uint8_t g_duty = 0;
volatile uint8_t b_duty = 0;

void TIM4_UPD_OVF_IRQHandler(void) __interrupt(23) {
    sfr_TIM4.SR.UIF = 0; // Clear interrupt flag
    static uint8_t pwm_cnt = 0;
    pwm_cnt++;
    if (pwm_cnt >= 100) pwm_cnt = 0;
    
    // Active low (0 = ON, 1 = OFF)
    sfr_PORTC.ODR.ODR3 = (pwm_cnt < r_duty) ? 0 : 1; // PC3 (Red)
    sfr_PORTC.ODR.ODR6 = (pwm_cnt < g_duty) ? 0 : 1; // PC6 (Green)
    sfr_PORTC.ODR.ODR4 = (pwm_cnt < b_duty) ? 0 : 1; // PC4 (Blue)
}

void main(void) {
    // Disable interrupts during initialization
    __asm__("sim");
    
    // Clock setup
    sfr_CLK.CKDIVR.byte = 0x00;
    
    sfr_PORTA.DDR.DDR1 = 1; sfr_PORTA.CR1.C11 = 1; sfr_PORTA.CR2.C21 = 1;
    sfr_PORTA.DDR.DDR2 = 1; sfr_PORTA.CR1.C12 = 1; sfr_PORTA.CR2.C22 = 1;
    sfr_PORTA.DDR.DDR3 = 1; sfr_PORTA.CR1.C13 = 1; sfr_PORTA.CR2.C23 = 1;
    sfr_PORTC.DDR.DDR3 = 1; sfr_PORTC.CR1.C13 = 1; sfr_PORTC.CR2.C23 = 1;
    sfr_PORTC.DDR.DDR4 = 1; sfr_PORTC.CR1.C14 = 1; sfr_PORTC.CR2.C24 = 1;
    sfr_PORTC.DDR.DDR5 = 1; sfr_PORTC.CR1.C15 = 1; sfr_PORTC.CR2.C25 = 1;
    sfr_PORTC.DDR.DDR6 = 1; sfr_PORTC.CR1.C16 = 1; sfr_PORTC.CR2.C26 = 1;
    sfr_PORTC.DDR.DDR7 = 1; sfr_PORTC.CR1.C17 = 1; sfr_PORTC.CR2.C27 = 1;
    
    // Geekstyles v2 (IN14v2) uses PA3, PA2, PA1 for buttons
    sfr_PORTA.DDR.DDR3 = 0; sfr_PORTA.CR1.C13 = 1; sfr_PORTA.CR2.C23 = 0; sfr_PORTA.ODR.ODR3 = 1;
    sfr_PORTA.DDR.DDR2 = 0; sfr_PORTA.CR1.C12 = 1; sfr_PORTA.CR2.C22 = 0; sfr_PORTA.ODR.ODR2 = 1;
    sfr_PORTA.DDR.DDR1 = 0; sfr_PORTA.CR1.C11 = 1; sfr_PORTA.CR2.C21 = 0; sfr_PORTA.ODR.ODR1 = 1;
    
    // Remaining pins as outputs
    sfr_PORTD.DDR.DDR0 = 1; sfr_PORTD.CR1.C10 = 1; sfr_PORTD.CR2.C20 = 1;
    sfr_PORTD.DDR.DDR1 = 1; sfr_PORTD.CR1.C11 = 1; sfr_PORTD.CR2.C21 = 1;
    sfr_PORTD.DDR.DDR2 = 1; sfr_PORTD.CR1.C12 = 1; sfr_PORTD.CR2.C22 = 1;
    sfr_PORTD.DDR.DDR3 = 1; sfr_PORTD.CR1.C13 = 1; sfr_PORTD.CR2.C23 = 1;
    sfr_PORTD.DDR.DDR4 = 1; sfr_PORTD.CR1.C14 = 1; sfr_PORTD.CR2.C24 = 1;
    sfr_PORTD.DDR.DDR5 = 1; sfr_PORTD.CR1.C15 = 1; sfr_PORTD.CR2.C25 = 1;
    sfr_PORTD.DDR.DDR6 = 1; sfr_PORTD.CR1.C16 = 1; sfr_PORTD.CR2.C26 = 1;
    sfr_PORTB.DDR.DDR4 = 1; sfr_PORTB.CR1.C14 = 0; sfr_PORTB.CR2.C24 = 0;
    sfr_PORTB.DDR.DDR5 = 1; sfr_PORTB.CR1.C15 = 0; sfr_PORTB.CR2.C25 = 0;

    // Set ALL GPIO pins to 1, EXCEPT Latch (5), Clock (10), and RGB LEDs (3, 4, 6) which must idle LOW
    uint8_t i;
    for (i = 0; i < 16; i++) {
        if (i == 5 || i == 10 || i == 3 || i == 4 || i == 6) {
            set_any_pin(i, 0);
        } else {
            set_any_pin(i, 1);
        }
    }
    sfr_PORTD.ODR.ODR0 = 1; // Explicit pull-up for PD0 since it's not mapped in set_any_pin
    
    // Enable SR1 outputs (Active Low on PD2 / Pin 9) permanently
    sfr_PORTD.DDR.DDR2 = 1;
    sfr_PORTD.CR1.C12 = 1;
    sfr_PORTD.CR2.C22 = 1;
    set_any_pin(9, 0);
    
    // Initialize RTC
    ds3231Init();
    
    // Initialize TIM2 to act as an exact 1us hardware tick counter
    // Clock = 16MHz. Prescaler = 2^4 = 16. Timer freq = 1 MHz.
    // 16-bit counter wraps every 65.536 milliseconds, easily covering any I2C delays.
    sfr_TIM2.PSCR.PSC = 4; 
    sfr_TIM2.CR1.CEN = 1;
    
    // Initialize TIM4 for 10kHz interrupt (100us) for RGB Software PWM
    sfr_TIM4.PSCR.PSC = 4; // 16MHz / 16 = 1MHz
    sfr_TIM4.ARR.byte = 100;
    sfr_TIM4.IER.UIE = 1;
    sfr_TIM4.CR1.CEN = 1;
    
    // Initialize RGB LED Pins (PC3=R, PC4=B, PC6=G)
    sfr_PORTC.DDR.DDR3 = 1; sfr_PORTC.CR1.C13 = 1; sfr_PORTC.CR2.C23 = 1;
    sfr_PORTC.DDR.DDR4 = 1; sfr_PORTC.CR1.C14 = 1; sfr_PORTC.CR2.C24 = 1;
    sfr_PORTC.DDR.DDR6 = 1; sfr_PORTC.CR1.C16 = 1; sfr_PORTC.CR2.C26 = 1;
    
    // Enable global interrupts
    __asm__("rim");
    
    // Initial Brightness (100% is best for tube health)
    uint8_t brightness_percent = 100;
    
    uint16_t last_tim2 = 0;
    uint16_t us_accum = 0;
    
    uint8_t zeroToNineMode = 0;
    uint8_t zeroToNineCounter = 0;
    uint8_t last_zz_sec = 60;
    
    uint8_t btn_a_hist = 0, btn_b_hist = 0, btn_c_hist = 0;
    uint8_t btn_a_state = 1, btn_b_state = 1, btn_c_state = 1;
    uint8_t btn_a_cnt = 0, btn_b_cnt = 0, btn_c_cnt = 0;

    uint8_t seconds = 0, minutes = 0, hours = 0;
    uint16_t ticks = 0; // Milliseconds since last second
    uint8_t last_seconds = 60;

    while(1) {
        // ------------------
        // Time Tracking via TIM2
        // ------------------
        uint16_t current_tim2 = (sfr_TIM2.CNTRH.byte << 8) | sfr_TIM2.CNTRL.byte;
        uint16_t diff = current_tim2 - last_tim2; // handles 16-bit wrap automatically
        last_tim2 = current_tim2;
        
        us_accum += diff;
        
        while (us_accum >= 1000) {
            us_accum -= 1000;
            ticks++;
        }
        
        ds3231_read_time(&seconds, &minutes, &hours);
        
        // Sync the hardware millisecond timer to the precise RTC tick
        if (seconds != last_seconds) {
            ticks = 0;
            us_accum = 0; // Sync the 1ms loop exactly
            last_seconds = seconds;
        }
        
        // Fallback: If the RTC is missing or completely broken, it will miss its 1-second deadline.
        // At 1050 ticks, we force the clock to tick forward independently!
        if (ticks >= 1050) {
            ticks = 0;
            us_accum = 0;
            
            // Increment BCD Seconds
            seconds++;
            if ((seconds & 0x0F) == 0x0A) {
                seconds = (seconds & 0xF0) + 0x10;
            }
            if (seconds >= 0x60) {
                seconds = 0x00;
                // Increment BCD Minutes
                minutes++;
                if ((minutes & 0x0F) == 0x0A) {
                    minutes = (minutes & 0xF0) + 0x10;
                }
                if (minutes >= 0x60) {
                    minutes = 0x00;
                    // Increment BCD Hours
                    hours++;
                    if ((hours & 0x0F) == 0x0A) {
                        hours = (hours & 0xF0) + 0x10;
                    }
                    if (hours >= 0x24) {
                        hours = 0x00;
                    }
                }
            }
            last_seconds = seconds;
        }

        uint8_t centiseconds = ticks / 10;
        if (centiseconds > 99) centiseconds = 99; // Clamp
        
        // Shift-register debounce (requires 8 consecutive identical reads to change state)
        static uint8_t btn_a_hist = 0;
        static uint8_t btn_b_hist = 0;
        static uint8_t btn_c_hist = 0;
        
        static uint8_t btn_a_state = 0;
        static uint8_t btn_b_state = 0;
        static uint8_t btn_c_state = 0;
        
        btn_a_hist = (btn_a_hist << 1) | (sfr_PORTA.IDR.IDR3 == 0);
        btn_b_hist = (btn_b_hist << 1) | (sfr_PORTA.IDR.IDR2 == 0);
        btn_c_hist = (btn_c_hist << 1) | (sfr_PORTA.IDR.IDR1 == 0);
        
        // Reset states on 8 consecutive HIGH reads
        if (btn_a_hist == 0x00) btn_a_state = 0;
        if (btn_b_hist == 0x00) btn_b_state = 0;
        if (btn_c_hist == 0x00) btn_c_state = 0;

        // Trigger on 8 consecutive LOW reads
        if (btn_a_hist == 0xFF && btn_a_state == 0) {
            btn_a_state = 1;
            uint8_t z = 0;
            ds3231_write_time(&z, &z, &z);
            ticks = 0;
            centiseconds = 0;
            us_accum = 0;
            seconds = 0;
            minutes = 0;
            hours = 0;
            last_seconds = 0;
        }
        
        if (btn_b_hist == 0xFF && btn_b_state == 0) {
            btn_b_state = 1;
            zeroToNineMode = !zeroToNineMode;
            if (zeroToNineMode) {
                last_zz_sec = seconds;
                zeroToNineCounter = 0;
            }
        }
        
        if (btn_c_hist == 0xFF && btn_c_state == 0) {
            btn_c_state = 1;
            brightness_percent += 20;
            if (brightness_percent > 100) brightness_percent = 20; // wrap to 20 so it's never fully OFF
        }

        uint8_t digits[6];
        
        // Colons: Geekstyles v2 only has the top dots wired (PD4=Left, PD5=Right)
        // Keep them permanently ON
        sfr_PORTD.ODR.ODR4 = 1;
        sfr_PORTD.ODR.ODR5 = 1;
        
        // RGB Under-lighting: Smooth rainbow crossfade
        static uint8_t rgb_state = 0;
        static uint16_t color_timer = 0;
        
        color_timer++;
        if (color_timer >= 3) { // Extremely fast, smooth fade
            color_timer = 0;
            switch(rgb_state) {
                case 0: g_duty++; if (g_duty >= 100) rgb_state = 1; break; // Red to Yellow
                case 1: r_duty--; if (r_duty == 0)   rgb_state = 2; break; // Yellow to Green
                case 2: b_duty++; if (b_duty >= 100) rgb_state = 3; break; // Green to Cyan
                case 3: g_duty--; if (g_duty == 0)   rgb_state = 4; break; // Cyan to Blue
                case 4: r_duty++; if (r_duty >= 100) rgb_state = 5; break; // Blue to Magenta
                case 5: b_duty--; if (b_duty == 0)   rgb_state = 0; break; // Magenta to Red
            }
        }
        
        if (zeroToNineMode) {
            // Anti-poisoning mode: show cycling digits at 1 Hz based on RTC seconds
            if (seconds != last_zz_sec) {
                last_zz_sec = seconds;
                zeroToNineCounter = (zeroToNineCounter + 1) % 10;
            }
            uint8_t j;
            for (j = 0; j < 6; ++j) {
                digits[j] = zeroToNineCounter;
            }
        } else {
            // Normal display
            digits[5] = (minutes >> 4) & 0x0F; // Leftmost: Minutes Tens
            digits[4] = minutes & 0x0F;        // Minutes Ones
            digits[3] = (seconds >> 4) & 0x0F; // Seconds Tens
            digits[2] = seconds & 0x0F;        // Seconds Ones
            digits[1] = (centiseconds / 10) % 10; // Centiseconds Tens
            digits[0] = centiseconds % 10;     // Rightmost: Centiseconds Ones
        }
        
        // Geekstyles v2 Hardware Brightness Fix:
        // Because OE is hardwired to Ground for most tubes, we must use Software PWM.
        // The main loop runs at ~900Hz. By counting 0 to 9, we get a ~90Hz PWM cycle!
        static uint8_t pwm_counter = 0;
        pwm_counter++;
        if (pwm_counter >= 10) pwm_counter = 0;
        
        if (pwm_counter < (brightness_percent / 10)) {
            display_digits(digits);
        } else {
            uint8_t blanks[6] = {10, 10, 10, 10, 10, 10}; // 10 is skipped by display_digits, turning cathode OFF
            display_digits(blanks);
        }
    }
}
