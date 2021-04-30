/* *****************************************************************************************
 *    File Name   :chip_driver_ma104.c
 *    Create Date :2021-04-13
 *    Modufy Date :2021-04-60
 *    Information :
 */

/* *****************************************************************************************
 *    Include
 */ 
#include <string.h>

#include "chip_driver_ma104.h"
#include "tool_ring_buffer.h"
#include "fw_usart.h"
#include "fw_io_pin.h"



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
 *    Extern Function/Variable
 */

bool chip_driver_ma104_reset(chip_driver_ma104_memory_t* _this);
bool chip_driver_ma104_isHardwareBusy(chip_driver_ma104_memory_t* _this);
static bool chip_driver_ma104_autoTransfer(chip_driver_ma104_memory_t* _this, fw_memory_t* sendMemory);



/* *****************************************************************************************
 *    Typedef List
 */ 

/* *****************************************************************************************
 *    Typedef Function
 */ 

/* *****************************************************************************************
 *    Struct/Union/Enum
 */ 

/* *****************************************************************************************
 *    Typedef Struct/Union/Enum
 */ 

/* *****************************************************************************************
 *    Private Variable
 */

/* *****************************************************************************************
 *    Public Variable
 */

/* *****************************************************************************************
 *    Private Function
 */  

/*----------------------------------------
 *  chip_driver_ma104_writeCache
 *----------------------------------------*/
