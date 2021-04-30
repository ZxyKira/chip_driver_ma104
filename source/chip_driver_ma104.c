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
#include "fw_usart.h"
#include "fw_io_pin.h"



/* *****************************************************************************************
 *    Macro
 */

#define ENABLE_FLAG 0x3F6EF1F6



/*----------------------------------------
 *  FLAG
 *----------------------------------------*/
#define FLAG_TXBUSY (1<<0)
#define FLAG_RXBUSY (1<<1)
#define FLAG_WAIT_USBOK (1<<2)



/*----------------------------------------
 *  MACRO_IS_FLAG
 *----------------------------------------*/
#define MACRO_IS_FLAG(_this, flags, action) if(_this->handle.flag & flags) action



/*----------------------------------------
 *  MACRO_CHECK_ENABLE_FLAG
 *----------------------------------------*/
#define MACRO_CHECK_ENABLE_FLAG(_this) (_this->enableFlag == ENABLE_FLAG)?true:false
  


/*----------------------------------------
 *  DEBUG_CHECK_POINTER
 *----------------------------------------*/
#ifdef NDEBUG
#define DEBUG_CHECK_POINTER(pointer, action)
#else
#define DEBUG_CHECK_POINTER(pointer, action) if(!pointer) action
#endif



/* *****************************************************************************************
 *    Extern Function/Variable
 */

bool chip_driver_ma104_reset(chip_driver_ma104_handle_t* _this);
bool chip_driver_ma104_isHardwareBusy(chip_driver_ma104_handle_t* _this);
static bool chip_driver_ma104_autoTransfer(chip_driver_ma104_handle_t* _this, fw_memory_t* sendMemory);



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
  chip_driver_ma104_handle_t* _this = attachment;
  if(!chip_driver_ma104_autoTransfer(_this, data)){
    //transfer finish or fail
    
    //unset FLAG_TXBUSY
    _this->handle.flag &= ~ FLAG_TXBUSY;
    
    if(_this->handle.transfer.execute){
      fw_memory_t transferMemory = _this->handle.transfer.memory;
      _this->handle.transfer.execute(_this, &transferMemory, _this->handle.transfer.attachment);
    }
  }
}



/*----------------------------------------
 *  chip_driver_ma104_event_read
 *----------------------------------------*/
static void chip_driver_ma104_event_read(fw_usart_handle_t* handle, fw_memory_t* data, void* attachment){
  chip_driver_ma104_handle_t* _this = attachment;
   
  //unset FLAG_RXBUSY
  _this->handle.flag &= ~ FLAG_RXBUSY;
    
  if(_this->handle.receiver.execute){
    fw_memory_t transferMemory = _this->handle.receiver.memory;
    _this->handle.receiver.execute(_this, &transferMemory, _this->handle.receiver.attachment);
  }
}



/*----------------------------------------
 *  chip_driver_ma104_autoTransfer
 *----------------------------------------*/
static bool chip_driver_ma104_autoTransfer(chip_driver_ma104_handle_t* _this, fw_memory_t* sendMemory){
  
  //check hardware busy
  if(chip_driver_ma104_isHardwareBusy(_this)){
    
    //set FLAG_WAIT_USBOK
    _this->handle.flag |= FLAG_WAIT_USBOK;
    
    //check event pointer and call event
    if(_this->config.hardwareBusy)
      _this->config.hardwareBusy(_this);
    
    return true;
  }
  
  int transferSize = _this->handle.transfer.memory.size - _this->handle.transfer.pointer;
  
  //not thing need to transmit
  if(transferSize <= 0){
    return false;
  }
    
  if(transferSize >= 6)
    transferSize = 6;
  
  //generator packet
  chip_driver_ma104_writeCache(_this->handle.transferCache ,&((uint8_t*)_this->handle.transfer.memory.ptr)[_this->handle.transfer.pointer], transferSize);
  
  //add pointer
  _this->handle.transfer.pointer += transferSize;
  

  return _this->reference.usart->api->send(_this->reference.usart, sendMemory, chip_driver_ma104_event_send, _this);
}



/* *****************************************************************************************
 *    Public Function
 */

/*----------------------------------------
 *  chip_driver_ma104_init
 *----------------------------------------*/
bool chip_driver_ma104_init(chip_driver_ma104_handle_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset){
  if((!_this) | (!usart))
    return false;
  
  if(MACRO_CHECK_ENABLE_FLAG(_this))
    return false;
  
	//check usart is enable?
	if(!usart->api->isEnable(usart))
    return false;
	
  //set baudrate 19200
  if(!usart->api->setBaudrate(usart, 19200))
    return false;	
	
  memset(_this, 0x00, sizeof(chip_driver_ma104_handle_t));
  
  _this->reference.usart = usart;
  _this->reference.usbok = usbok;
  _this->reference.reset = reset;
  
  //reset ma104
  chip_driver_ma104_reset(_this);
  

  
  _this->handle.transferCache[0] = 0x02;
  _this->handle.transferCache[1] = 0x05;
  
  _this->enableFlag = ENABLE_FLAG;
  
  return true;
}


/*----------------------------------------
 *  chip_driver_ma104_write
 *----------------------------------------*/
