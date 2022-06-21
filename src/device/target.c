#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "fsl_pint.h"
#include "fsl_inputmux.h"
#include "fsl_usart.h"

#include "vfs.h"
#include "target.h"

/***************************************************************************
 * external symbols
 ***************************************************************************/
extern FileObject * opened_fds[];	/* device/vfs.c  */
/***************************************************************************
 * test device driver
 ***************************************************************************/
static int dev_init_test(Device *dev);
static int dev_open_test(FileObject *f);
static int dev_close_test(FileObject *f);
static int dev_read_test(FileObject *f, void *buf, size_t len);

Device dev_test={
    .name="test",
    .refcnt=0,
    .sem_read=NULL,
    .sem_write=NULL,
    .init=dev_init_test,
    .open=dev_open_test,
    .close=dev_close_test,
    .read=dev_read_test,
    .write=NULL,
    .ioctl=NULL
};

static int dev_init_test(Device *dev)
{
    dev->mutex=sem_new(1);
    if (dev->mutex) return 1;
    return 0;
}

static int dev_open_test(FileObject *f)
{
	sem_p(f->dev->mutex);
    if (f->flags & (O_READ)) {
        f->dev->refcnt++;
        sem_v(f->dev->mutex);
        return 1;
    }
    sem_v(f->dev->mutex);
    return 0;
}

static int dev_close_test(FileObject *f)
{
	sem_p(f->dev->mutex);
    f->dev->refcnt--;
    sem_v(f->dev->mutex);
    return 1;
}

static int dev_read_test(FileObject *f, void *buf, size_t len)
{
	int n;
	char *file="ceci est un test\r\n";
	sem_p(f->dev->mutex);
	// calculate max available readable data length
	n=f->offset<strlen(file) ? strlen(file)-f->offset : 0;
	// actually, we have to read the min from available length and expected length
	n=n<(int)len ? n : (int)len;
	memcpy(buf, file+f->offset, n);
	f->offset+=n;
	sem_v(f->dev->mutex);
	return n;
}

/***************************************************************************
 * leds device driver
 ***************************************************************************/
static void leds(uint32_t val)
{
	// bit 0 of val controls LED RED
	GPIO_PinWrite(BOARD_LED_RED_GPIO,BOARD_LED_RED_GPIO_PORT,BOARD_LED_RED_GPIO_PIN, (~(val>>0))&1);
	//  bit 1 of val controls LED GREEN
	GPIO_PinWrite(BOARD_LED_GREEN_GPIO,BOARD_LED_GREEN_GPIO_PORT,BOARD_LED_GREEN_GPIO_PIN, (~(val>>1))&1);
	//  bit 2 of val controls LED BLUE
	GPIO_PinWrite(BOARD_LED_BLUE_GPIO,BOARD_LED_BLUE_GPIO_PORT,BOARD_LED_BLUE_GPIO_PIN, (~(val>>2))&1);
}

static int dev_init_leds(Device *dev);
static int dev_open_leds(FileObject *f);
static int dev_close_leds(FileObject *f);
static int dev_write_leds(FileObject *f, void *buf, size_t len);

Device dev_leds={
    .name="leds",
    .refcnt=0,
    .init=dev_init_leds,
    .open=dev_open_leds,
    .close=dev_close_leds,
    .write=dev_write_leds,
    .sem_read=NULL,
    .sem_write=NULL,
    .ioctl=NULL,
    
	/* A COMPLETER */
};

static int dev_init_leds(Device *dev)
{
	// leds init
	gpio_pin_config_t ledcfg = { kGPIO_DigitalOutput, 1};
	GPIO_PortInit(BOARD_LED_RED_GPIO,BOARD_LED_RED_GPIO_PORT);
	GPIO_PinInit(BOARD_LED_RED_GPIO,BOARD_LED_RED_GPIO_PORT,BOARD_LED_RED_GPIO_PIN,&ledcfg);
	GPIO_PinInit(BOARD_LED_GREEN_GPIO,BOARD_LED_GREEN_GPIO_PORT,BOARD_LED_GREEN_GPIO_PIN,&ledcfg);
	GPIO_PinInit(BOARD_LED_BLUE_GPIO,BOARD_LED_BLUE_GPIO_PORT,BOARD_LED_BLUE_GPIO_PIN,&ledcfg);

	leds(0);

	/* A COMPLETER */
	dev->mutex = sem_new(1);//create r/w mutex

	return 1;
}

