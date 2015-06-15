// Platform-dependent functions

#include "platform.h"
#include "c_stdio.h"
#include "c_string.h"
#include "c_stdlib.h"
#include "gpio.h"
#include "user_interface.h"
#include "driver/uart.h"
// Platform specific includes

void output_redirect(const char *str){

#ifdef DEVELOP_VERSION    
   uart0_sendStr(str);
#endif
}

int platform_init()
{
  // Setup PWMs
  pwms_init();

  cmn_platform_init();
  // All done
  return PLATFORM_OK;
}

// ****************************************************************************
// KEY_LED functions
uint8_t platform_key_led( uint8_t level){
  uint8_t temp;
  gpio16_output_set(1);   // set to high first, for reading key low level
  gpio16_input_conf();
  temp = gpio16_input_get();
  gpio16_output_conf();
  gpio16_output_set(level);
  return temp;
}

// ****************************************************************************
// GPIO functions
int platform_gpio_mode( unsigned pin, unsigned mode, unsigned pull )
{
  // NODE_DBG("Function platform_gpio_mode() is called. pin_mux:%d, func:%d",pin_mux[pin],pin_func[pin]);
  if (pin >= NUM_GPIO)
    return -1;
  if(pin == 0){
    if(mode==PLATFORM_GPIO_INPUT)
      gpio16_input_conf();
    else
      gpio16_output_conf();
    return 1;
  }

  platform_pwm_close(pin);    // closed from pwm module, if it is used in pwm

  switch(pull){
    case PLATFORM_GPIO_PULLUP:
      PIN_PULLDWN_DIS(pin_mux[pin]);
      PIN_PULLUP_EN(pin_mux[pin]);
      break;
    case PLATFORM_GPIO_PULLDOWN:
      PIN_PULLUP_DIS(pin_mux[pin]);
      PIN_PULLDWN_EN(pin_mux[pin]);
      break;
    case PLATFORM_GPIO_FLOAT:
      PIN_PULLUP_DIS(pin_mux[pin]);
      PIN_PULLDWN_DIS(pin_mux[pin]);
      break;
    default:
      PIN_PULLUP_DIS(pin_mux[pin]);
      PIN_PULLDWN_DIS(pin_mux[pin]);
      break;
  }

  switch(mode){
    case PLATFORM_GPIO_INPUT:
#ifdef GPIO_INTERRUPT_ENABLE
      //lua_gpio_unref(pin);    // unref the lua ref call back.
#endif
      GPIO_DIS_OUTPUT(pin_num[pin]);
    case PLATFORM_GPIO_OUTPUT:
      ETS_GPIO_INTR_DISABLE();
#ifdef GPIO_INTERRUPT_ENABLE
      pin_int_type[pin] = GPIO_PIN_INTR_DISABLE;
#endif
      PIN_FUNC_SELECT(pin_mux[pin], pin_func[pin]);
      //disable interrupt
      gpio_pin_intr_state_set(GPIO_ID_PIN(pin_num[pin]), GPIO_PIN_INTR_DISABLE);
      //clear interrupt status
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin_num[pin]));
      GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pin_num[pin])), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin_num[pin]))) & (~ GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE))); //disable open drain; 
      ETS_GPIO_INTR_ENABLE();
      break;
#ifdef GPIO_INTERRUPT_ENABLE
    case PLATFORM_GPIO_INT:
      ETS_GPIO_INTR_DISABLE();
      PIN_FUNC_SELECT(pin_mux[pin], pin_func[pin]);
      GPIO_DIS_OUTPUT(pin_num[pin]);
      gpio_register_set(GPIO_PIN_ADDR(GPIO_ID_PIN(pin_num[pin])), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
                        | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
                        | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));
      ETS_GPIO_INTR_ENABLE();
      break;
#endif
    default:
      break;
  }
  return 1;
}

int platform_gpio_write( unsigned pin, unsigned level )
{
  // NODE_DBG("Function platform_gpio_write() is called. pin:%d, level:%d",GPIO_ID_PIN(pin_num[pin]),level);
  if (pin >= NUM_GPIO)
    return -1;
  if(pin == 0){
    gpio16_output_conf();
    gpio16_output_set(level);
    return 1;
  }

  GPIO_OUTPUT_SET(GPIO_ID_PIN(pin_num[pin]), level);
}

