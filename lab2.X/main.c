#include <xc.h>
#pragma config FOSC = HS // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF // Watchdog Timer Enable bit
#pragma config PWRTE = OFF // Power-up Timer Enable bit
#pragma config BOREN = OFF // Brown-out Reset Enable bit
#pragma config LVP = OFF // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit
#pragma config CPD = OFF // Data EEPROM Memory Code Protection bit
#pragma config WRT = OFF // Flash Program Memory Write Enable bits
#pragma config CP = OFF // Flash Program Memory Code Protection bit

#include <string.h>
#include <stdio.h>
#include <conio.h>

void setup_USART() {
    RCSTA = 0;
    RCSTAbits.CREN = 1;
    RCSTAbits.SPEN = 1;
    TXSTA = 0;
    TXSTAbits.TXEN = 1; //stes bit TXIF as well
    TXSTAbits.BRGH = 1;
    SPBRG = 25;
    /* RCIE = 1;
     INTCONbits.GIE = 1;
     INTCONbits.PEIE = 1; */

}

void enviar_caracter(char c) {
    TXREG = c; // Read/Write transmit buffer register
    while (TXIF == 0); // transmit interrupt flag bits
}

void enviar(char msg[]) {
    int i = 0;
    while (msg[i] != '\0') {
        enviar_caracter(msg[i]);
        i++;
    }
}

void botao(int *mode, int *lastValidButtonState) {
    if (PORTBbits.RB3 != *lastValidButtonState) {
        *lastValidButtonState = PORTBbits.RB3;
        if (!PORTBbits.RB3) {
            if (*mode) {
                *mode = 0; //modo normal
            } else {
                *mode = 1; //modo standby
            }
        }
    }
}

int debouncer(int *lastButtonState, int *t) {

    if (*lastButtonState == PORTBbits.RB3) {
        *t = *t + 1;
    } else {
        *t = 0;
    }
    *lastButtonState = PORTBbits.RB3;

    if (*t == 10) {
        *t = 0;
        return 1;
    }
    return 0;

}

char receber_caracter() {
    char carac;
    while (RCIF == 0); //receive interrupt flag bits, 1 se tiver cheio
    carac = RCREG; //tem a mensagem
    return carac;
}

void receber_msg(char output[]) {
    unsigned int i = 0;
    int x = 1;
    while (x) {
        output[i] = receber_caracter();
        if (output[i] == '\n') {
            x = 0;
            output[i + 1] = '\0';
        }
        i++;
    }
}

void motorRight() {
    PORTCbits.RC5 = 1; //liga resistencia de aquecimento
    PORTDbits.RD2 = 1;
    PORTDbits.RD3 = 0;
}

void motorLeft() {
    PORTCbits.RC5 = 1; //liga resistencia de aquecimento
    PORTDbits.RD2 = 0;
    PORTDbits.RD3 = 1;
}

void motorStop() {
    PORTCbits.RC5 = 0; //desliga resistencia de aquecimento
    PORTDbits.RD2 = 0;
    PORTDbits.RD3 = 0;
}

void sendMotorState(int motorState, int monitorizacao) {
    if (monitorizacao) {
        switch (motorState) {
            case(0):enviar("<mot>\n\tStopped\n</mot>\n");
                break;
            case(1):enviar("<mot>\n\tRight\n</mot>\n");
                break;
            case(2):enviar("<mot>\n\tLeft\n</mot>\n");
                break;
        }
    }
}

void normalMode(int left, int right, int night, int limit, int* motorState, int monitorizacao) {
    if (left < night && right < night) {
        motorStop();
        if (*motorState != 0) {
            *motorState = 0;
            sendMotorState(*motorState, monitorizacao);
        }
    } else if ((right - left) > limit) {
        motorRight();
        if (*motorState != 1) {
            *motorState = 1;
            sendMotorState(*motorState, monitorizacao);
        }
    } else if ((left - right) > limit) {
        motorLeft();
        if (*motorState != 2) {
            *motorState = 2;
            sendMotorState(*motorState, monitorizacao);
        }
    } else {
        motorStop();
        if (*motorState != 0) {
            *motorState = 0;
            sendMotorState(*motorState, monitorizacao);
        }
    }
}

void standbyMode(int* motorState, int monitorizacao) {
    motorStop();
    if (*motorState != 0) {
        *motorState = 0;
        sendMotorState(*motorState, monitorizacao);
    }
}

int stringToInt(char string[]) {
    int i, result = 0;
    for (i = 0; i < (strlen(string) - 1); i++) {
        result = 10 * result + (string[i] - '0');
    }

    return result;
}

void mostraSensores(int valLeft, int valRight) {
    char valString[20];

    sprintf(valString, "<val>\n\tLeft:%d Right:%d\n</val>\n", valLeft, valRight);
    enviar(valString);
}

