#include <p24fxxxx.h>
#include <stdio.h>
#include <string.h>
// Configuration Bits
#ifdef __PIC24FJ64GA004__ //Defined by MPLAB when using 24FJ64GA004 device
_CONFIG1(JTAGEN_OFF& GCP_OFF& GWRP_OFF& COE_OFF& FWDTEN_OFF& ICS_PGx1& IOL1WAY_ON)
_CONFIG2(FCKSM_CSDCMD& OSCIOFNC_OFF& POSCMOD_HS& FNOSC_PRI& I2C1SEL_SEC)
#else
_CONFIG1(JTAGEN_OFF& GCP_OFF& GWRP_OFF& COE_OFF& FWDTEN_OFF& ICS_PGx2)
_CONFIG2(FCKSM_CSDCMD& OSCIOFNC_OFF& POSCMOD_HS& FNOSC_PRI)
#endif

void delay() {
    int i = 0;
    for (i = 0; i < 20000; i++) {};

}

void enviar_caracter(char c) {
    while (U2STAbits.UTXBF);
    U2TXREG = c;
}

void enviar_msg(char msg[]) {
    int i = 0;
    while (msg[i] != '\0') {
        enviar_caracter(msg[i]);
        i++;
    }
}

char receber_caracter() {
    char carac;
    while (U2STAbits.URXDA == 0); //receive interrupt flag bits, 1 se tiver cheio
    carac = U2RXREG; //tem a mensagem
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


void botao(int* mode, int* lastButtonState) {
    if (PORTDbits.RD6 != *lastButtonState) {
        *lastButtonState = PORTDbits.RD6;
        if (!PORTDbits.RD6) {
            if (*mode) {
                *mode = 0; //modo normal
            }
            else {
                *mode = 1; //modo standby
            }
        }
    }
}

int debouncer(int* lastButtonState, int* t) {

    if (*lastButtonState == PORTDbits.RD6) {
        *t = *t + 1;
    }
    else {
        *t = 0;
    }
    *lastButtonState = PORTDbits.RD6;

    if (*t == 3) {
        *t = 0;
        return 1;
    }
    return 0;

}

void motorRight() {
    LATAbits.LATA0 = 1;
    LATAbits.LATA1 = 0;
}

void motorLeft() {
    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 1;
}

void motorStop() {

    LATAbits.LATA0 = 0;
    LATAbits.LATA1 = 0;
}

void sendMotorState(int motorState, int monitorizacao) {
    if (monitorizacao) {
        switch (motorState) {
        case(0):enviar_msg("<mot>Stopped<mot\\>\n");
            break;
        case(1):enviar_msg("<mot>Right<mot\\>\n");
            break;
        case(2):enviar_msg("<mot>Left<mot\\>\n");
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
    }
    else if ((right - left) > limit) {
        motorRight();
        if (*motorState != 1) {
            *motorState = 1;
            sendMotorState(*motorState, monitorizacao);
        }
    }
    else if ((left - right) > limit) {
        motorLeft();
        if (*motorState != 2) {
            *motorState = 2;
            sendMotorState(*motorState, monitorizacao);
        }
    }
    else {
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
    char valString[40];

    sprintf(valString, "<val>\n\tLeft:%d Right:%d\n</val>\n", valLeft, valRight);
    enviar_msg(valString);
}

void enviarAck() {
    enviar_msg("<ack>\n</ack>\n");
}

void mostraTemp(int valTemp) {
    char valString[40];

    sprintf(valString, "<val>\n\tTemp:%d\n</val>\n", valTemp);
    enviar_msg(valString);
}


int sensorValue(int channel) {
    AD1CHS = channel;


    AD1CON1bits.SAMP = 1;
    delay();
    AD1CON1bits.SAMP = 0;

    while (!AD1CON1bits.DONE) {}
    return ADC1BUF0;
}



void checkXML(char msg[], int* limit, int* night, int* mode, int* monitorizacao, int* temperature, int* valTemp, int* op) {
    int teste;
    if (!*op) {
        if (strstr(msg, "<lim>") != NULL) {
            *op = 1;
        }
        else if (strstr(msg, "<min>") != NULL) {
            *op = 2;
        }
        else if (strstr(msg, "<mod>") != NULL) {
            *op = 3;
        }
        else if (strstr(msg, "<mon>") != NULL) {
            *op = 4;
        }
        else if (strstr(msg, "<sen>") != NULL) {
            *op = 5;
        }
        else if (strstr(msg, "<tmp>") != NULL) {
            *op = 6;
        }
        else if (strstr(msg, "<mxt>") != NULL) {
            *op = 7;
        }
        else {
            *op = 0;
            enviar_msg("<wrn>\n\tError 1\n</wrn>\n");
        }
    }
    else {
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
                }
                else {
                    enviar_msg("<wrn>\n\tError 2\n</wrn>\n");
                }
                *op = 0;
                break;
            case(4): teste = *valTemp;
                if (teste == 1 || teste == 0) {
                    *monitorizacao = teste;
                    enviarAck();
                }
                else {
                    enviar_msg("<wrn>\n\tError 2\n</wrn>\n");
                }
                *op = 0;
                break;
            case(5): *op = 0;
                mostraSensores(sensorValue(0x2), sensorValue(0x3));
                break;
            case(6): mostraTemp(sensorValue(0x5));
                *op = 0;
                break;
            case(7):*temperature = *valTemp;
                *op = 0;
                break;
            default: enviar_msg("<wrn>\n\tError 1\n</wrn>\n");
            }
        }
        else {
            *valTemp = stringToInt(msg);
        }
    }
}