int platform_gpio_read( unsigned pin )
{
  // NODE_DBG("Function platform_gpio_read() is called. pin:%d",GPIO_ID_PIN(pin_num[pin]));
  if (pin >= NUM_GPIO)
    return -1;

  if(pin == 0){
    // gpio16_input_conf();
    return 0x1 & gpio16_input_get();
  }

  // GPIO_DIS_OUTPUT(pin_num[pin]);
  return 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(pin_num[pin]));
}

#ifdef GPIO_INTERRUPT_ENABLE
static void platform_gpio_intr_dispatcher( platform_gpio_intr_handler_fn_t cb){
  uint8 i, level;
  uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  for (i = 0; i < GPIO_PIN_NUM; i++) {
    if (pin_int_type[i] && (gpio_status & BIT(pin_num[i])) ) {
      //disable interrupt
      gpio_pin_intr_state_set(GPIO_ID_PIN(pin_num[i]), GPIO_PIN_INTR_DISABLE);
      //clear interrupt status
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(pin_num[i]));
      level = 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(pin_num[i]));
      if(cb){
        cb(i, level);
      }
      gpio_pin_intr_state_set(GPIO_ID_PIN(pin_num[i]), pin_int_type[i]);
    }
  }
}

void platform_gpio_init( platform_gpio_intr_handler_fn_t cb )
{
  ETS_GPIO_INTR_ATTACH(platform_gpio_intr_dispatcher, cb);
}

int platform_gpio_intr_init( unsigned pin, GPIO_INT_TYPE type )
{
  if (pin >= NUM_GPIO)
    return -1;
  ETS_GPIO_INTR_DISABLE();
  //clear interrupt status
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin_num[pin]));
  pin_int_type[pin] = type;
  //enable interrupt
  gpio_pin_intr_state_set(GPIO_ID_PIN(pin_num[pin]), type);
  ETS_GPIO_INTR_ENABLE();
}
#endif

// ****************************************************************************
// UART
// TODO: Support timeouts.

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;
uint32_t platform_uart_setup( unsigned id, uint32_t baud, int databits, int parity, int stopbits )
{
  switch( baud )
  {
    case BIT_RATE_1200:
    case BIT_RATE_2400:
    case BIT_RATE_4800:
    case BIT_RATE_9600:
    case BIT_RATE_19200:
    case BIT_RATE_38400:
    case BIT_RATE_57600:
    case BIT_RATE_74880:
    case BIT_RATE_115200:
    case BIT_RATE_230400:
    case BIT_RATE_460800:
    case BIT_RATE_921600:
      UartDev.baut_rate = baud;
      break;
    default:
      UartDev.baut_rate = BIT_RATE_9600;
      break;
  }

  switch( databits )
  {
    case 5:
      UartDev.data_bits = FIVE_BITS;
      break;
    case 6:
      UartDev.data_bits = SIX_BITS;
      break;
    case 7:
      UartDev.data_bits = SEVEN_BITS;
      break;
    case 8:
      UartDev.data_bits = EIGHT_BITS;
      break;
    default:
      UartDev.data_bits = EIGHT_BITS;
      break;
  }

  switch (stopbits)
  {
    case PLATFORM_UART_STOPBITS_1:
      UartDev.stop_bits = ONE_STOP_BIT;
      break;
    case PLATFORM_UART_STOPBITS_2:
      UartDev.stop_bits = TWO_STOP_BIT;
      break;
    default:
      UartDev.stop_bits = ONE_STOP_BIT;
      break;
  }

  switch (parity)
  {
    case PLATFORM_UART_PARITY_EVEN:
      UartDev.parity = EVEN_BITS;
      break;
    case PLATFORM_UART_PARITY_ODD:
      UartDev.parity = ODD_BITS;
      break;
    default:
      UartDev.parity = NONE_BITS;
      break;
  }

  uart_setup(id);

  return baud;
}

// Send: version with and without mux
void platform_uart_send( unsigned id, u8 data ) 
{
  uart_tx_one_char(id, data);
}