void enviarAck() {
    enviar("<ack>\n</ack>\n");
}

void mostraTemp(int valTemp) {
    char valString[20];

    sprintf(valString, "<val>\n\tTemp:%d\n</val>\n", valTemp);
    enviar(valString);
}

int sensor(int chanel) {
    ADCON1 = 0x80;

    switch (chanel) {
        case 0: ADCON0 = 0x81;
            break;
        case 1: ADCON0 = 0x89;
            break;
        case 2: ADCON0 = 0x91;
            break;
        case 3: ADCON0 = 0x99;
            break;
        case 4: ADCON0 = 0xA1;
            break;
        case 5: ADCON0 = 0xA9;
            break;
        case 6: ADCON0 = 0xB1;
            break;
        case 7: ADCON0 = 0xB9;
            break;
    }

    ADCON0bits.GO = 1;
    while (GO); //wait until conversion is finished
    return ((ADRESH << 8) + ADRESL);
}

void checkXML(char msg[], int* limit, int* night, int *mode, int *monitorizacao, int *temperature, int *valTemp, int *op) {
    int teste;
    if (!*op) {
        if (strstr(msg, "<lim>") != NULL) {
            *op = 1;
        } else if (strstr(msg, "<min>") != NULL) {
            *op = 2;
        } else if (strstr(msg, "<mod>") != NULL) {
            *op = 3;
        } else if (strstr(msg, "<mon>") != NULL) {
            *op = 4;
        } else if (strstr(msg, "<sen>") != NULL) {
            *op = 5;
        } else if (strstr(msg, "<tmp>") != NULL) {
            *op = 6;
        } else if (strstr(msg, "<mxt>") != NULL) {
            *op = 7;
        } else {
            *op = 0;
            enviar("<wrn>\n\tError 1\n</wrn>\n");
        }
    } else {
        if (strstr(msg, "</") != NULL) {
            switch (*op) {
                case(1): *limit = *valTemp;
                    *op = 0;
                    enviarAck();
                    break;
                case(2): *night = *valTemp;
                    *op = 0;
                    enviarAck();
                    break;
                case(3): teste = *valTemp;
                    if (teste == 1 || teste == 0) {
                        *mode = teste;
                        enviarAck();
                    } else {
                        enviar("<wrn>\n\tError 2\n</wrn>\n");
                    }
                    *op = 0;
                    break;
                case(4): teste = *valTemp;
                    if (teste == 1 || teste == 0) {
                        *monitorizacao = teste;
                        enviarAck();
                    } else {
                        enviar("<wrn>\n\tError 2\n</wrn>\n");
                    }
                    *op = 0;
                    break;
                case(5): *op = 0;
                    mostraSensores(sensor(0), sensor(1));
                    break;
                case(6): mostraTemp(sensor(2));
                    *op = 0;
                    break;
                case(7):*temperature = *valTemp;
                    *op = 0;
                    break;
                default: enviar("<wrn>\n\tError 1\n</wrn>\n");
            }
        } else {
            *valTemp = stringToInt(msg);
        }
    }
}

int main(void) {
    TRISBbits.TRISB3 = 1; //botao
    TRISDbits.TRISD0 = 0; //led standby
    TRISDbits.TRISD1 = 0; //led normal
    TRISDbits.TRISD2 = 0; //led right
    TRISDbits.TRISD3 = 0; //led left
    TRISC = 0; //digital ports as output

    char msg[40] = "\0";
    int lastButtonState = 1, lastValidButtonState = 1;
    int motorState = 0; //0-stop, 1-right 2-left
    int t = 0;
    int night = 200;
    int limit = 200;
    int monitorizacao = 1;
    int temperature = 200;
    int mode = 0; //0-normal, 1-standby
    int op = 0; //0-nada 1-lim 2-min 3-mod 4-mon 5-tmp 6-sen 
    int valTemp = 0;
    setup_USART();


    while (1) {

        //  sensorTemp();

        if (RCIF) {
            receber_msg(msg);
            checkXML(msg, &limit, &night, &mode, &monitorizacao, &temperature, &valTemp, &op);
        }

        if (temperature < sensor(2)) {
            mode = 1; //modo standby
        } else {
            if (debouncer(&lastButtonState, &t)) {
                botao(&mode, &lastValidButtonState);
            }
        }



        if (mode) {
            //modo standby
            PORTDbits.RD0 = 0;
            PORTDbits.RD1 = 1;
            standbyMode(&motorState, monitorizacao);
        } else {
            //modo normal
            PORTDbits.RD0 = 1;
            PORTDbits.RD1 = 0;
            normalMode(sensor(0), sensor(1), night, limit, &motorState, monitorizacao);

        }
    }
}
