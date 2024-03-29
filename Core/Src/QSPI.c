/*
 * QSPI.c
 *
 *  Created on: Dec 22, 2023
 *      Author: tj
 */

#include "QSPI.h"
#include "MEM_Pool.h"
#include "SYS_config.h"

uint8_t OSPI_WriteReadTransaction(OSPI_HandleTypeDef *hospi, unsigned long *params, unsigned long numParams, unsigned long numRead)
{
  OSPI_RegularCmdTypeDef sCommand = {0};
  unsigned long x, i;

  i = 0;
  x = numParams;

  sCommand.OperationType      = HAL_OSPI_OPTYPE_COMMON_CFG;
  sCommand.FlashId            = HAL_OSPI_FLASH_ID_1;

  sCommand.Instruction        = params[0];
  x--;
  i++;

  sCommand.InstructionMode    = HAL_OSPI_INSTRUCTION_1_LINE;
  sCommand.InstructionSize    = HAL_OSPI_INSTRUCTION_8_BITS;
  sCommand.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;

  sCommand.Address			  = params[1];
  sCommand.AddressMode		  = HAL_OSPI_ADDRESS_4_LINES;
  switch (gCFGparams.QSPIaddr)
  {
  case 0 : sCommand.AddressMode = HAL_OSPI_ADDRESS_NONE; break;
  case 1 : sCommand.AddressSize = HAL_OSPI_ADDRESS_8_BITS; break;
  case 2 : sCommand.AddressSize = HAL_OSPI_ADDRESS_16_BITS; break;
  case 3 : sCommand.AddressSize = HAL_OSPI_ADDRESS_24_BITS; break;
  default: sCommand.AddressSize = HAL_OSPI_ADDRESS_32_BITS; break;
  }
  x--;
  i++;

  sCommand.AddressDtrMode     = HAL_OSPI_ADDRESS_DTR_DISABLE;

  sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_4_LINES;
  switch (gCFGparams.QSPIalt)
  {
  case 0 : sCommand.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE; break;
  case 1 : sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_8_BITS; break;
  case 2 : sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_16_BITS; break;
  case 3 : sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_24_BITS; break;
  default: sCommand.AlternateBytesSize = HAL_OSPI_ALTERNATE_BYTES_32_BITS; break;
  }
  if (gCFGparams.QSPIalt)
  {
	  sCommand.AlternateBytes = params[2];
	  x--;
	  i++;
  }

  sCommand.DataMode           = HAL_OSPI_DATA_4_LINES;
  if (numRead)
  {
	  sCommand.NbData         = numRead * 4;
  }
  else
  {
	  sCommand.NbData         = x * 4;
  }

  sCommand.DataDtrMode        = HAL_OSPI_DATA_DTR_DISABLE;
  sCommand.DummyCycles        = 0;
  if (numRead)
  {
	  sCommand.DummyCycles    = gCFGparams.QSPIturn;
  }
  sCommand.DQSMode            = HAL_OSPI_DQS_DISABLE;			//HAL_OSPI_DQS_ENABLE - does not work! - it will not finish
  sCommand.SIOOMode           = HAL_OSPI_SIOO_INST_EVERY_CMD;	//HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD;	//HAL_OSPI_SIOO_INST_EVERY_CMD;

  /**
   * Remark: HAL_QSPI_Command sets all the parameter needed for entire transaction
   * It can be followed by HAL_QSPI_Reveive OR (!) HAL_QSPI_Transmit, but not both!
   * The transaction finishes with one of these calls.
   * So, not possible to write longer and then to read (not intended!)
   */
  if (HAL_OSPI_Command(hospi, &sCommand, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    return -1;
  }

  if (numRead)
  {
	  if (HAL_OSPI_Receive(hospi, (uint8_t *)params, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
	  {
	    return -1;
	  }
  }
  else
  {
	  if (i)
	  {
		  if (HAL_OSPI_Transmit(hospi, (uint8_t *)&params[i], HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		  {
		 	return -1;
		  }
	  }
  }

  return 1;
}

uint8_t QSPI_Transaction(unsigned long *params, unsigned long numParams, unsigned long rdWords)
{
	uint8_t e;
	if (gCFGparams.CfgFlags & 0x1)
		QSPI_ActivatePins();
	e = OSPI_WriteReadTransaction(&hospi1, params, numParams, rdWords);
	if (gCFGparams.CfgFlags & 0x1)
		QSPI_ReleasePins();

	return e;
}

unsigned long QSPI_SetClock(unsigned long div)
{
	extern void MX_OCTOSPI1_Init(void);

	if ( ! div)
		return gCFGparams.QSPIdiv;

	if (div > 255)
		div = 255;		/* max. possible divider */

	gCFGparams.QSPIdiv = div;

	/* initialize QSPI again */
	HAL_OSPI_DeInit(&hospi1);
	MX_OCTOSPI1_Init();

	return div;
}

unsigned long QSPI_ReadChipID(EResultOut out)
{
	unsigned long *params;
	unsigned long cid;

	params = (unsigned long *)MEM_PoolAlloc(MEM_POOL_SEG_SIZE);
	if ( ! params)
		return 0;			/* error */

	/* fill the params with words to read ChipID */
	params[0] = 0xE0;			/* CMD */
	params[1] = 0;				/* ADDR */
	params[2] = 0x00001F;		/* ALT */
	/* HCC header write */
	params[3] = 0x00040580;
	params[4] = 0x400B0FD0;		/* ChipID register address */
	params[5] = 0xAAA00000;

	QSPI_Transaction(params, 6, 0);		//#1 - write

	params[0] = 0xE1;			/* CMD */
	params[1] = 0;				/* ADDR */
	params[2] = 0x00001E;		/* ALT */
	/* noww turn around and read 5 words */

	QSPI_Transaction(params, 0, 5);

	if (gCFGparams.Debug & DBG_VERBOSE)
	{
		int i;
		for (i = 0; i < 5; i++)
			print_log(out, " %08lx", params[i]);
		Generic_Send((uint8_t *)"\r\n", 2, out);
	}

	cid = params[3];

	MEM_PoolFree(params);
	/* take the ChipID from response */
	return cid;
}

void QSPI_DeInit(void)
{
	/* un-initialize all QSPI signals */
	HAL_OSPI_DeInit(&hospi1);
}

void QSPI_ReleasePins(void)
{
    /**OCTOSPI1 GPIO Configuration
    PA2     ------> OCTOSPIM_P1_NCS
    PB0     ------> OCTOSPIM_P1_IO1
    PE12     ------> OCTOSPIM_P1_IO0
    PE14     ------> OCTOSPIM_P1_IO2
    PE15     ------> OCTOSPIM_P1_IO3
    PB10     ------> OCTOSPIM_P1_CLK
    */
#if 0
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2);
#endif

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_10);

    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_15);
}

void QSPI_ActivatePins(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

    /**OCTOSPI1 GPIO Configuration
    PA2     ------> OCTOSPIM_P1_NCS
    PB0     ------> OCTOSPIM_P1_IO1
    PE12     ------> OCTOSPIM_P1_IO0
    PE14     ------> OCTOSPIM_P1_IO2
    PE15     ------> OCTOSPIM_P1_IO3
    PB10     ------> OCTOSPIM_P1_CLK
    PA1      ------> OCTOSPIM_P1_DQS - not working in SDR mode
    */
#if 0
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = gCFGparams.QSPIspeed;		//GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = gCFGparams.QSPIspeed;		//GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = gCFGparams.QSPIspeed;		//GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}
