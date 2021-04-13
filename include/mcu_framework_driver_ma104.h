/* *****************************************************************************************
 *    File Name   :mcu_framework_driver_ma104.h
 *    Create Date :2021-04-13
 *    Modufy Date :
 *    Information :
 */
 
#ifndef mcu_framework_driver_ma104_H_
#define mcu_framework_driver_ma104_H_

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

#include <stdbool.h>
#include <stdint.h>

#include "fw_usart.h"
#include "fw_io_pin.h"

/* *****************************************************************************************
 *    Type define list
 */ 
typedef struct _mcu_framework_driver_ma104_handle_memory_t{ 
  fw_usart_handle_t*  fw_usart;
	fw_io_pin_handle_t* fw_pin_usbok;
	fw_io_pin_handle_t* fw_pin_reset;
	fw_memory_t usart_txd_memory;
	fw_memory_t usart_rxd_memory;
	fw_memory_t receiverBuffer;
	uint32_t flag;
	uint8_t xferCache[10];
}mcu_framework_driver_ma104_handle_memory_t;
/* *****************************************************************************************
 *    Function Type
 */ 


/* *****************************************************************************************
 *    API List
 */ 

#ifdef __cplusplus
}
#endif //__cplusplus
#endif
/* *****************************************************************************************
 *    End of file
 */ 
