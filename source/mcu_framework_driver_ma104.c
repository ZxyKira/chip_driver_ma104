/* *****************************************************************************************
 *    File Name   :mcu_framework_driver_ma104.c
 *    Create Date :2021-04-13
 *    Modufy Date :2021-04-14
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

#define INIT_FLAG ((uint16_t)0xDC5E)

#define FLAG_BUSY (1<<0)
#define FLAG_RECEIVER_ENABLE (1<<1)
#define FLAG_WAIT_USBOK (1<<2)

#define MACRO_IS_FLAG(_this, flags, action) if(_this->flag & flags) action
#define MACRO_CHECK_INIT_FLAG(_this) (_this->initFlag == INIT_FLAG)?true:false
  

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

bool mcu_framework_driver_ma104_reset(mcu_framework_driver_ma104_memory_t* _this);
bool mcu_framework_driver_ma104_isHardwareBusy(mcu_framework_driver_ma104_memory_t* _this);
static bool mcu_framework_driver_ma104_autoTransfer(mcu_framework_driver_ma104_memory_t* _this, fw_memory_t* sendMemory);
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


static bool mcu_framework_driver_ma104_writeCache(uint8_t* cache, uint8_t* data, uint8_t len){
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
  mcu_framework_driver_ma104_memory_t* _this = attachment;
  if(!mcu_framework_driver_ma104_autoTransfer(_this, data)){
    //transfer finish or fail
    
    //unset FLAG_BUSY
    _this->flag &= ~ FLAG_BUSY;
    
    if(_this->transfer.execute){
      fw_memory_t transferMemory = _this->transfer.memory;
      _this->transfer.execute(_this, &transferMemory, _this->transfer.attachment);
    }
  }
}

static void mcu_framework_driver_ma104_event_read(fw_usart_handle_t* handle, fw_memory_t* buffer, void* attachment){
  mcu_framework_driver_ma104_memory_t* _this = attachment;
  
  //is disable receiver;
  if(!(_this->flag & FLAG_RECEIVER_ENABLE))
    return;
  
  //insert data
  tool_ring_buffer_insert(&_this->ringBuffer, buffer->ptr);
  
  //usart read byte
  _this->fw_usart->api->read(_this->fw_usart, buffer, mcu_framework_driver_ma104_event_read, _this);
  
  return;
}

static bool mcu_framework_driver_ma104_autoTransfer(mcu_framework_driver_ma104_memory_t* _this, fw_memory_t* sendMemory){
  
  //check hardware busy
  if(mcu_framework_driver_ma104_isHardwareBusy(_this)){
    
    //set FLAG_WAIT_USBOK
    _this->flag |= FLAG_WAIT_USBOK;
    
    //check event pointer and call event
    if(_this->event.hardwareBusy)
      _this->event.hardwareBusy(_this);
    
    return true;
  }
  
  int transferSize = _this->transfer.memory.size - _this->transfer.pointer;
  
  //not thing need to transmit
  if(transferSize <= 0){
    return false;
  }
    
  if(transferSize >= 6)
    transferSize = 6;
  
  //generator packet
  mcu_framework_driver_ma104_writeCache(_this->transferCache ,&((uint8_t*)_this->transfer.memory.ptr)[_this->transfer.pointer], transferSize);
  
  //add pointer
  _this->transfer.pointer += transferSize;
  

  return _this->fw_usart->api->send(_this->fw_usart, sendMemory, mcu_framework_driver_ma104_event_send, _this);
}

/* *****************************************************************************************
 *    Public Function
 */
bool mcu_framework_driver_ma104_init(mcu_framework_driver_ma104_memory_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset){
  if((!_this) | (!usart))
    return false;
  
  if(MACRO_CHECK_INIT_FLAG(_this))
    return false;
  
  memset(_this, 0x00, sizeof(mcu_framework_driver_ma104_memory_t));
  
  _this->fw_usart = usart;
  _this->fw_pin_usbok = usbok;
  _this->fw_pin_reset  = reset;
  
  //reset ma104
  mcu_framework_driver_ma104_reset(_this);
  
  //init usert
  if(!_this->fw_usart->api->init(_this->fw_usart))
    return false;
  
  //set baudrate 19200
  if(!_this->fw_usart->api->setBaudrate(_this->fw_usart, 19200))
    return false;
  
  _this->transferCache[0] = 0x02;
  _this->transferCache[1] = 0x05;
  
  _this->initFlag = INIT_FLAG;
  
  return true;
}

bool mcu_framework_driver_ma104__deinit(mcu_framework_driver_ma104_memory_t* _this){
  _this->fw_usart->api->deinit(_this->fw_usart);
  return true;
}

bool mcu_framework_driver_ma104_receiverEnable(mcu_framework_driver_ma104_memory_t* _this, void* buffer, uint32_t size){
  if(!buffer)
    return false;
  
  if(_this->flag & FLAG_RECEIVER_ENABLE)
    return false;
  
  if(_this->fw_usart->api->isReadBusy(_this->fw_usart))
    return false;
  
  if(!tool_ring_buffer_init(&_this->ringBuffer, buffer, size))
    return false;

  _this->flag |= FLAG_RECEIVER_ENABLE;
  
  fw_memory_t readMemory = {
    .ptr = &_this->receiverCache,
    .size = 1
  };
  
  //start receiver
  if(_this->fw_usart->api->read(_this->fw_usart, &readMemory, mcu_framework_driver_ma104_event_read, _this))
    return true;
  
  //start receiver fail
  _this->flag &= ~FLAG_RECEIVER_ENABLE;
  return false;
}