bool chip_driver_ma104_write(chip_driver_ma104_handle_t* _this, fw_memory_t* data, chip_driver_ma104_execute_t execute, void* attachment){
  DEBUG_CHECK_POINTER(data, return false);
  DEBUG_CHECK_POINTER(data->ptr, return false);

  //check BUSY flag; is busy return false
  MACRO_IS_FLAG(_this, FLAG_TXBUSY, return false);
  
  //set busy
  _this->handle.flag |= FLAG_TXBUSY;  
  
  //write data in to this
  _this->handle.transfer.memory = *data;
  _this->handle.transfer.pointer = 0;
  _this->handle.transfer.attachment = attachment;
  _this->handle.transfer.execute = execute;
  
  fw_memory_t sendMemory = {
    .ptr = &_this->handle.transferCache[0],
    .size = 10
  };
  
  //try to transfer data as usart, if successful will return true
  if(chip_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->handle.flag &= ~FLAG_TXBUSY;  
  
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_writeByte
 *----------------------------------------*/
bool chip_driver_ma104_writeByte(chip_driver_ma104_handle_t* _this, uint8_t data){
  //check BUSY flag; is busy return false
  MACRO_IS_FLAG(_this, FLAG_TXBUSY, return false);
  
  //set busy
  _this->handle.flag |= FLAG_TXBUSY;
  
  //write data in to this
  _this->handle.transfer.attachment = (void*)((uint32_t)data);
  _this->handle.transfer.memory.ptr = &_this->handle.transfer.attachment;
  _this->handle.transfer.memory.size = 1;
  _this->handle.transfer.pointer = 0;
  
  fw_memory_t sendMemory = {
    .ptr = &_this->handle.transferCache[0],
    .size = 10
  };
  
  //try to transfer data as usart, if successful will return true
  if(chip_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->handle.flag &= ~FLAG_TXBUSY;  
  
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_read
 *----------------------------------------*/
bool chip_driver_ma104_read(chip_driver_ma104_handle_t* _this, fw_memory_t* data, chip_driver_ma104_execute_t execute, void* attachment){
  DEBUG_CHECK_POINTER(data, return false);
  DEBUG_CHECK_POINTER(data->ptr, return false);
		
  //check BUSY flag; is busy return false
  MACRO_IS_FLAG(_this, FLAG_RXBUSY, return false);
  
  //set busy
  _this->handle.flag |= FLAG_RXBUSY;
	
	fw_usart_handle_t* usart = _this->reference.usart;
	_this->handle.receiver.execute = execute;
	_this->handle.receiver.attachment = attachment;
	_this->handle.receiver.memory = *data;
	
	bool result = usart->api->read(usart, data, chip_driver_ma104_event_read, _this);
	
	if(result)
    return true;
	
	//receiver fail unset busy
  _this->handle.flag &= ~FLAG_RXBUSY;
	
	return false;
}



/*----------------------------------------
 *  chip_driver_ma104_readByte
 *----------------------------------------*/
bool chip_driver_ma104_readByte(chip_driver_ma104_handle_t* _this, uint8_t* buffer){
  return _this->reference.usart->api->readByte(_this->reference.usart, buffer);
}



/*----------------------------------------
 *  chip_driver_ma104_isBusyWrite
 *----------------------------------------*/
bool chip_driver_ma104_isBusyWrite(chip_driver_ma104_handle_t* _this){
  MACRO_IS_FLAG(_this, FLAG_TXBUSY, return true);
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_isBusyRead
 *----------------------------------------*/
bool chip_driver_ma104_isBusyRead(chip_driver_ma104_handle_t* _this){
  MACRO_IS_FLAG(_this, FLAG_RXBUSY, return true);
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_reset
 *----------------------------------------*/
bool chip_driver_ma104_reset(chip_driver_ma104_handle_t* _this){
  if(!_this->reference.reset)
    return false;

  _this->reference.reset->api->setLow(_this->reference.reset);
  _this->reference.reset->api->setHigh(_this->reference.reset);
  
  return true;
}



/*----------------------------------------
 *  chip_driver_ma104_beginTransfer
 *----------------------------------------*/
bool chip_driver_ma104_beginTransfer(chip_driver_ma104_handle_t* _this){
  //check busy flag, if flag is unset will return false
  if(!(_this->handle.flag & FLAG_TXBUSY))
    return false;
  
  fw_memory_t sendMemory = {
    .ptr = &_this->handle.transferCache[0],
    .size = 10
  };  
  
  //try to transfer data as usart, if successful will return true
  if(chip_driver_ma104_autoTransfer(_this, &sendMemory))
    return true;
  
  //transfer fail, unset busy
  _this->handle.flag &= ~FLAG_TXBUSY;  
  
  return false;
}



/*----------------------------------------
 *  chip_driver_ma104_isHardwareBusy
 *----------------------------------------*/
bool chip_driver_ma104_isHardwareBusy(chip_driver_ma104_handle_t* _this){
  if(!_this->reference.usbok)
    return false;
  
  return _this->reference.usbok->api->getValue(_this->reference.usbok);
}



/*----------------------------------------
 *  chip_driver_ma104_event_setHardwareBusy
 *----------------------------------------*/
bool chip_driver_ma104_event_setHardwareBusy(chip_driver_ma104_handle_t* _this, chip_driver_ma104_event_hardwareBusy_t event){
  _this->config.hardwareBusy = event;
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
  .write              = chip_driver_ma104_write,
  .writeByte          = chip_driver_ma104_writeByte,
  .read               = chip_driver_ma104_read,
  .readByte           = chip_driver_ma104_readByte,
  .isBusyWrite        = chip_driver_ma104_isBusyWrite,
  .isBusyRead         = chip_driver_ma104_isBusyRead,
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
