/* *****************************************************************************************
 *    File Name   :chip_driver_ma104.h
 *    Create Date :2021-04-13
 *    Modufy Date :2021-04-30
 *    Information :
 */
 
#ifndef CHIP_DRIVER_MA104_VERSION
#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

/* *****************************************************************************************
 *    Include
 */ 
#include <stdbool.h>
#include <stdint.h>

#include "fw_usart.h"
#include "fw_io_pin.h"
#include "version.h"



/* *****************************************************************************************
 *    Macro
 */ 

/*----------------------------------------
 *  CHIP_DRIVER_MA104_REQ_FW_USART_VERSION
 *----------------------------------------*/
#define CHIP_DRIVER_MA104_REQ_FW_USART_VERSION VERSION_DEFINEE(1,0,0)
#if VERSION_CHECK_COMPATIBLE(FW_USART_VERSION, CHIP_DRIVER_MA104_REQ_FW_USART_VERSION)
  #if VERSION_CHECK_COMPATIBLE(FW_USART_VERSION, CHIP_DRIVER_MA104_REQ_FW_USART_VERSION) == 2
    #error "FW_USART_VERSION under 1.0.x"
  #else
    #warning "FW_USART_VERSION revision under 1.0.0"
  #endif
#endif



/*----------------------------------------
 *  CHIP_DRIVER_MA104_REQ_FW_IO_PIN_VERSION
 *----------------------------------------*/
#define CHIP_DRIVER_MA104_REQ_FW_IO_PIN_VERSION VERSION_DEFINEE(1,0,0)
#if VERSION_CHECK_COMPATIBLE(FW_IO_PIN_VERSION, CHIP_DRIVER_MA104_REQ_FW_IO_PIN_VERSION)
  #if VERSION_CHECK_COMPATIBLE(FW_IO_PIN_VERSION, CHIP_DRIVER_MA104_REQ_FW_IO_PIN_VERSION) == 2
    #error "FW_IO_PIN_VERSION under 1.0.x"
  #else
    #warning "FW_IO_PIN_VERSION revision under 1.0.0"
  #endif
#endif



/*----------------------------------------
 *  CHIP_DRIVER_MA104_VERSION
 *----------------------------------------*/
#define CHIP_DRIVER_MA104_VERSION VERSION_DEFINEE(1, 0, 0)



/* *****************************************************************************************
 *    Typedef List
 */ 
typedef struct _chip_driver_ma104_handle_t chip_driver_ma104_handle_t;



/* *****************************************************************************************
 *    Typedef Function
 */ 

/*----------------------------------------
 *  chip_driver_ma104_execute_t
 *----------------------------------------*/
typedef void(*chip_driver_ma104_execute_t)(chip_driver_ma104_handle_t* _this, fw_memory_t* data, void* attachment);



/*----------------------------------------
 *  chip_driver_ma104_event_hardwareBusy_t
 *----------------------------------------*/
typedef void(*chip_driver_ma104_event_hardwareBusy_t)(chip_driver_ma104_handle_t* _this);



/* *****************************************************************************************
 *    Struct/Union/Enum
 */ 

/*----------------------------------------
 *  chip_driver_ma104_api_t
 *----------------------------------------*/
struct chip_driver_ma104_api_t{
  bool     (*init)             (chip_driver_ma104_handle_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset);
  bool     (*receiverEnable)   (chip_driver_ma104_handle_t* _this, void* buffer, uint32_t size);
  bool     (*receiverDisable)  (chip_driver_ma104_handle_t* _this);
  bool     (*write)            (chip_driver_ma104_handle_t* _this, fw_memory_t* data, chip_driver_ma104_execute_t execute, void* attachment);
  bool     (*writeByte)        (chip_driver_ma104_handle_t* _this, uint8_t data);
  bool     (*read)             (chip_driver_ma104_handle_t* _this, fw_memory_t* data, chip_driver_ma104_execute_t execute, void* attachment);
  bool     (*readByte)         (chip_driver_ma104_handle_t* _this, uint8_t* buffer);
  bool     (*isBusyWrite)      (chip_driver_ma104_handle_t* _this);
  bool     (*isBusyRead)       (chip_driver_ma104_handle_t* _this);
  bool     (*reset)            (chip_driver_ma104_handle_t* _this);
  bool     (*beginTransfer)    (chip_driver_ma104_handle_t* _this);
  bool     (*isHardwareBusy)   (chip_driver_ma104_handle_t* _this);
  
  struct{
    bool (*setHardwareBusy)(chip_driver_ma104_handle_t* _this, chip_driver_ma104_event_hardwareBusy_t event);
  }event;
};



/*----------------------------------------
 *  chip_friver_ma104_xfer_t
 *----------------------------------------*/
struct chip_friver_ma104_xfer_t{
  fw_memory_t memory;
  uint32_t pointer;
  void* attachment;
  chip_driver_ma104_execute_t execute;
};


/* *****************************************************************************************
 *    Typedef Struct/Union/Enum
 */ 

/*----------------------------------------
 *  chip_driver_ma104_handle_t
 *----------------------------------------*/
typedef struct _chip_driver_ma104_handle_t{ 
  uint32_t enableFlag;
  
  struct{
		
    fw_usart_handle_t*  usart;
    fw_io_pin_handle_t* usbok;
    fw_io_pin_handle_t* reset;
  }reference;
  
  struct{
		
    chip_driver_ma104_event_hardwareBusy_t hardwareBusy;
  }config;  
  
  struct{
		
    uint32_t flag;
    
    struct{
      fw_memory_t memory;
      uint32_t pointer;
      void* attachment;
      chip_driver_ma104_execute_t execute;
    }transfer;
    
    struct{
      fw_memory_t memory;
      void* attachment;
      chip_driver_ma104_execute_t execute;
    }receiver;
		
    uint8_t transferCache[10];
  }handle;

}chip_driver_ma104_handle_t;



/* *****************************************************************************************
 *    Inline Function
 */ 

/* *****************************************************************************************
 *    Extern
 */ 
extern const struct chip_driver_ma104_api_t chip_driver_ma104_api;

extern bool chip_driver_ma104_init(chip_driver_ma104_handle_t* _this, fw_usart_handle_t* usart, fw_io_pin_handle_t* usbok, fw_io_pin_handle_t* reset);
extern bool chip_driver_ma104_write(chip_driver_ma104_handle_t* _this, fw_memory_t* data, chip_driver_ma104_execute_t execute, void* attachment);
extern bool chip_driver_ma104_writeByte(chip_driver_ma104_handle_t* _this, uint8_t data);
extern bool chip_driver_ma104_read(chip_driver_ma104_handle_t* _this, fw_memory_t* data, chip_driver_ma104_execute_t execute, void* attachment);
extern bool chip_driver_ma104_readByte(chip_driver_ma104_handle_t* _this, uint8_t* buffer);
extern bool chip_driver_ma104_isBusyWrite(chip_driver_ma104_handle_t* _this);
extern bool chip_driver_ma104_isBusyRead(chip_driver_ma104_handle_t* _this);
extern bool chip_driver_ma104_beginTransfer(chip_driver_ma104_handle_t* _this);
extern bool chip_driver_ma104_isHardwareBusy(chip_driver_ma104_handle_t* _this);
extern bool chip_driver_ma104_event_setHardwareBusy(chip_driver_ma104_handle_t* _this, chip_driver_ma104_event_hardwareBusy_t event);



#ifdef __cplusplus
}
#endif //__cplusplus
#endif
/* *****************************************************************************************
 *    End of file
 */ 
