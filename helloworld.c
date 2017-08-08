#include "cmsis_os.h"
#include "Driver_USART.h"

extern ARM_DRIVER_USART Driver_USART0;   // standart olarak(CMSIS için) tanimlanmasi gereken uart degiskeni

void SetupUSART()
{
	Driver_USART0.Initialize(NULL);  // UART'ın ilk değerlerini veriyor
    Driver_USART0.PowerControl(ARM_POWER_FULL);  // uart guc ayarlamasi

    Driver_USART0.Control(ARM_USART_MODE_ASYNCHRONOUS       // asenkron haberlesme yapilacak
                          | ARM_USART_FLOW_CONTROL_NONE     // hardware seviyesinde akis kontrolü yok
                          | ARM_USART_DATA_BITS_8           // 8 bitlik data transferi yapilacak
                          | ARM_USART_PARITY_NONE           // parity yok
                          | ARM_USART_STOP_BITS_1, 115200); // baudrate 115200 bit/sec

    Driver_USART0.Control(ARM_USART_CONTROL_TX, 1); // uart tx enable
    Driver_USART0.Control(ARM_USART_CONTROL_RX, 1); // uart rx enable

    osDelay(50);
}

int main(void)
{
	SetupUSART();  // uart hazirlaniyor.

	Driver_USART0.Send(" Hello world!\0", 14); // uart'tan veri gönderme fonksiyonu

	// uart 25mHz'de çalistigindan sonuçlari görebilmek için delay veriyoruz.
	osDelay(500);
}