static int dev_open_leds(FileObject *f)
{
	sem_p(f->dev->mutex);
    if (f->dev->refcnt || (f->flags & (O_READ|O_NONBLOCK|O_APPEND|O_SHLOCK|O_EXLOCK|O_ASYNC|O_SYNC|O_CREAT|O_TRUNC|O_EXCL)))
        goto err;
    if (f->flags & O_WRITE) {
        f->dev->refcnt++;
        sem_v(f->dev->mutex);
        return 1;
    }
err:
	sem_v(f->dev->mutex);
    return 0;
}

static int dev_close_leds(FileObject *f)
{
	sem_p(f->dev->mutex);
    f->dev->refcnt--;
    sem_v(f->dev->mutex);
    return 1;
}

static int dev_write_leds(FileObject *f, void *buf, size_t len)
{
    sem_p(f->dev->mutex);//enter critical space

	leds(*(uint32_t*)buf);

	sem_v(f->dev->mutex);//leave critical space


    return 1;
}

/***************************************************************************
 * SWCenter External Interrupt Button device driver
 ***************************************************************************/
static int dev_init_btn(Device *dev);
static int dev_open_btn(FileObject *f);
static int dev_close_btn(FileObject *f);
static int dev_read_btn(FileObject *f, void *buf, size_t len);

Device dev_swuser={
    .name="swuser",
    .refcnt=0,
    .init=dev_init_btn,
    .open=dev_open_btn,
    .close=dev_close_btn,
    .read=dev_read_btn,
	.write=NULL,
    .sem_read=NULL,
    .sem_write=NULL,
    .ioctl=NULL,
	/* A COMPLETER */
};

/*
 *  ISR callback (IRQ mode !!)
 */
static void on_swuser_cb(pint_pin_int_t pintr, uint32_t pmatch_status)
{
	/* A COMPLETER */
	sem_v(dev_swuser.sem_read);//free a button to be used again ! (will unlock dev_read_btn)
}

static int dev_init_btn(Device *dev)
{
    /* Connect trigger sources to PINT */
    INPUTMUX_Init(INPUTMUX);
    INPUTMUX_AttachSignal(INPUTMUX, kPINT_PinInt0, kINPUTMUX_GpioPort1Pin9ToPintsel);
    /* Turnoff clock to inputmux to save power. Clock is only needed to make changes */
    INPUTMUX_Deinit(INPUTMUX);
    /* Initialize PINT */
    PINT_Init(PINT);
    /* Setup Pin Interrupt 0 for rising edge */
    PINT_PinInterruptConfig(PINT, kPINT_PinInt0, kPINT_PinIntEnableRiseEdge, on_swuser_cb);
    NVIC_SetPriority(PIN_INT0_IRQn,10);
	/* Enable callbacks for PINT0 by Index */
    PINT_EnableCallbackByIndex(PINT, kPINT_PinInt0);

	/* A COMPLETER */
    dev->sem_read = sem_new(0);//init read sem
    dev->mutex = sem_new(1);//init lock


    return 1;
}

static int dev_open_btn(FileObject *f)
{
	sem_p(f->dev->mutex);
    if (f->dev->refcnt || (f->flags & (O_WRITE|O_NONBLOCK|O_APPEND|O_SHLOCK|O_EXLOCK|O_ASYNC|O_SYNC|O_CREAT|O_TRUNC|O_EXCL)))
        goto err;
    if (f->flags & O_READ) {
        f->dev->refcnt++;
		sem_v(f->dev->mutex);
        return 1;
    }
err:
	sem_v(f->dev->mutex);
    return 0;
}

static int dev_close_btn(FileObject *f)
{
	sem_p(f->dev->mutex);
    f->dev->refcnt--;
	sem_v(f->dev->mutex);
    return 1;
}

static int dev_read_btn(FileObject *f, void *buf, size_t len)
{
	/* A COMPLETER */
	sem_p(f->dev->sem_read);
    *(uint32_t*)buf =1;
    return 4;
}




#define RING_BUF_SIZE	128

typedef volatile struct RingBuffer {
	char buf[RING_BUF_SIZE];
	int  i_w;		/* pointeur (index) d'écriture */
	int  i_r;		/* pointeur (index) de lecture */
} RingBuffer;

RingBuffer serialBuf = {
	.i_w=0,
	.i_r=0
};

static int dev_init_serial(Device *dev);
static int dev_open_serial(FileObject *f);
static int dev_close_serial(FileObject *f);
static int dev_write_serial(FileObject *f, void *buf, size_t len);
static int dev_read_serial(FileObject *f, void *buf, size_t len);

