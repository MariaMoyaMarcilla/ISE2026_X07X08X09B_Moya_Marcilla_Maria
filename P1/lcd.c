#include "LCD.h"
#include <Driver_SPI.h>
#include "Arial12x12.h"
#include <stdio.h>
#include "cmsis_os2.h"


extern ARM_DRIVER_SPI Driver_SPI1;
ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;
ARM_SPI_STATUS stat;

char buffer [512];
uint16_t positionL1 = 0;
uint16_t positionL2 = 0;

/*-------------------------------------------------------------------------------------------------------------------------
Función LCD_RESET: Iniciliza la interfaz SPI
--------------------------------------------------------------------------------------------------------------------------*/

void LCD_reset(void){
  /*Inicialización y configuración del driver SPI para gestionar el LCD*/
  SPIdrv->Initialize(NULL);
  SPIdrv->PowerControl(ARM_POWER_FULL);
  SPIdrv->Control(ARM_SPI_MODE_MASTER|ARM_SPI_CPOL1_CPHA1|ARM_SPI_MSB_LSB|ARM_SPI_DATA_BITS(8), 20000000);
  SPIdrv->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
  
  /*Configuración de los pines RESET, A0 Y CS*/
  init_pins_lcd();
  
  /*Generación de la señal de reset*/
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
  osDelay(1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
  osDelay(1);
}

/*-------------------------------------------------------------------------------------------------------------------------
Configuración de los pines para LCD_RESET, LCD_CS_N y LCD_A0
--------------------------------------------------------------------------------------------------------------------------*/
void init_pins_lcd (void){
  GPIO_InitTypeDef GPIO_InitStruct;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  
  /*Inicialización del pin A6 - LCD_RESET*/
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /*Inicialización del pin D14 - LCD_CS_N*/
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  
  /*Inicialización del pin F13 - LCD_A0*/
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

/*-------------------------------------------------------------------------------------------------------------------------
LCD_wr_data: Función que escribe un dato en el LCD
--------------------------------------------------------------------------------------------------------------------------*/

void LCD_wr_data(unsigned char data){
 // Seleccionar CS = 0;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
 // Seleccionar A0 = 1;
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET);
 // Escribir un dato (data) usando la función SPIDrv->Send(…);
  SPIdrv->Send(&data, sizeof (data));
 // Esperar a que se libere el bus SPI;
  do{
    stat = SPIdrv->GetStatus();
  } while(stat.busy);
 // Seleccionar CS = 1;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
}

/*-------------------------------------------------------------------------------------------------------------------------
LCD_wr_cmd: Función que escribe un comando en el LCD
--------------------------------------------------------------------------------------------------------------------------*/

void LCD_wr_cmd(unsigned char cmd){
 // Seleccionar CS = 0;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
 // Seleccionar A0 = 0;
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_RESET);
 // Escribir un dato (cmd) usando la función SPIDrv->Send(…);
  SPIdrv->Send(&cmd, sizeof (cmd));
 // Esperar a que se libere el bus SPI;
  do{
    stat = SPIdrv->GetStatus();
  } while(stat.busy);
 // Seleccionar CS = 1;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
}

/*-------------------------------------------------------------------------------------------------------------------------
LCD_init
--------------------------------------------------------------------------------------------------------------------------*/
void LCD_init(void){
  LCD_wr_cmd(0xAE);
  LCD_wr_cmd(0xA2);
  LCD_wr_cmd(0xA0);
  LCD_wr_cmd(0xC8);
  LCD_wr_cmd(0x22);
  LCD_wr_cmd(0x2F);
  LCD_wr_cmd(0x40);
  LCD_wr_cmd(0xAF);
  LCD_wr_cmd(0x81);
  LCD_wr_cmd(0x17);
  LCD_wr_cmd(0xA4);
  LCD_wr_cmd(0xA6);
}

/*-------------------------------------------------------------------------------------------------------------------------
LCD_update: Actualiza el buffer de datos
--------------------------------------------------------------------------------------------------------------------------*/

void LCD_update(void)
{
 int i;
 LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
 LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
 LCD_wr_cmd(0xB0); // Página 0

 for(i=0;i<128;i++){
 LCD_wr_data(buffer[i]);
 }

 LCD_wr_cmd(0x00); // 4 bits de la parte baja de la dirección a 0
 LCD_wr_cmd(0x10); // 4 bits de la parte alta de la dirección a 0
 LCD_wr_cmd(0xB1); // Página 1

 for(i=128;i<256;i++){
 LCD_wr_data(buffer[i]);
 }

 LCD_wr_cmd(0x00);
 LCD_wr_cmd(0x10);
 LCD_wr_cmd(0xB2); //Página 2
 for(i=256;i<384;i++){
 LCD_wr_data(buffer[i]);
 }

 LCD_wr_cmd(0x00);
 LCD_wr_cmd(0x10);
 LCD_wr_cmd(0xB3); // Pagina 3


 for(i=384;i<512;i++){
 LCD_wr_data(buffer[i]);
 }
}

/*-------------------------------------------------------------------------------------------------------------------------
Función que permite borrar el contenido del buffer y poner los cursores a 0
--------------------------------------------------------------------------------------------------------------------------*/

void LCD_clean(void){
  int i;
  for(i=0;i<512;i++){
    buffer[i] = 0x00;
  }
}

/*-------------------------------------------------------------------------------------------------------------------------
Función que permite la escritura de un caracter en la linea 1 y 2 del LCD
--------------------------------------------------------------------------------------------------------------------------*/

void symbolToLocalBuffer(uint8_t line, uint8_t symbol){
  uint8_t i, value1, value2;
  uint16_t offset = 0;
  uint8_t char_width;
  
  offset = 25*(symbol - ' ');
  
  char_width = Arial12x12[offset]*2;
  
  if(line == 1){
    for(i=0; i<char_width; i++){
      value1 = Arial12x12[offset+i*2+1];
      value2 = Arial12x12[offset+i*2+2];
      
      if((positionL1 + i) < 128 ){
        buffer[i+positionL1] = value1; //pagina 0
        buffer[i+128+positionL1] = value2; // pagina1
      }
    }
    positionL1 = positionL1+Arial12x12[offset];
  }else if(line == 2){
    for(i=0; i<char_width; i++){
      value1 = Arial12x12[offset+i*2+1];
      value2 = Arial12x12[offset+i*2+2];
      
      if((positionL2 + i) < 386 ){
        buffer[i+257+positionL2] = value1; //pagina 3
        buffer[i+385+positionL2] = value2; // pagina 4
      }
    }
    positionL2 = positionL2+Arial12x12[offset];
  }
}

/*-------------------------------------------------------------------------------------------------------------------------
Función que permite escribir una cadena de caracteres en la linea seleccionada
--------------------------------------------------------------------------------------------------------------------------*/

void charToLocalBuffer(uint8_t line, uint16_t position, const char *text){
  if(line == 1){
    positionL1 = position;
  } else if(line == 2){
    positionL2 = position;
  }
  
  for(int k = 0; text[k] != '\0'; k++){
    symbolToLocalBuffer(line, (uint8_t)text[k]);
  }
}