bool mcu_framework_driver_ma104_receiverDisable(mcu_framework_driver_ma104_memory_t* _this){
  //check flag FLAG_RECEIVER_ENABLE is enable; is disable return false
  if(!(_this->flag & FLAG_RECEIVER_ENABLE))
    return false;
  
  //unset FLAG_RECEIVER_ENABLE
  _this->flag &= ~FLAG_RECEIVER_ENABLE;
  
  //usart hardware abortRead
  return _this->fw_usart->api->abortRead(_this->fw_usart);
}

bool mcu_framework_driver_ma104_write(mcu_framework_driver_ma104_memory_t* _this, fw_memory_t* data, mcu_framework_driver_ma104_execute_t execute, void* attachment){
  DEBUG_CHECK_POINTER(data, return false);
  DEBUG_CHECK_POINTER(data->ptr, return false);

  //check BUSY flag; is busy return false
  MACRO_IS_FLAG(_this, FLAG_BUSY, return false);
  
  //set busy
  _this->flag |= FLAG_BUSY;  
  
  //write data in to this
  _this->transfer.memory = *data;
  _this->transfer.pointer = 0;
  _this->transfer.attachment = attachment;
  _this->transfer.execute = execute;
  
  fw_memory_t sendMemory = {
    .ptr = &_this->transferCache[0],
    .size = 10
  };
  
  //try to transfer data as usart, if successful will return true
  if(mcu_framework_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->flag &= ~FLAG_BUSY;  
  
  return false;
}

bool mcu_framework_driver_ma104_writeByte(mcu_framework_driver_ma104_memory_t* _this, uint8_t data){
  //check BUSY flag; is busy return false
  MACRO_IS_FLAG(_this, FLAG_BUSY, return false);
  
  //set busy
  _this->flag |= FLAG_BUSY;
  
  //write data in to this
  _this->transfer.attachment = (void*)((uint32_t)data);
  _this->transfer.memory.ptr = &_this->transfer.attachment;
  _this->transfer.memory.size = 1;
  _this->transfer.pointer = 0;
  
  fw_memory_t sendMemory = {
    .ptr = &_this->transferCache[0],
    .size = 10
  };
  
  //try to transfer data as usart, if successful will return true
  if(mcu_framework_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->flag &= ~FLAG_BUSY;  
  
  return false;
}

uint32_t mcu_framework_driver_ma104_read(mcu_framework_driver_ma104_memory_t* _this, void *buffer, uint32_t size){
  if(tool_ring_buffer_isEmpty(&_this->ringBuffer))
    return 0;
  
  return tool_ring_buffer_popMult(&_this->ringBuffer, buffer, size);
}

bool mcu_framework_driver_ma104_readByte(mcu_framework_driver_ma104_memory_t* _this, uint8_t* buffer){
  if(tool_ring_buffer_isEmpty(&_this->ringBuffer))
    return false;
  
  if(!buffer)
    return false;
    
  return tool_ring_buffer_pop(&_this->ringBuffer, buffer);
}

uint32_t mcu_framework_driver_ma104_getReceiverCount(mcu_framework_driver_ma104_memory_t* _this){
  return tool_ring_buffer_getCount(&_this->ringBuffer);
}

bool mcu_framework_driver_ma104_isBusy(mcu_framework_driver_ma104_memory_t* _this){
  MACRO_IS_FLAG(_this, FLAG_BUSY, return true);
  return false;
}

bool mcu_framework_driver_ma104_reset(mcu_framework_driver_ma104_memory_t* _this){
  if(!_this->fw_pin_reset)
    return false;

  _this->fw_pin_reset->api->setLow(_this->fw_pin_reset);
  _this->fw_pin_reset->api->setHigh(_this->fw_pin_reset);
  
  return true;
}

bool mcu_framework_driver_ma104_beginTransfer(mcu_framework_driver_ma104_memory_t* _this){
  //check busy flag, if flag is unset will return false
  if(!(_this->flag & FLAG_BUSY))
    return false;
  
  fw_memory_t sendMemory = {
    .ptr = &_this->transferCache[0],
    .size = 10
  };  
  
  //try to transfer data as usart, if successful will return true
  if(mcu_framework_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->flag &= ~FLAG_BUSY;  
  
  return false;
}

bool mcu_framework_driver_ma104_isHardwareBusy(mcu_framework_driver_ma104_memory_t* _this){
  if(!_this->fw_pin_usbok)
    return false;
  
  return _this->fw_pin_usbok->api->getValue(_this->fw_pin_usbok);
}

bool mcu_framework_driver_ma104_event_setHardwareBusy(mcu_framework_driver_ma104_memory_t* _this, mcu_framework_driver_ma104_event_hardwareBusy_t event){
  _this->event.hardwareBusy = event;
return true;
}

/* *****************************************************************************************
 *    API Link
 */ 
const mcu_framework_driver_ma104_api_t mcu_framework_driver_ma104_api = {
  .init               = mcu_framework_driver_ma104_init,
  .receiverEnable     = mcu_framework_driver_ma104_receiverEnable,
  .receiverDisable    = mcu_framework_driver_ma104_receiverDisable,
  .write              = mcu_framework_driver_ma104_write,
  .writeByte          = mcu_framework_driver_ma104_writeByte,
  .read               = mcu_framework_driver_ma104_read,
  .readByte           = mcu_framework_driver_ma104_readByte,
  .getReceiverCount   = mcu_framework_driver_ma104_getReceiverCount,
  .isBusy             = mcu_framework_driver_ma104_isBusy,
  .reset              = mcu_framework_driver_ma104_reset,
  .beginTransfer      = mcu_framework_driver_ma104_beginTransfer,
  .isHardwareBusy     = mcu_framework_driver_ma104_isHardwareBusy,
  .event = {
    .setHardwareBusy  = mcu_framework_driver_ma104_event_setHardwareBusy,
  }
};

/* *****************************************************************************************
 *    End of file
 */ 