Device dev_serial={
    .name="serial",
    .refcnt=0,
    .init=dev_init_serial,
    .open=dev_open_serial,
    .close=dev_close_serial,
    .write=dev_write_serial,
	.read=dev_read_serial,
    .sem_read=NULL,
    .sem_write=NULL,
    .ioctl=NULL,

	/* A COMPLETER */
};
void FLEXCOMM0_IRQHandler()
{
    if ((USART0->FIFOSTAT & USART_FIFOSTAT_RXERR_MASK) != 0U) {
        /* Clear rx error state. */
        USART0->FIFOSTAT |= USART_FIFOSTAT_RXERR_MASK;
        /* clear rxFIFO */
        USART0->FIFOCFG |= USART_FIFOCFG_EMPTYRX_MASK;
    } else if (((serialBuf.i_w+1) % RING_BUF_SIZE)!=serialBuf.i_r) {
    	serialBuf.buf[serialBuf.i_w]=USART_ReadByte(USART0);
    	serialBuf.i_w = (serialBuf.i_w+1) % RING_BUF_SIZE;
    } else USART_ReadByte(USART0);
    sem_v(dev_serial.sem_read);
}
static int dev_init_serial(Device *dev)
{
	usart_config_t config;
	USART_GetDefaultConfig(&config);
	config.baudRate_Bps = BOARD_DEBUG_UART_BAUDRATE;
	config.enableTx     = true;
	config.enableRx     = true;
	USART_Init(USART0, &config, CLOCK_GetFlexCommClkFreq(0U));
	USART_EnableInterrupts(USART0, kUSART_RxLevelInterruptEnable | kUSART_RxErrorInterruptEnable);
	EnableIRQ(FLEXCOMM0_IRQn);

	dev->mutex =sem_new(1);//create r/w mutex
	dev->sem_read =  sem_new(0);
	dev->sem_write =  sem_new(1);
//	sem_p(dev->sem_read);
	return 1;
}

static int dev_open_serial(FileObject *f)
{
	sem_p(f->dev->mutex);
    if (f->dev->refcnt || (f->flags & (O_NONBLOCK|O_APPEND|O_SHLOCK|O_EXLOCK|O_ASYNC|O_SYNC|O_CREAT|O_TRUNC|O_EXCL)))
        goto err;
    if (f->flags & (O_WRITE|O_READ)) {
        f->dev->refcnt++;
        sem_v(f->dev->mutex);
        return 1;
    }
err:
	sem_v(f->dev->mutex);
    return 0;
}

static int dev_close_serial(FileObject *f)
{
	sem_p(f->dev->mutex);
    f->dev->refcnt--;
    sem_v(f->dev->mutex);
    return 1;
}

static int dev_write_serial(FileObject *f, void *buf, size_t len)
{
    sem_p(f->dev->mutex);//enter critical space

//	leds(*(uint32_t*)buf);
//	USART_WriteBlocking(USART0, buf, sizeof(txbuff) - 1);c
	USART_WriteBlocking(USART0, buf, len);
	sem_v(f->dev->mutex);//leave critical space


    return 1;
}
static int dev_read_serial(FileObject *f, void *buf, size_t len){
	sem_p(f->dev->sem_read);
	char c;
	uint32_t interruptMask = 0U;


	//On bloque les interruptions pendant la lecture du caractère sur le ring buffer
	interruptMask = USART_GetEnabledInterrupts(USART0);
	USART_DisableInterrupts(USART0, interruptMask);
	c=serialBuf.buf[serialBuf.i_r];
	serialBuf.i_r = (serialBuf.i_r+1) % RING_BUF_SIZE;
	//Réactive les interruptions
	USART_EnableInterrupts(USART0, interruptMask);
	*(char*)buf=c;
	return 0;
}
/***************************************************************************
 * Device table
 ***************************************************************************/
Device * device_table[]={
	&dev_test,
	&dev_leds,
    &dev_swuser,
	&dev_serial
};

/*****************************************************************************
 * Target Init
 *****************************************************************************/
extern Semaphore* vfs_mutex;

void dev_init()
{
    int i=0;
    Device *dev=device_table[0];
    while (dev) {
        if (dev->init) dev->init(dev);
        dev=device_table[++i];
    }
    
    for (int i=0;i<MAX_OPENED_FDS;i++)
        opened_fds[i]=NULL;

    vfs_mutex=sem_new(1);
}