void initializeADC() {
    AD1PCFG = 0xFFD3; //AN2,AN3 and AN5 analogic
    AD1CON1 = 0x0000;
    AD1CSSL = 0;
    AD1CON2 = 0;
    AD1CON3 = 0x0002;
    AD1CON1bits.ADON = 1;  //turn ADC on
}

void initializeUART() {
    U2BRG = 25; //4MHz
    U2STA = 0;
    U2MODE = 0x8000;
    U2STAbits.UTXEN = 1; //Enable Transmit
}

void configurationI2C() {
    I2C2CON = 0b1000000000000000; //enable the I2C2 module and configures the SDA2 and SCL2 pin as serial port pins
    I2C2BRG = 0x27; //FCy=4 MHz, Fscl-100KHz  pag 133
}

void idle() {
    while (I2C2STATbits.TRSTAT || I2C2CONbits.SEN || I2C2CONbits.RSEN || I2C2CONbits.PEN || I2C2CONbits.RCEN || I2C2CONbits.ACKEN);
}

int requestI2C() {
    int received = 2;

    I2C2CONbits.SEN = 1; //começa um evento - indica que vai transmitir - o daniel ACHA
    idle();

    I2C2TRN = 0b00010100; //mensagem a enviar - quer escrever no slave arduino (1010)
    idle();

    I2C2TRN = 0x07; //enviar 7
    idle();

    I2C2CONbits.PEN = 1; // stop event
    idle();

    I2C2CONbits.SEN = 1;
    idle();

    I2C2TRN = 0b00010101; //para ler do slave
    idle();

    I2C2CONbits.RCEN = 1; //starts a master reception

    while (!I2C2STATbits.RBF); //receive completed
    received = I2C2RCV; //GUARDA DADOS	

    I2C2CONbits.ACKEN = 1;
    idle();
    I2C2CONbits.PEN = 1;
    idle();

    return received;
}



int main(void)
{

    initializeUART();
    initializeADC();
    configurationI2C();
    TRISDbits.TRISD6 = 1; //botao
    TRISAbits.TRISA0 = 0; // LED right
    TRISAbits.TRISA1 = 0; // LED left
    TRISAbits.TRISA4 = 0; // normal
    TRISAbits.TRISA5 = 0; // stanby

    char msg[40] = "\0";
    int lastButtonState = 1, lastValidButtonState = 1;
    int t = 0;
    int motorState = 0; //0-stop, 1-right, 2-left

    int night = 100, limit = 200, monitorizacao = 1, temperature = 800;
    int mode = 0, lastMode = 0; //0-normal, 1-standby
    int op = 0; //0-nada 1-lim 2-min 3-mod 4-mon 5-tmp 6-sen 
    int valTemp = 0;

    //AN2-left AN3-right AN5-temperature	

    while (1) {

        if (U2STAbits.URXDA) { //se existe caracter para receber
            receber_msg(msg);
            checkXML(msg, &limit, &night, &mode, &monitorizacao, &temperature, &valTemp, &op);
        }
        if (temperature < sensorValue(0x5)) {
            mode = 1;
        }
        else {
            if (debouncer(&lastButtonState, &t)) {
                botao(&mode, &lastValidButtonState);
            }
        }


        if (mode) {
            if (!lastMode) {
                lastMode = 1;
            }


            //standby
            LATAbits.LATA4 = 0;
            LATAbits.LATA5 = 1;
            standbyMode(&motorState, monitorizacao);

        }
        else {
            //normal
            if (lastMode) {
                lastMode = 0;
                limit = requestI2C();
            }

            char valString[40];

            sprintf(valString, "\n%d\n", limit);
            enviar_msg(valString);

            LATAbits.LATA4 = 1;
            LATAbits.LATA5 = 0;
            normalMode(sensorValue(0x2), sensorValue(0x3), night, limit, &motorState, monitorizacao);
        }


    }

}




