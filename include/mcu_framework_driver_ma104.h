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
#include "tool_ring_buffer.h"

/* *****************************************************************************************
 *    Type define list
 */ 
typedef struct _mcu_framework_driver_ma104_handle_memory_t mcu_framework_driver_ma104_handle_memory_t;
 
typedef void(*mcu_framework_driver_ma104_execute_t)(mcu_framework_driver_ma104_handle_memory_t* _this, fw_memory_t* data, void* attachment);
 
typedef struct _mcu_framework_driver_ma104_handle_memory_t{ 
  fw_usart_handle_t*  fw_usart;
	fw_io_pin_handle_t* fw_pin_usbok;
	fw_io_pin_handle_t* fw_pin_reset;
	tool_ring_buffer_t ringBuffer;
	uint32_t flag;
	
	struct{
		fw_memory_t memory;
		uint32_t pointer;
		void* attachment;
		mcu_framework_driver_ma104_execute_t execute;
	}transfer;
	
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
