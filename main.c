#include<reg52.h>
#include<intrins.h>

#define LED_PORT P1
#define SEG_PORT P0
#define BIT_PORT P2

sbit KEY_SPEED = P3^2;

void delay_ms(unsigned int ms);
void display_no_delay(void);
unsigned char key_process(void);
void led_flow_no_delay(void);
void Timer0_Init(void);
void Timer0_ISR(void);
void show_status_code(unsigned char code1, unsigned char code2);

unsigned char led_pos = 0;
unsigned char count = 0;
unsigned char speed_level = 2;
unsigned int delay_time[] = {500, 300, 150};
unsigned char code seg_table[] = {
    0x3F, 0x06, 0x5B, 0x4F,
    0x66, 0x6D, 0x7D, 0x07,
    0x7F, 0x6F, 0x77, 0x7C,
    0x39, 0x5E, 0x79, 0x71
};
bit isRunning = 0;
bit show_status = 0;
unsigned int status_timer = 0;
bit isChangingSpeed = 0;
bit firstStart = 1;

unsigned char display_buffer[2] = {0, 0};
unsigned char display_pos = 0;
unsigned int led_timer = 0;

void delay_ms(unsigned int ms) {
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 100; j++);
}

void display_no_delay() {
    SEG_PORT = 0x00;
    switch(display_pos) {
        case 0: BIT_PORT = 0x01; break;
        case 1: BIT_PORT = 0x02; break;
    }
    SEG_PORT = seg_table[display_buffer[display_pos]];
    
    display_pos++;
    if(display_pos >= 2) display_pos = 0;
}

unsigned char key_process() {
    static unsigned char key_state = 0;
    static unsigned int press_time = 0;
    unsigned char result = 0;
    
    switch(key_state) {
        case 0:
            if(KEY_SPEED == 0) {
                key_state = 1;
                press_time = 0;
            }
            break;
            
        case 1:
            delay_ms(20);
            if(KEY_SPEED == 0) {
                key_state = 2;
                isChangingSpeed = 1;
            } else {
                key_state = 0;
            }
            break;
            
        case 2:
            delay_ms(10);
            press_time++;
            
            if(KEY_SPEED != 0) {
                if(press_time < 50) {
                    result = 1;
                    isChangingSpeed = 0;
                } else {
                    result = 2;
                    isRunning = 1;
                    speed_level++;
                    if(speed_level > 3) speed_level = 1;
                    show_status_code(speed_level, 0xA);
                }
                key_state = 0;
            } else if(press_time >= 50) {
                show_status_code(speed_level, 0xA);
                key_state = 3;
            }
            break;
            
        case 3:
            if(KEY_SPEED != 0) {
                result = 2;
                key_state = 0;
                isChangingSpeed = 0;
                isRunning = 1;
                speed_level++;
                if(speed_level > 3) speed_level = 1;
                show_status_code(speed_level, 0xA);
            } else {
                show_status_code(speed_level, 0xA);
            }
            break;
    }
    
    return result;
}

void led_flow_no_delay() {
    if(firstStart) {
        led_pos = 0;
        count = 1;
        firstStart = 0;
    } else {
        led_pos++;
        if(led_pos >= 8) {
            led_pos = 0;
        }
        
        count++;
        if(count > 99) count = 0;
    }
    
    LED_PORT = 0xFF;
    LED_PORT &= ~(0x01 << led_pos);
}

void Timer0_Init() {
    TMOD |= 0x01;
    TH0 = 0xFC;
    TL0 = 0x66;
    ET0 = 1;
    TR0 = 1;
    EA = 1;
}

void Timer0_ISR() interrupt 1 {
    TH0 = 0xFC;
    TL0 = 0x66;
    
    display_no_delay();
    
    if(show_status) {
        status_timer++;
        if(status_timer >= 500) {
            show_status = 0;
        }
    }
    
    if(isRunning && !show_status && !isChangingSpeed) {
        led_timer++;
        if(led_timer >= delay_time[speed_level-1]) {
            led_timer = 0;
            led_flow_no_delay();
        }
    }
}

void show_status_code(unsigned char code1, unsigned char code2) {
    if(code1 > 15) code1 = 0;
    if(code2 > 15) code2 = 0;
    
    display_buffer[0] = code1;
    display_buffer[1] = code2;
    show_status = 1;
    status_timer = 0;
}

void main() {
    unsigned char key_result;
    
    LED_PORT = 0xFF;
    Timer0_Init();
    
    while(1) {
        key_result = key_process();
        
        if(key_result == 1) {
            isRunning = !isRunning;
            if(isRunning) {
                show_status_code(0xC, 0xA);
            } else {
                show_status_code(0x0, 0x5);
            }
        } 
        
        if(!show_status && !isChangingSpeed) {
            display_buffer[0] = count % 10;
            display_buffer[1] = count / 10;
        }
    }
}
