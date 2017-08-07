#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmsis_os.h"
#include "Driver_USART.h"


#define DIGIT 2
#define BUFSIZE ((DIGIT * 2) + 2)

#define COMPARE(x) ((47 < x && x > 58) || (64 < x || x > 71))


extern ARM_DRIVER_USART Driver_USART0;   // standart olarak(CMSIS için) tanimlanmasi gereken uart struct degiskeni


osThreadId tid_calc;

void th_calc(void const *args);

osThreadDef(th_calc, osPriorityNormal, 1, 0);

/**
 * Bu fonksiyon https://stackoverflow.com/a/39052987 andresinden alindi ve projeye uyarlandi.
 * hex2int
 * take a hex string and convert it to a 32bit number (max 8 hex digits)
 */
uint32_t hex2int(char *hex)
{
    uint32_t val = 0;
    for (int i = 0; i < DIGIT; i++) {
        // get current character then increment
        uint8_t byte = hex[i];
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9')
            byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f')
            byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F')
            byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

char buff[BUFSIZE];
int count;

void th_calc(const void *args)
{
    int rsltsz = 2 * DIGIT + 7; // uart'tan gönderilecek sonuc mesajinin uzunlugu
    char tmp1[DIGIT], tmp2[DIGIT], result[rsltsz];
    int num1, num2, sum;

    while (1) {
        osSignalWait(0x01, osWaitForever); // sinyal gelmesini bekle

        num1 = 0;
        num2 = 0;
        sum = 0;
        buff[BUFSIZE - 1] = '\0';

        // birinci sayi parse
        strncpy(tmp1, buff, DIGIT);
        num1 = hex2int(tmp1);

        // ikinci sayi parse
        strncpy(tmp2, &buff[DIGIT + 1], DIGIT);
        num2 = hex2int(tmp2);

        // hesaplama islemi
        switch (buff[DIGIT]) {
            case '+':
                sum = num1 + num2;
                break;
            case '-':
                sum = num1 - num2;
                break;
            case '/':
                sum = num1 / num2;
                break;
            case '*':
                sum = num1 * num2;
                break;
            default:
                Driver_USART0.Send("*ERROR: wrong operator\0", 23); // hata mesaji ver
                osDelay(500);
                Driver_USART0.Receive(&buff, BUFSIZE);  // seri portu dinlemeye basla
                continue;
        }

        sprintf(result, "*result= %X\n", sum); // sonucu result'a hex olarak koy
        Driver_USART0.Send(result, rsltsz);   // sonucu uart'tan yolla
        osDelay(500);

        Driver_USART0.Receive(&buff, BUFSIZE); // seri portu dinlemeye basla
    }
}

// callback fonksiyonu uart interrupt'indan çikarken isletilir.
void PCUSART_callback(uint32_t event)
{
    // callback critical section'dir ve lock vardir.
    switch (event) {
        case ARM_USART_EVENT_RECEIVE_COMPLETE:
            // buraya gelindiyse Driver_USART0.Receive(&buff, BUFSIZE) fonsiyonu tamamlanmistir,
            // uarttan data almak için fonksiyonun yeniden çagirilmasi gerekir

            osSignalSet(tid_calc, 0x01); // hesaplama threadini uyandir
            break;
    }
}

void SetupUSART()
{
    Driver_USART0.Initialize(PCUSART_callback);  // interrupt dönmeden önce calistirilacak olan fonksiyon
    Driver_USART0.PowerControl(ARM_POWER_FULL);  // uart guc ayarlamasi

    Driver_USART0.Control(ARM_USART_MODE_ASYNCHRONOUS    // asenkron haberlesme yapilacak
                          | ARM_USART_FLOW_CONTROL_NONE  // hardware seviyesinde akis kontrolü yok
                          | ARM_USART_DATA_BITS_8         // 8 bitlik data transferi yapilacak
                          | ARM_USART_PARITY_NONE                 // parity yok
                          | ARM_USART_STOP_BITS_1, 115200); // baudrate 115200 bit/sec

    Driver_USART0.Control(ARM_USART_CONTROL_TX, 1); // uart tx enable
    Driver_USART0.Control(ARM_USART_CONTROL_RX, 1); // uart rx enable

    Driver_USART0.Receive(&buff, BUFSIZE); // BUFFSIZE kadar bayt gelince interrupt üretecek
    osDelay(50);
}

const char mymessage[] = " *Basic Serial Port Calculator v1\n\0";

int main(void)
{
    SetupUSART();  // uart hazirlaniyor.

    Driver_USART0.Send(mymessage, strlen(mymessage)); // uart'tan veri gönderme fonksiyonu

    tid_calc = osThreadCreate(osThread(th_calc), NULL); // hesaplamayi yapacak thread olusuturuluyor.

    while (1)
        osDelay(100);
}