static bool chip_driver_ma104_writeCache(uint8_t* cache, uint8_t* data, uint8_t len){
  if(len > 6)
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



/*----------------------------------------
 *  chip_driver_ma104_event_send
 *----------------------------------------*/
static void chip_driver_ma104_event_send(fw_usart_handle_t* handle, fw_memory_t* data, void* attachment){
  chip_driver_ma104_memory_t* _this = attachment;
  if(!chip_driver_ma104_autoTransfer(_this, data)){
    //transfer finish or fail
    
    //unset FLAG_BUSY
    _this->flag &= ~ FLAG_BUSY;
    
    if(_this->transfer.execute){
      fw_memory_t transferMemory = _this->transfer.memory;
      _this->transfer.execute(_this, &transferMemory, _this->transfer.attachment);
    }
  }
}



/*----------------------------------------
 *  chip_driver_ma104_event_read
 *----------------------------------------*/
static void chip_driver_ma104_event_read(fw_usart_handle_t* handle, fw_memory_t* buffer, void* attachment){
  chip_driver_ma104_memory_t* _this = attachment;
  
  //is disable receiver;
  if(!(_this->flag & FLAG_RECEIVER_ENABLE))
    return;
  
  //insert data
  tool_ring_buffer_insert(&_this->ringBuffer, buffer->ptr);
  
  //usart read byte
  _this->fw_usart->api->read(_this->fw_usart, buffer, chip_driver_ma104_event_read, _this);
  
  return;
}



/*----------------------------------------
 *  chip_driver_ma104_autoTransfer
 *----------------------------------------*/
static bool chip_driver_ma104_autoTransfer(chip_driver_ma104_memory_t* _this, fw_memory_t* sendMemory){
  
  //check hardware busy
  if(chip_driver_ma104_isHardwareBusy(_this)){
    
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
  chip_driver_ma104_writeCache(_this->transferCache ,&((uint8_t*)_this->transfer.memory.ptr)[_this->transfer.pointer], transferSize);
  
  //add pointer
  _this->transfer.pointer += transferSize;
  

  return _this->fw_usart->api->send(_this->fw_usart, sendMemory, chip_driver_ma104_event_send, _this);
}



/* *****************************************************************************************
 *    Public Function
 */

/*----------------------------------------
 *  chip_driver_ma104_init
 *----------------------------------------*/
bool chip_driver_ma104_init(chip_driver_ma104_memory_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset){
  if((!_this) | (!usart))
    return false;
  
  if(MACRO_CHECK_INIT_FLAG(_this))
    return false;
  
  memset(_this, 0x00, sizeof(chip_driver_ma104_memory_t));
  
  _this->fw_usart = usart;
  _this->fw_pin_usbok = usbok;
  _this->fw_pin_reset  = reset;
  
  //reset ma104
  chip_driver_ma104_reset(_this);
  
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



/*----------------------------------------
 *  chip_driver_ma104_receiverEnable
 *----------------------------------------*/
bool chip_driver_ma104_receiverEnable(chip_driver_ma104_memory_t* _this, void* buffer, uint32_t size){
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
  if(_this->fw_usart->api->read(_this->fw_usart, &readMemory, chip_driver_ma104_event_read, _this))
    return true;
  
  //start receiver fail
  _this->flag &= ~FLAG_RECEIVER_ENABLE;
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_receiverDisable
 *----------------------------------------*/
bool chip_driver_ma104_receiverDisable(chip_driver_ma104_memory_t* _this){
  //check flag FLAG_RECEIVER_ENABLE is enable; is disable return false
  if(!(_this->flag & FLAG_RECEIVER_ENABLE))
    return false;
  
  //unset FLAG_RECEIVER_ENABLE
  _this->flag &= ~FLAG_RECEIVER_ENABLE;
  
  //usart hardware abortRead
  return _this->fw_usart->api->abortRead(_this->fw_usart);
}



/*----------------------------------------
 *  chip_driver_ma104_write
 *----------------------------------------*/
bool chip_driver_ma104_write(chip_driver_ma104_memory_t* _this, fw_memory_t* data, chip_driver_ma104_execute_t execute, void* attachment){
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
  if(chip_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->flag &= ~FLAG_BUSY;  
  
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_writeByte
 *----------------------------------------*/
bool chip_driver_ma104_writeByte(chip_driver_ma104_memory_t* _this, uint8_t data){
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
  if(chip_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->flag &= ~FLAG_BUSY;  
  
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_read
 *----------------------------------------*/
uint32_t chip_driver_ma104_read(chip_driver_ma104_memory_t* _this, void *buffer, uint32_t size){
  if(tool_ring_buffer_isEmpty(&_this->ringBuffer))
    return 0;
  
  return tool_ring_buffer_popMult(&_this->ringBuffer, buffer, size);
}



/*----------------------------------------
 *  chip_driver_ma104_readByte
 *----------------------------------------*/
bool chip_driver_ma104_readByte(chip_driver_ma104_memory_t* _this, uint8_t* buffer){
  if(tool_ring_buffer_isEmpty(&_this->ringBuffer))
    return false;
  
  if(!buffer)
    return false;
    
  return tool_ring_buffer_pop(&_this->ringBuffer, buffer);
}



/*----------------------------------------
 *  chip_driver_ma104_getReceiverCount
 *----------------------------------------*/
uint32_t chip_driver_ma104_getReceiverCount(chip_driver_ma104_memory_t* _this){
  return tool_ring_buffer_getCount(&_this->ringBuffer);
}



/*----------------------------------------
 *  chip_driver_ma104_isBusy
 *----------------------------------------*/
bool chip_driver_ma104_isBusy(chip_driver_ma104_memory_t* _this){
  MACRO_IS_FLAG(_this, FLAG_BUSY, return true);
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_reset
 *----------------------------------------*/
bool chip_driver_ma104_reset(chip_driver_ma104_memory_t* _this){
  if(!_this->fw_pin_reset)
    return false;

  _this->fw_pin_reset->api->setLow(_this->fw_pin_reset);
  _this->fw_pin_reset->api->setHigh(_this->fw_pin_reset);
  
  return true;
}



/*----------------------------------------
 *  chip_driver_ma104_beginTransfer
 *----------------------------------------*/
bool chip_driver_ma104_beginTransfer(chip_driver_ma104_memory_t* _this){
  //check busy flag, if flag is unset will return false
  if(!(_this->flag & FLAG_BUSY))
    return false;
  
  fw_memory_t sendMemory = {
    .ptr = &_this->transferCache[0],
    .size = 10
  };  
  
  //try to transfer data as usart, if successful will return true
  if(chip_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->flag &= ~FLAG_BUSY;  
  
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_isHardwareBusy
 *----------------------------------------*/
bool chip_driver_ma104_isHardwareBusy(chip_driver_ma104_memory_t* _this){
  if(!_this->fw_pin_usbok)
    return false;
  
  return _this->fw_pin_usbok->api->getValue(_this->fw_pin_usbok);
}



/*----------------------------------------
 *  chip_driver_ma104_event_setHardwareBusy
 *----------------------------------------*/
bool chip_driver_ma104_event_setHardwareBusy(chip_driver_ma104_memory_t* _this, chip_driver_ma104_event_hardwareBusy_t event){
  _this->event.hardwareBusy = event;
return true;
}



/* *****************************************************************************************
 *    API Link
 */ 

/*----------------------------------------
 *  chip_driver_ma104_api
 *----------------------------------------*/
const struct chip_driver_ma104_api_t chip_driver_ma104_api = {
  .init               = chip_driver_ma104_init,
  .receiverEnable     = chip_driver_ma104_receiverEnable,
  .receiverDisable    = chip_driver_ma104_receiverDisable,
  .write              = chip_driver_ma104_write,
  .writeByte          = chip_driver_ma104_writeByte,
  .read               = chip_driver_ma104_read,
  .readByte           = chip_driver_ma104_readByte,
  .getReceiverCount   = chip_driver_ma104_getReceiverCount,
  .isBusy             = chip_driver_ma104_isBusy,
  .reset              = chip_driver_ma104_reset,
  .beginTransfer      = chip_driver_ma104_beginTransfer,
  .isHardwareBusy     = chip_driver_ma104_isHardwareBusy,
  .event = {
    .setHardwareBusy  = chip_driver_ma104_event_setHardwareBusy,
  }
};



/* *****************************************************************************************
 *    End of file
 */ 
