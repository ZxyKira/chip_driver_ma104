/* *****************************************************************************************
 *    File Name   :mcu_framework_driver_ma104.c
 *    Create Date :2021-04-13
 *    Modufy Date :
 *    Information :
 */

#include <string.h>

#include "mcu_framework_driver_ma104.h"

#include "fw_usart.h"
#include "fw_io_pin.h"

#include "tool_ring_buffer.h"

/* *****************************************************************************************
 *    Macro
 */

#define FLAG_BUSY (1<<0)
#define FLAG_RECEIVER_ENABLE (1<<1)


#define MACRO_IS_BUSY(_this, action) if(_this->flag & FLAG_BUSY) action
	

#ifdef NDEBUG
#define DEBUG_CHECK_POINTER(pointer, action)
#else
#define DEBUG_CHECK_POINTER(pointer, action) if(!pointer) action
#endif
/* *****************************************************************************************
 *    Parameter
 */
 
/* *****************************************************************************************
 *    Type/Structure
 */ 
 
/* *****************************************************************************************
 *    Extern Function/Variable
 */

bool mcu_framework_driver_ma104_reset(mcu_framework_driver_ma104_handle_memory_t* _this);
/* *****************************************************************************************
 *    Public Variable
 */

/* *****************************************************************************************
 *    Private Variable
 */

/* *****************************************************************************************
 *    Inline Function
 */
 
/* *****************************************************************************************
 *    Private Function
 */  

static bool ma104_writeCache(uint8_t* cache, uint8_t* data, uint8_t len){
	if(len >= 6)
		return false;
	
	int i;
	
	//write len
	cache[2] = len;
	cache[9] = 0x00;
	cache[9] ^= 0x05; 
	cache[9] ^= len;
	
	for(i=0; i<6; i++){
		if(i >= len)
			cache[3+i] = 0;
		else
			cache[3+i] = data[i];
		
		cache[9] ^= cache[3+i];
	}

	return true;
} 
 
static void mcu_framework_driver_ma104_event_send(fw_usart_handle_t* handle, fw_memory_t* data, void* attachment){

}

static void mcu_framework_driver_ma104_event_read_t(fw_usart_handle_t* handle, fw_memory_t* buffer, void* attachment){

}
/* *****************************************************************************************
 *    Public Function
 */
bool mcu_framework_driver_ma104_init(mcu_framework_driver_ma104_handle_memory_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset){
	if((!_this) | (!usart))
		return false;
	
	memset(_this, 0x00, sizeof(mcu_framework_driver_ma104_handle_memory_t));
	
	_this->fw_pin_usbok = usbok;
	_this->fw_pin_reset	= reset;
	
	//reset ma104
	mcu_framework_driver_ma104_reset(_this);
	
	//init usert
	if(_this->fw_usart->api->init(_this->fw_usart))
		return false;
	
	//set baudrate 19200
	if(_this->fw_usart->api->setBaudrate(_this->fw_usart, 19200))
		return false;
	
	_this->xferCache[0] = 0x02;
	_this->xferCache[1] = 0x05;
	
	return true;
}

bool mcu_framework_driver_ma104_receiverEnable(mcu_framework_driver_ma104_handle_memory_t* _this, void* buffer, uint32_t size){
	if(!buffer)
		return false;
	
	if(_this->flag & FLAG_RECEIVER_ENABLE)
		return false;
	
  if(!tool_ring_buffer_init(&_this->ringBuffer, buffer, size))
		return false;

	_this->flag |= FLAG_RECEIVER_ENABLE;
	
	return true;
}

bool mcu_framework_driver_ma104_receiverDisable(mcu_framework_driver_ma104_handle_memory_t* _this){

}

bool mcu_framework_driver_ma104_write(mcu_framework_driver_ma104_handle_memory_t* _this, fw_memory_t* data, mcu_framework_driver_ma104_execute_t execute, void* attachment){
	DEBUG_CHECK_POINTER(data, return false);
	DEBUG_CHECK_POINTER(data->ptr, return false);

	MACRO_IS_BUSY(_this, return false);
	
	_this->transfer.memory = *data;
	_this->transfer.pointer = 0;
	_this->transfer.attachment = attachment;
	_this->transfer.execute = execute;
}

bool mcu_framework_driver_ma104_writeByte(mcu_framework_driver_ma104_handle_memory_t* _this){
	
}

bool mcu_framework_driver_ma104_read(mcu_framework_driver_ma104_handle_memory_t* _this, fw_memory_t* data, mcu_framework_driver_ma104_execute_t execute, void* attachment){
	
}

bool mcu_framework_driver_ma104_readByte(mcu_framework_driver_ma104_handle_memory_t* _this, uint8_t* buffer){
	if(tool_ring_buffer_isEmpty(&_this->ringBuffer))
		return false;
	
	if(!buffer)
		return false;
		
	return tool_ring_buffer_pop(&_this->ringBuffer, buffer);
}

uint32_t mcu_framework_driver_ma104_getReceiverCount(mcu_framework_driver_ma104_handle_memory_t* _this){
	return tool_ring_buffer_getCount(&_this->ringBuffer);
}

bool mcu_framework_driver_ma104_isBusy(mcu_framework_driver_ma104_handle_memory_t* _this){
	MACRO_IS_BUSY(_this, return true);
	return false;
}

bool mcu_framework_driver_ma104_reset(mcu_framework_driver_ma104_handle_memory_t* _this){
	if(!_this->fw_pin_reset)
		return false;

	_this->fw_pin_reset->api->setLow(_this->fw_pin_reset);
	_this->fw_pin_reset->api->setHigh(_this->fw_pin_reset);
	
	return true;
}

/* *****************************************************************************************
 *    API Link
 */ 


/* *****************************************************************************************
 *    End of file
 */ 
