/*
 * File:   major_project.c
 * Author: ASHWIN-PC
 * 
 * Created on 04 May 2022, 7:26 PM
 */
#include <xc.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "22S1_ELEC3042_I2C_PCF8574.h"
#include "22S1_ELEC3042_SPI.h"


// volatile variables that will change in ISR
void displayButtons(uint8_t arr);
void displayStateColor(char col);
//void setLights(char LS, char BN, char BST, char BS, char BP);

struct Light {
    char LS;
    char BN;
    char BST;
    char BS;
    char BP;

};

struct Light trafficLights;
struct Light lastTrafficLights;

uint8_t LCD_Addr = 0x27;

enum color {
    red, yellow, green
};
enum color lights = green;

enum MINMAX {
    min, max
};
enum MINMAX time = min;


// millis counter
volatile uint32_t millis = 0;
// timer to create millis time

ISR(TIMER2_COMPA_vect) {
    millis++;
}


volatile uint16_t adc = 0;
// ADC free running mode

ISR(ADC_vect) {
    adc = ADC;
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint32_t timePeriod;
uint32_t lastTimeFun = 0;
// function to read and map ADC value for period counter

uint32_t timeFun() {

    if (timePeriod > 99999) {
        timePeriod = 0;
    }
    uint16_t scale = map(adc, 0, 1023, 8, 100);
    if (millis - lastTimeFun >= scale) {
        timePeriod++;
        lastTimeFun = millis;
    }
    return timePeriod;
}

enum ONOFF {
    ON, OFF
};

enum BUTTON {
    pressed, released
};
// button press holders

enum STATES {
    HAZARD, BW_STRAIGHT, BW_TURN, LITTLE_ST, PEDESTRIAN
};

enum STATES STATE = HAZARD;
enum STATES NEXT_STATE = HAZARD;



//System input buttons ////////////////////////////////////////////////////////

// virtual buttons
enum BUTTON vs0 = released;
enum BUTTON vs1 = released;
enum BUTTON vs2 = released;
enum BUTTON vs4 = released;
enum BUTTON vs3 = released;
enum BUTTON vs5 = released;

//Real buttons
//MCP input  buttons
enum BUTTON s0 = released;
enum BUTTON s1 = released;
enum BUTTON s2 = released;
enum BUTTON s4 = released;
// Arduino input buttons
volatile enum BUTTON s3 = released;
volatile enum BUTTON s5 = released;
volatile enum BUTTON MCP_interrupt = released;
volatile enum BUTTON hazard = released;
// ISR to read MCP interrupts and IO pins

ISR(PCINT2_vect) {
    s3 = (PIND & _BV(7)) ? released : pressed;
    s5 = (PIND & _BV(4)) ? released : pressed;
    MCP_interrupt = (PIND & _BV(2)) ? released : pressed;
    hazard = (PIND & _BV(3)) ? released : pressed;
}
// function to read button on the MCP chip

void readMCP() {
    uint8_t portA = SPI_Read_Command(0x12);
    uint8_t portB = SPI_Read_Command(0x13);

    s0 = (portA & 1 << 0) ? released : pressed;
    s1 = (portA & 1 << 4) ? released : pressed;
    s2 = (portB & 1 << 0) ? released : pressed;
    s4 = (portB & 1 << 4) ? released : pressed;
}
//function to copy real button to virtual buttons.

void copyToVirtualButton() {

    if (s0 == pressed) {
        vs0 = pressed;
    }
    if (s1 == pressed) {
        vs1 = pressed;
    }
    if (s2 == pressed) {
        vs2 = pressed;
    }
    if (s3 == pressed) {
        vs3 = pressed;
    }
    if (s4 == pressed) {
        vs4 = pressed;
    }
    if (s5 == pressed) {
        vs5 = pressed;
    }
}

//Function to read and display virtual buttons on LCD

void buttonRead() {
    uint8_t temp = 0;
    if (MCP_interrupt == pressed) {
        readMCP();
    }
    copyToVirtualButton();

    if (vs0 == pressed) {
        temp |= 1 << 0;
    } else {
        temp |= 0 << 0;
    }
    if (vs1 == pressed) {
        temp |= 1 << 1;
    } else {
        temp |= 0 << 1;
    }
    if (vs2 == pressed) {
        temp |= 1 << 2;
    } else {
        temp |= 0 << 2;
    }
    if (vs3 == pressed) {
        temp |= 1 << 3;
    } else {
        temp |= 0 << 3;
    }
    if (vs4 == pressed) {
        temp |= 1 << 4;
    } else {
        temp |= 0 << 4;
    }
    if (vs5 == pressed) {
        temp |= 1 << 5;
    } else {
        temp |= 0 << 5;
    }
    displayButtons(temp);
}
////////////////////////////////////////////////////////////////////////////////
// set the output traffic lights to their respected color.

void setLights(struct Light *current, struct Light *last) {

    if (last == current) {
        return;
    }

    if (last->LS != current->LS || last->BN != current->BN) {
        last->LS = current->LS;
        last->BN = current->BN;
        uint8_t B = 0;

        switch (current->LS) {
            case 'G':
                B |= 1 << 3;
                break;
            case 'Y':
                B |= 1 << 2;
                break;
            case 'R':
                B |= 1 << 1;
                break;
            case 'O':

                break;
        }
        switch (current->BN) {
            case 'G':
                B |= 1 << 7;
                break;
            case 'Y':
                B |= 1 << 6;
                break;
            case 'R':
                B |= 1 << 5;
                break;
            case 'O':

                break;
        }
        SPI_Send_Command(0x13, B);

    }

    if (last->BST != current->BST || last->BS != current->BS) {// lastBP == BP{
        last->BST = current->BST;
        last->BS = current->BS;
        uint8_t A = 0;

        switch (current->BST) {
            case 'G':
                A |= 1 << 7;
                break;
            case 'Y':
                A |= 1 << 6;
                break;
            case 'R':
                A |= 1 << 5;
                break;
            case 'O':

                break;
        }
        switch (current->BS) {
            case 'G':
                A |= 1 << 3;
                break;
            case 'Y':
                A |= 1 << 2;
                break;
            case 'R':
                A |= 1 << 1;
                break;
            case 'O':

                break;
        }
        SPI_Send_Command(0x12, A);
    }
    if (last->BP != current->BP) {
        last->BP = current->BP;
        switch (current->BP) {
            case 'G':
                PORTD &= ~_BV(5);
                PORTD |= _BV(6);
                break;
            case 'Y':
                break;
            case 'R':

                PORTD |= _BV(5);
                PORTD &= ~_BV(6);
                break;
            case 'O':
                PORTD &= ~_BV(6);
                PORTD &= ~_BV(5);
                break;
        }
    }

}

// setup the hardware

void setup() {
    DDRD = 0b01100000; //
    PORTD = 0b10011100; //set pullup on input pins
    DDRB = 0b00101110; //set 
    PORTB = 0b00000000; //set all pins to low
    DDRC = 0b00000000; //set pin d8=data out
    PORTC = 0b00110000;
    // Setup ADC
    DIDR0 = 0b00000001;
    ADMUX = 0b01000000;
    ADCSRA = 0b11101110; // Adc prescaller of 64, auto trigger, ISR
    ADCSRB = 0b00000000; // // free running mode

    //Setup button interrupts
    PCICR = 0b00000100;
    PCMSK2 = 0b10011100;

    // buzzer sound
    TCCR1A = 0b00000011;
    TCCR1B = 0b00011011;
    TIMSK1 = 0b00000000;

    // Setup millies timer 8bit timer 2 prescaler 128..
    TCCR2A = 0b00000010;
    TCCR2B = 0b00000101;
    OCR2A = 124;
    TIMSK2 = 0b00000010;

    // Enable global interrupts
    sei();
}

enum ONOFF blinkState;
unsigned long lastHazardBlink = 0;

void hazardBlink() {
    if (millis - lastHazardBlink >= 1000) {
        lastHazardBlink = millis;
        if (blinkState == OFF) {
            blinkState = ON;
            for (int i = 0; i < 5; i++) {
                trafficLights.BN = 'Y';
                trafficLights.BP = 'R';
                trafficLights.BS = 'Y';
                trafficLights.BST = 'Y';
                trafficLights.LS = 'Y';
            }
        } else {
            blinkState = OFF;
            trafficLights.BN = 'O';
            trafficLights.BP = 'O';
            trafficLights.BS = 'O';
            trafficLights.BST = 'O';
            trafficLights.LS = 'O';

        }
    }
}


enum ONOFF pedState = ON;
uint32_t lastPedBlink = 0;

void pedBlink() {
    if (timePeriod - lastPedBlink >= 5) {
        lastPedBlink = timePeriod;
        if (pedState == OFF) {
            pedState = ON;
            trafficLights.BP = 'G';
        } else {
            pedState = OFF;
            trafficLights.BP = 'O';
        }
    }
}


uint32_t lastTimeChirp = 0;
uint32_t lastChirp = 0;
uint8_t chirpPeriod = 50;
uint8_t chirpFreq = 200;
uint8_t chirpLenght = 10;
enum ONOFF chirpDecending = OFF;
uint8_t count = 200;

// handles all the chirp tones

void sharpChirp() {

    if (timePeriod - lastChirp >= chirpPeriod) {
        lastChirp = timePeriod;
    }

    if (timePeriod - lastChirp < chirpLenght) {
        OCR1A = chirpFreq;
        TCCR1A |= _BV(6);
    } else {
        TCCR1A &= ~_BV(6);
    }
    if (STATE == PEDESTRIAN && lights == green) {

        if (chirpDecending == ON) {

            if (timePeriod - lastTimeChirp >= 1) {
                lastTimeChirp = timePeriod;
                chirpPeriod = 1;
                chirpFreq = count -= 10;
                ;
                chirpLenght = 1;
                if (count <= 50) {
                    count = 200;
                    chirpDecending = OFF;
                }
            }
        } else {
            chirpPeriod = 10;
            chirpFreq = 50;
            chirpLenght = 5;
        }
    } else {
        chirpDecending = ON;
        chirpPeriod = 50;
        chirpFreq = 200;
        chirpLenght = 10;
    }
}


enum STATES lastDState = PEDESTRIAN;
enum STATES lastDNextState = PEDESTRIAN;
enum MINMAX lastDTime = max;
// function to display state, minMax and next state.

void displayState() {
    char haz[] = "HZD";
    char bsw[] = "BWS";
    char bwt[] = "BWT";
    char lst[] = "LST";
    char ped[] = "PED";
    char Dmin[] = "MIN";
    char Dmax[] = "MAX";
    if (lastDState != STATE) {

        LCD_Position(LCD_Addr, 0x40);
        switch (STATE) {
            case HAZARD:
                LCD_Write(LCD_Addr, haz, 3);
                break;
            case BW_STRAIGHT:
                LCD_Write(LCD_Addr, bsw, 3);
                break;
            case BW_TURN:
                LCD_Write(LCD_Addr, bwt, 3);
                break;
            case LITTLE_ST:
                LCD_Write(LCD_Addr, lst, 3);
                break;
            case PEDESTRIAN:
                LCD_Write(LCD_Addr, ped, 3);
                break;
        }
        lastDState = STATE;
        LCD_Position(LCD_Addr, 0x17);
    }
    if (lastDNextState != NEXT_STATE) {
        LCD_Position(LCD_Addr, 0x4d);
        switch (NEXT_STATE) {
            case HAZARD:
                LCD_Write(LCD_Addr, haz, 3);
                break;
            case BW_STRAIGHT:
                LCD_Write(LCD_Addr, bsw, 3);
                break;
            case BW_TURN:
                LCD_Write(LCD_Addr, bwt, 3);
                break;
            case LITTLE_ST:
                LCD_Write(LCD_Addr, lst, 3);
                break;
            case PEDESTRIAN:
                LCD_Write(LCD_Addr, ped, 3);
                break;

        }
        lastDNextState = NEXT_STATE;
        LCD_Position(LCD_Addr, 0x17);
    }

    if (lastDTime != time) {
        LCD_Position(LCD_Addr, 0x47);
        switch (time) {
            case min:

                LCD_Write(LCD_Addr, Dmin, 3);


                break;
            case max:
                LCD_Write(LCD_Addr, Dmax, 3);

                break;
        }
        lastDTime = time;
        LCD_Position(LCD_Addr, 0x17);
    }
}

uint8_t lastArr;
// function to display buttons on the lCD

void displayButtons(uint8_t arr) {

    if (arr == lastArr) {
        return;
    }

    for (int i = 0; i < 6; i++) {
        if ((arr & (1 << i)) != (lastArr & (1 << i))) {
            LCD_Position(LCD_Addr, i);
            if ((arr & 1 << i) == 0) {
                LCD_Write_Chr(LCD_Addr, 'O');
            } else {
                LCD_Write_Chr(LCD_Addr, 'X');
            }
        }
    }
    lastArr = arr;
    LCD_Position(LCD_Addr, 17);
}

char lastColor = 'O';
// function to display state light color on the LCD

void displayStateColor(char col) {

    if (col == lastColor) {
        return;
    }
    LCD_Position(LCD_Addr, 0x43);

    switch (col) {
        case 'G':
            LCD_Write_Chr(LCD_Addr, 'G');
            break;
        case 'R':
            LCD_Write_Chr(LCD_Addr, 'R');
            break;
        case 'Y':
            LCD_Write_Chr(LCD_Addr, 'Y');
            break;
        case 'O':
            LCD_Write_Chr(LCD_Addr, 'O');

            break;
    }
    lastColor = col;
    LCD_Position(LCD_Addr, 17);
}


const uint8_t SEGMENT_MAP[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57};

uint32_t lastVal0;
uint32_t lastVal1;
uint32_t lastVal2;
uint32_t lastVal3;
uint32_t lastVal4;
uint32_t lastTime = 0;
// function to display time on the LCD

void displayTime(uint32_t time) {

    if (time == lastTime) {
        return;
    }
    uint32_t val0 = time / 10000;
    uint32_t val1 = (time - val0 * 10000) / 1000;
    uint32_t val2 = (time - val0 * 10000 - val1 * 1000) / 100;
    uint32_t val3 = (time - val0 * 10000 - val1 * 1000 - val2 * 100) / 10;
    uint32_t val4 = (time - val0 * 10000 - val1 * 1000 - val2 * 100 - val3 * 10) / 1;

    if (val0 != lastVal0) {
        LCD_Position(LCD_Addr, 0x0b);
        LCD_Write_Chr(LCD_Addr, SEGMENT_MAP[val0]);
    }
    if (val1 != lastVal1) {
        LCD_Position(LCD_Addr, 0x0c);
        LCD_Write_Chr(LCD_Addr, SEGMENT_MAP[val1]);
    }
    if (val2 != lastVal2) {
        LCD_Position(LCD_Addr, 0x0d);
        LCD_Write_Chr(LCD_Addr, SEGMENT_MAP[val2]);
    }
    if (val3 != lastVal3) {
        LCD_Position(LCD_Addr, 0x0e);
        LCD_Write_Chr(LCD_Addr, SEGMENT_MAP[val3]);
    }
    if (val4 != lastVal4) {

        LCD_Position(LCD_Addr, 0x0f);
        LCD_Write_Chr(LCD_Addr, SEGMENT_MAP[val4]);
    }

    LCD_Position(LCD_Addr, 17);
    lastTime = time;
    lastVal0 = val0;
    lastVal1 = val1;
    lastVal2 = val2;
    lastVal3 = val3;
    lastVal4 = val4;
}

unsigned long lastTenSecMillis = 0;
// function to wait millis

uint8_t millisWait(uint16_t wait) {
    if (millis - lastTenSecMillis >= wait) {
       // lastTenSecMillis = millis;
        return 1;
    }
    return 0;
}

uint32_t lastPeriodWait = 0;
//function to wait period

uint16_t periodWait(uint16_t wait) {
    if (timePeriod - lastPeriodWait >= wait / 100) {
        lastPeriodWait = timePeriod;
        return 1;
    }
    return 0;
}
// check for hazard button

void checkForHazard() {
    if (hazard == pressed) {
        STATE = HAZARD;
        lights = green;
    }
}

int main(void) {
    char def[] = "OOOOOO";
    //char str1[] = "MAJOR PROJECT!";
    setup(); // set up the physical hardware
    setup_I2C();
    setup_SPI();
    setup_LCD(LCD_Addr);
    LCD_Write(LCD_Addr, def, 6);
    readMCP();

    while (1) {
        buttonRead();
        timeFun();
        displayTime(timePeriod / 10);
        displayState();
        sharpChirp();
        setLights(&trafficLights, &lastTrafficLights);
        checkForHazard();


        switch (STATE) {
            case HAZARD:

                NEXT_STATE = BW_STRAIGHT;
                switch (lights) {
                    case green:
                        hazardBlink();
                        lastTenSecMillis = millis;
                        lights = yellow;
                        break;
                    case yellow:
                        vs0 = released;
                        vs1 = released;
                        vs2 = released;
                        vs3 = released;
                        vs4 = released;
                        vs5 = released;
                        displayStateColor('Y');
                        hazardBlink();
                        if (hazard == pressed) {
                            lastTenSecMillis = millis;
                        }
                        if (millisWait(10000)) {
                            lights = red;
                            lastPeriodWait = timePeriod;
                        }
                        break;
                    case red:
                        blinkState = OFF;
                        trafficLights.BN = 'R';
                        trafficLights.BP = 'R';
                        trafficLights.BS = 'R';
                        trafficLights.BST = 'R';
                        trafficLights.LS = 'R';
                        displayStateColor('R');

                        if (periodWait(3000)) {
                            STATE = NEXT_STATE;
                            lights = green;
                            time = min;
                        }

                        break;
                }

                break;
            case BW_STRAIGHT:
                switch (lights) {
                    case green:
                        displayStateColor('G');
                        vs0 = released;
                        vs3 = released;
                        vs4 = released;
                        trafficLights.BN = 'G';
                        trafficLights.BS = 'G';
                        switch (time) {
                            case min:
                                if (vs1 == pressed || vs2 == pressed || vs5 == pressed) {
                                    if (vs1 == pressed) {
                                        NEXT_STATE = BW_TURN;
                                    } else if (vs2 == pressed) {
                                        NEXT_STATE = LITTLE_ST;
                                    } else {
                                        NEXT_STATE = PEDESTRIAN;
                                    }
                                    if (periodWait(8000)) {
                                        if (s0 == pressed || s3 == pressed || s4 == pressed) {
                                            time = max;
                                        } else {
                                            lights = yellow;
                                        }
                                    }
                                } else {
                                    lastPeriodWait = timePeriod;
                                }

                                break;
                            case max:
                                if (vs1 == pressed) {
                                    NEXT_STATE = BW_TURN;
                                } else if (vs2 == pressed) {
                                    NEXT_STATE = LITTLE_ST;
                                } else {
                                    NEXT_STATE = PEDESTRIAN;
                                }
                                if (periodWait(2000)) {

                                    lights = yellow;

                                }

                                break;
                        }

                        break;
                    case yellow:
                        displayStateColor('Y');
                        switch (NEXT_STATE) {
                            case BW_TURN:
                                trafficLights.BN = 'Y';
                                if (periodWait(3000)) {
                                    lights = red;
                                }

                                break;
                            case LITTLE_ST:
                                trafficLights.BN = 'Y';
                                trafficLights.BS = 'Y';
                                if (periodWait(3000)) {
                                    lights = red;
                                }

                                break;
                            case PEDESTRIAN:
                                trafficLights.BN = 'Y';
                                trafficLights.BS = 'Y';
                                if (periodWait(3000)) {
                                    lights = red;
                                }
                                break;
                            case HAZARD:
                                break;
                            case BW_STRAIGHT:
                                break;
                        }
                        break;
                    case red:
                        displayStateColor('R');
                        switch (NEXT_STATE) {
                            case BW_TURN:
                                trafficLights.BN = 'R';
                                if (periodWait(3000)) {
                                    STATE = NEXT_STATE;
                                    lights = green;
                                    time = min;
                                }
                                break;
                            case LITTLE_ST:
                                trafficLights.BN = 'R';
                                trafficLights.BS = 'R';
                                if (periodWait(3000)) {
                                    STATE = NEXT_STATE;
                                    lights = green;
                                    time = min;
                                }
                                break;
                            case PEDESTRIAN:
                                trafficLights.BN = 'R';
                                trafficLights.BS = 'R';
                                if (periodWait(3000)) {
                                    STATE = NEXT_STATE;
                                    lights = green;
                                    time = min;
                                }
                                break;
                            case HAZARD:
                                break;
                            case BW_STRAIGHT:
                                break;
                        }
                        break;
                }

                break;
            case BW_TURN:
                switch (lights) {
                    case green:
                        displayStateColor('G');
                        trafficLights.BS = 'G';
                        trafficLights.BST = 'G';
                        vs1 = released;
                        switch (time) {
                            case min:
                                if (periodWait(2000)) {
                                    if (s1 == pressed) {
                                        time = max;
                                    } else {
                                        if (vs2 == pressed) {
                                            NEXT_STATE = LITTLE_ST;
                                        } else if (vs5 == pressed) {
                                            NEXT_STATE = PEDESTRIAN;
                                        } else {
                                            NEXT_STATE = BW_STRAIGHT;
                                        }
                                        lights = yellow;
                                    }
                                }
                                break;
                            case max:
                                if (periodWait(2000)) {
                                    if (vs2 == pressed) {
                                        NEXT_STATE = LITTLE_ST;
                                    } else if (vs5 == pressed) {
                                        NEXT_STATE = PEDESTRIAN;
                                    } else {
                                        NEXT_STATE = BW_STRAIGHT;
                                    }
                                    lights = yellow;
                                }
                                break;
                        }

                        break;
                    case yellow:
                        displayStateColor('Y');
                        switch (NEXT_STATE) {
                            case LITTLE_ST:
                                trafficLights.BS = 'Y';
                                trafficLights.BST = 'Y';

                                if (periodWait(3000)) {
                                    lights = red;
                                }

                                break;
                            case PEDESTRIAN:
                                trafficLights.BS = 'Y';
                                trafficLights.BST = 'Y';
                                if (periodWait(3000)) {
                                    lights = red;
                                }
                                break;
                            case BW_STRAIGHT:
                                trafficLights.BST = 'Y';
                                if (periodWait(3000)) {
                                    lights = red;
                                }
                                break;

                            case HAZARD:
                                break;
                            case BW_TURN:
                                break;

                        }
                        break;
                    case red:
                        displayStateColor('R');
                        switch (NEXT_STATE) {
                            case LITTLE_ST:
                                trafficLights.BS = 'R';
                                trafficLights.BST = 'R';
                                if (periodWait(3000)) {
                                    STATE = NEXT_STATE;
                                    lights = green;
                                    time = min;
                                }

                                break;
                            case PEDESTRIAN:
                                trafficLights.BS = 'R';
                                trafficLights.BST = 'R';
                                if (periodWait(3000)) {
                                    STATE = NEXT_STATE;
                                    lights = green;
                                    time = min;
                                }
                                break;
                            case BW_STRAIGHT:
                                trafficLights.BST = 'R';
                                if (periodWait(3000)) {
                                    STATE = NEXT_STATE;
                                    lights = green;
                                    time = min;
                                }
                                break;

                            case HAZARD:
                                break;
                            case BW_TURN:
                                break;
                        }


                        break;
                }
                break;
            case LITTLE_ST:
                switch (lights) {
                    case green:
                        displayStateColor('G');
                        trafficLights.LS = 'G';
                        switch (time) {
                            case min:
                                vs2 = released;
                                if (periodWait(3000)) {
                                    if (s2 == pressed) {
                                        time = max;
                                    } else {
                                        if (vs5 == pressed) {
                                            NEXT_STATE = PEDESTRIAN;
                                        } else {
                                            NEXT_STATE = BW_STRAIGHT;
                                        }

                                        lights = yellow;
                                    }
                                }
                                break;
                            case max:
                                if (periodWait(2000)) {
                                    if (vs5 == pressed) {
                                        NEXT_STATE = PEDESTRIAN;
                                    } else {
                                        NEXT_STATE = BW_STRAIGHT;
                                    }
                                    lights = yellow;
                                }
                                break;
                        }

                        break;
                    case yellow:
                        displayStateColor('Y');
                        switch (NEXT_STATE) {

                            case PEDESTRIAN:
                                trafficLights.LS = 'Y';
                                if (periodWait(3000)) {
                                    lights = red;
                                }
                                break;
                            case BW_STRAIGHT:
                                trafficLights.LS = 'Y';
                                if (periodWait(3000)) {
                                    lights = red;
                                }
                                break;
                            case HAZARD:
                                break;
                            case BW_TURN:
                                break;
                            case LITTLE_ST:
                                break;
                        }
                        break;

                    case red:
                        displayStateColor('R');
                        switch (NEXT_STATE) {

                            case PEDESTRIAN:
                                trafficLights.LS = 'R';
                                if (periodWait(3000)) {
                                    lights = green;
                                    STATE = NEXT_STATE;
                                    time = min;
                                }
                                break;
                            case BW_STRAIGHT:
                                trafficLights.LS = 'R';
                                if (periodWait(3000)) {
                                    lights = green;
                                    STATE = NEXT_STATE;
                                    time = min;
                                }
                                break;
                            case HAZARD:
                                break;
                            case BW_TURN:
                                break;
                            case LITTLE_ST:
                                break;
                        }

                        break;
                }
                break;
            case PEDESTRIAN:
                switch (lights) {
                    case green:
                        displayStateColor('G');
                        vs5 = released;
                        trafficLights.BP = 'G';
                        if (periodWait(7000)) {
                            lights = yellow;
                        }
                        NEXT_STATE = BW_STRAIGHT;
                        pedState = ON;
                        break;
                    case yellow:

                        pedBlink();
                        if (periodWait(3000)) {
                            lights = red;
                        }
                        break;
                    case red:
                        displayStateColor('R');
                        trafficLights.BP = 'R';
                        if (periodWait(3000)) {
                            lights = green;
                            STATE = NEXT_STATE;
                        }
                        break;
                }
                break;
        }
    }
}
