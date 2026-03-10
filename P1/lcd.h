#ifndef __LCD_h
#define __LCD_h

#include "stm32f4xx_hal.h"

/*    PROTOTIPOS DE FUNCIONES DEL LCD    */
void LCD_reset(void);
void init_pins_lcd(void);
void LCD_wr_data(unsigned char data);
void LCD_wr_cmd(unsigned char cmd);
void LCD_init(void);
void LCD_update(void);
void LCD_clean(void);
void symbolToLocalBuffer(uint8_t line, uint8_t symbol);
void charToLocalBuffer(uint8_t line, uint16_t position, const char *text);

#endif
