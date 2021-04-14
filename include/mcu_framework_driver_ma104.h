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
typedef struct _mcu_framework_driver_ma104_memory_t mcu_framework_driver_ma104_memory_t;
 
typedef void(*mcu_framework_driver_ma104_execute_t)(mcu_framework_driver_ma104_memory_t* _this, fw_memory_t* data, void* attachment);
typedef void(*mcu_framework_driver_ma104_event_hardwareBusy_t)(mcu_framework_driver_ma104_memory_t* _this);
 
typedef struct _mcu_framework_driver_ma104_memory_t{ 
  fw_usart_handle_t*  fw_usart;
	fw_io_pin_handle_t* fw_pin_usbok;
	fw_io_pin_handle_t* fw_pin_reset;
	tool_ring_buffer_t ringBuffer;
	uint16_t initFlag;
	uint16_t flag;
	
	struct{
		fw_memory_t memory;
		uint32_t pointer;
		void* attachment;
		mcu_framework_driver_ma104_execute_t execute;
	}transfer;
	
	struct{
	  mcu_framework_driver_ma104_event_hardwareBusy_t hardwareBusy;
	}event;
	
	uint8_t transferCache[10];
	uint8_t receiverCache;
}mcu_framework_driver_ma104_memory_t;

typedef struct _mcu_framework_driver_ma104_api_t{
	bool     (*init)(mcu_framework_driver_ma104_memory_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset);
	bool     (*receiverEnable)(mcu_framework_driver_ma104_memory_t* _this, void* buffer, uint32_t size);
	bool     (*receiverDisable)(mcu_framework_driver_ma104_memory_t* _this);
	bool     (*write)(mcu_framework_driver_ma104_memory_t* _this, fw_memory_t* data, mcu_framework_driver_ma104_execute_t execute, void* attachment);
	bool     (*writeByte)(mcu_framework_driver_ma104_memory_t* _this, uint8_t data);
	uint32_t (*read)(mcu_framework_driver_ma104_memory_t* _this, void *buffer, uint32_t size);
	bool     (*readByte)(mcu_framework_driver_ma104_memory_t* _this, uint8_t* buffer);
	uint32_t (*getReceiverCount)(mcu_framework_driver_ma104_memory_t* _this);
	bool     (*isBusy)(mcu_framework_driver_ma104_memory_t* _this);
	bool     (*reset)(mcu_framework_driver_ma104_memory_t* _this);
	bool     (*beginTransfer)(mcu_framework_driver_ma104_memory_t* _this);
	bool     (*isHardwareBusy)(mcu_framework_driver_ma104_memory_t* _this);
	
	struct{
		bool (*setHardwareBusy)(mcu_framework_driver_ma104_memory_t* _this, mcu_framework_driver_ma104_event_hardwareBusy_t event);
	}event;
}mcu_framework_driver_ma104_api_t;
/* *****************************************************************************************
 *    Function Type
 */ 


/* *****************************************************************************************
 *    API List
 */ 
extern const mcu_framework_driver_ma104_api_t mcu_framework_driver_ma104_api;

bool mcu_framework_driver_ma104_init(mcu_framework_driver_ma104_memory_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset);
bool mcu_framework_driver_ma104_receiverEnable(mcu_framework_driver_ma104_memory_t* _this, void* buffer, uint32_t size);
bool mcu_framework_driver_ma104_receiverDisable(mcu_framework_driver_ma104_memory_t* _this);
bool mcu_framework_driver_ma104_write(mcu_framework_driver_ma104_memory_t* _this, fw_memory_t* data, mcu_framework_driver_ma104_execute_t execute, void* attachment);
bool mcu_framework_driver_ma104_writeByte(mcu_framework_driver_ma104_memory_t* _this, uint8_t data);
uint32_t mcu_framework_driver_ma104_read(mcu_framework_driver_ma104_memory_t* _this, void *buffer, uint32_t size);
bool mcu_framework_driver_ma104_readByte(mcu_framework_driver_ma104_memory_t* _this, uint8_t* buffer);
uint32_t mcu_framework_driver_ma104_getReceiverCount(mcu_framework_driver_ma104_memory_t* _this);
bool mcu_framework_driver_ma104_isBusy(mcu_framework_driver_ma104_memory_t* _this);
bool mcu_framework_driver_ma104_reset(mcu_framework_driver_ma104_memory_t* _this);
bool mcu_framework_driver_ma104_beginTransfer(mcu_framework_driver_ma104_memory_t* _this);
bool mcu_framework_driver_ma104_isHardwareBusy(mcu_framework_driver_ma104_memory_t* _this);
bool mcu_framework_driver_ma104_event_setHardwareBusy(mcu_framework_driver_ma104_memory_t* _this, mcu_framework_driver_ma104_event_hardwareBusy_t event);

#ifdef __cplusplus
}
#endif //__cplusplus
#endif
/* *****************************************************************************************
 *    End of file
 */ 
