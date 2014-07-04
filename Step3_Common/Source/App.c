/*****************************************************************************/
/*!
 *\MODULE              Application
 *
 *\COMPONENT           $HeadURL: http://svn/apps/Application_Notes/JN-AN-1085-JenNet-Tutorial/Tags/Release_1v4-Public/Step3_Common/Source/App.c $
 *
 *\VERSION			   $Revision: 5394 $
 *
 *\REVISION            $Id: App.c 5394 2010-02-15 14:15:22Z mlook $
 *
 *\DATED               $Date: 2010-02-15 14:15:22 +0000 (Mon, 15 Feb 2010) $
 *
 *\AUTHOR              $Author: mlook $
 *
 *\DESCRIPTION         Slimme meter v0.5
 */


/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <Jenie.h>
#include <JPI.h>		/* Jenie Peripheral Interface {v2} */
#include <AppHardwareApi.h>
#include <Printf.h>		/* Basic Printf to UART0-19200-8-NP-1 {v2} */
#include <LedControl.h>	/* Led Interface {v2} */
#include <LcdDriver.h>
#include <Button.h>		/* Button Interface {v3} */
#include "App.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define u8Uart						0		// UART0 or UART1
#define LED1						0
#define LED2						1
#define p1_Message_Size				64		// in bytes
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void LCD_WRITE(char *pString, int Column, int Row, bool Update, bool Clear);
PUBLIC bool_t bUartIO_GetChar (char *pcGetChar); 	// Read CHAR from UART
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool_t    	bNetworkUp;							 /* Network up {v2} */
PRIVATE uint8   	au8Led[2];							 /* Led states {v2} */
PRIVATE uint16    	u8Tick;							     /* Ticker {v2} */
PRIVATE uint64  	u64Parent; 							 /* Parent address {v3} */
PRIVATE uint64  	u64Last;							 /* Last address to send to us {v3} */
PRIVATE uint8 		y=0;
bool_t				TickFlag;
PRIVATE	int			cUART_BUFFER;
uint8 				UART_BUFFER[p1_Message_Size];		// create P1 buffer
/****************************************************************************
 *
 * NAME: vApp_CbInit
 *
 * DESCRIPTION:
 * Application initialisation
 *
 * RETURNS:
 * Nothing
 *
 ****************************************************************************/

PUBLIC void vApp_CbInit(bool_t bWarmStart)
{
    /* Is this a cold start ? */
    if (! bWarmStart)
    {
        /* Network won't be up */
        bNetworkUp = FALSE;
    }

    /* Initialise LEDs and buttons {v3} */
    vLedInitRfd();
    vButtonInitRfd();

    /* Turn off LEDs */
    au8Led[0] = 0;
    au8Led[1] = 0;

	vLcdResetDefault(); // Initialise the LCD panel using default settings for bias and gain
	vLcdClear(); 		// Clear the shadow memory of any text or graphics

	/* initialize UART in order to communicate with P1*/
	vAHI_UartEnable(u8Uart);
/* Reset UART */
	vAHI_UartReset(u8Uart, TRUE, TRUE);
	vAHI_UartReset(u8Uart, FALSE, FALSE);
/* Set baud rate (1150000) */
	vAHI_UartSetClockDivisor(u8Uart, E_AHI_UART_RATE_115200);
/* Set control (no parity, 8 bit word length, 1 stop bit (8N1)) */
	vAHI_UartSetControl(u8Uart,
    E_AHI_UART_EVEN_PARITY,
    E_AHI_UART_PARITY_DISABLE,
    E_AHI_UART_WORD_LEN_8,
    E_AHI_UART_1_STOP_BIT,
    E_AHI_UART_RTS_HIGH);
}

/****************************************************************************
 *
 * NAME: vApp_CbMain
 *
 * DESCRIPTION:
 * Main user routine. This is called by the Basic Operating System (BOS)
 * at regular intervals.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vApp_CbMain(void)
{
	uint64 				u64Address;		/* Address to send data to {v3} */
	teJenieStatusCode 	eStatus; 		/* Jenie status code {v3} */
	int length=(sizeof(UART_BUFFER)/sizeof(char));

    /* Regular watchdog reset */
    #ifdef WATCHDOG_ENABLED
       vAHI_WatchdogRestart();
    #endif


	if(bUartIO_GetChar(&UART_BUFFER[cUART_BUFFER]))// Get data from UART1 > UART_CHARACTER if there's a byte in UART1
			{
			if(cUART_BUFFER++==(length-2)/* || UART_BUFFER[cUART_BUFFER]=='!'*/) // -2 because the last element of the buffer array may not be addressed. Search for fix!!!
				{


				/* Choose an address to send data to */
				if (u64Last != 0ULL) u64Address = u64Last;
				else                 u64Address = u64Parent;

				vPrintf("Buffer %s\n",UART_BUFFER); // output to UART for testing
					/* Initialise data for transmission */

				eStatus = eJenie_SendData(u64Address, UART_BUFFER, length, TXOPTION_ACKREQ);

				if (E_JENIE_DEFERRED == eStatus)
					{
					/* Light LED1 */
					vLedControl(LED2,1);
					}
				cUART_BUFFER=0; // reset counter when buffer is send
				}
			}

	/* Network is down ? */
	if (! bNetworkUp)
	{
		/* Flash LED0 quickly while we wait for the network to come up */
		au8Led[0] = 0x02;
	}

	/* Network up and permit join is on {v3} ? */
	else if (bJenie_GetPermitJoin())
	{
		/* Flash LED0 quickish while we are allowing joining */
		au8Led[0] = 0x04;
	}
	/* Led has been left flashing ? */
	else if (au8Led[0] != 0 && au8Led[0] != 0xFF)
	{
		/* Turn off LED */
		au8Led[0] = 0x00;
	}

}

/****************************************************************************
 *
 * NAME: vApp_StackMgmtEvent
 *
 * DESCRIPTION:
 * Used to receive stack management events
 *
 * PARAMETERS:      Name            		RW  Usage
 *					*psStackMgmtEvent		R	Pointer to event structure
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vApp_CbStackMgmtEvent(teEventType eEventType, void *pvEventPrim)
{
	teJenieStatusCode eStatus; /* Jenie status code {v3} */

	/* Which event occurred ? */
    switch (eEventType)
    {
		/* Indicates stack is up and running */
		case E_JENIE_NETWORK_UP:
		{
			/* Get pointer to correct primitive structure {v3} */
			tsNwkStartUp *psNwkStartUp = (tsNwkStartUp *) pvEventPrim;
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(NETWORK_UP, %x:%x, %x:%x, %d, %x, %d)\n",
				(uint32)(psNwkStartUp->u64ParentAddress >> 32),
				(uint32)(psNwkStartUp->u64ParentAddress &  0xFFFFFFFF),
				(uint32)(psNwkStartUp->u64LocalAddress  >> 32),
				(uint32)(psNwkStartUp->u64LocalAddress &  0xFFFFFFFF),
				psNwkStartUp->u16Depth,
				psNwkStartUp->u16PanID,
    			psNwkStartUp->u8Channel);

			/* Network is now up */
			bNetworkUp = TRUE;
			/* Note our parent address {v3} */
			u64Parent = psNwkStartUp->u64ParentAddress;

			/* Turn on permit joining {v3} */
			eStatus = eJenie_SetPermitJoin(TRUE);
			/* Output to UART */
			vPrintf("eJenie_SetPermitJoin(%d) = %d\n",
				bJenie_GetPermitJoin(),
				eStatus);
		}   break;

		/* Indicates stack has reset */
		case E_JENIE_STACK_RESET:
		{
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(STACK_RESET)\n");

			/* Network is now down */
			bNetworkUp = FALSE;
			/* Clear our parent address {v3} */
			u64Parent = 0ULL;

			/* Turn off permit joining {v3} */
			eStatus = eJenie_SetPermitJoin(FALSE);
			/* Output to UART */
			vPrintf("eJenie_SetPermitJoin(%d) = %d\n",
				bJenie_GetPermitJoin(),
				eStatus);
		}   break;

		/* Indicates child has joined {v3} */
		case E_JENIE_CHILD_JOINED:
		{
			/* Get pointer to correct primitive structure */
			tsChildJoined *psChildJoined = (tsChildJoined *) pvEventPrim;
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(CHILD_JOINED, %x:%x)\n",
				(uint32)(psChildJoined->u64SrcAddress >> 32),
				(uint32)(psChildJoined->u64SrcAddress &  0xFFFFFFFF));

			/* Note our latest child */
			u64Last = psChildJoined->u64SrcAddress;

			/* Turn off permit joining */
			eStatus = eJenie_SetPermitJoin(FALSE);
			/* Output to UART */
			vPrintf("eJenie_SetPermitJoin(%d) = %d\n",
				bJenie_GetPermitJoin(),
				eStatus);
		}   break;

		/* Indicates child has left {v3} */
		case E_JENIE_CHILD_LEAVE:
		{
			/* Get pointer to correct primitive structure */
			tsChildLeave *psChildLeave = (tsChildLeave *) pvEventPrim;
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(CHILD_LEAVE, %x:%x)\n",
				(uint32)(psChildLeave->u64SrcAddress >> 32),
				(uint32)(psChildLeave->u64SrcAddress &  0xFFFFFFFF));

			/* Was that the last device to send us something ? */
			if (u64Last == psChildLeave->u64SrcAddress)
			{
				/* Clear the last address */
				u64Last = 0ULL;
			}

			/* Turn on permit joining */
			eStatus = eJenie_SetPermitJoin(TRUE);
			/* Output to UART */
			vPrintf("eJenie_SetPermitJoin(%d) = %d\n",
				bJenie_GetPermitJoin(),
				eStatus);
		}   break;

		default:
		{
			/* Unknown event type */
		}   break;
    }
}

/****************************************************************************
 *
 * NAME: vApp_CbStackDataEvent
 *
 * DESCRIPTION:
 * Used to receive stack data events
 *
 * PARAMETERS:      Name            		RW  Usage
 *					*psStackDataEvent		R	Pointer to data structure
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vApp_CbStackDataEvent(teEventType eEventType, void *pvEventPrim)
{

	/* Which event occurred ? */
    switch(eEventType)
    {
		/* Incoming data {v3} */
		case E_JENIE_DATA:
		{
			/* Get pointer to correct primitive structure */
			tsData *psData = (tsData *) pvEventPrim;
			/* Output to UART */
			vPrintf("vApp_CbStackDataEventDATA, from= %x:%x, Flag= %x, length= %d, string= %s\n",
				(uint32)(psData->u64SrcAddress >> 32),
				(uint32)(psData->u64SrcAddress &  0xFFFFFFFF),
				 psData->u8MsgFlags,
				 psData->u16Length,
				 (psData->pau8Data[psData->u16Length-1] == 0) ? (char *) psData->pau8Data : "");

			/* Display DATA on LCD */
			LCD_WRITE("",0,0,1,1); // Clear Display
			LCD_WRITE(psData->pau8Data,0,y,1,0); // Display buffer on lcd, refresh, no clear
				if (y++==6) // 6 rows displayed?
					{
					y=0;
					LCD_WRITE("",0,y,0,1); // Screen full? Clear LCD
					}
		}   break;

		/* Incoming data ack {v3} */
		case E_JENIE_DATA_ACK:
		{
			/* Get pointer to correct primitive structure */
			tsDataAck *psDataAck = (tsDataAck *) pvEventPrim;
			/* Output to UART */
			vPrintf("vApp_CbStackDataEvent(DATA_ACK, %x:%x)\n",
				(uint32)(psDataAck->u64SrcAddress >> 32),
				(uint32)(psDataAck->u64SrcAddress &  0xFFFFFFFF));

			/* Turn off LED1 */
			vLedControl(LED2,0);
		}   break;

		default:
		{
			/* Unknown event type */
		}	break;
    }
}

/****************************************************************************
 *
 * NAME: vApp_CbHwEvent
 *
 * DESCRIPTION:
 * Adds events to the hardware event queue.
 *
 * PARAMETERS:      Name            RW  Usage
 *                  u32Device       R   Peripheral responsible for interrupt e.g DIO
 *					u32ItemBitmap	R   Source of interrupt e.g. DIO bit map
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vApp_CbHwEvent(uint32 u32DeviceId,uint32 u32ItemBitmap)
{
    uint8   		 	u8Led;			/* LED loop variable */
    /* Is this the tick timer ? */
	if (u32DeviceId == E_JPI_DEVICE_TICK_TIMER)
    {
        /* Increment our ticker */
        u8Tick++;
        if (u8Tick==0)
			{
        	TickFlag=TRUE;
			}
        else
        	TickFlag=FALSE;

        /* Loop through LEDs */
        for (u8Led = 0; u8Led < 2; u8Led++)
        {
            /* Set LED according to status */
            if (au8Led[u8Led] == 0 || au8Led[u8Led] == 0xFF) vLedControl(u8Led, au8Led[u8Led]);
            else				   							 vLedControl(u8Led, au8Led[u8Led] & u8Tick);
        }
    }
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
/* LCD write SaKa */
PRIVATE void LCD_WRITE(char *pString, int Column, int Row, bool Update, bool Clear)
{
vLcdWriteText(pString, Row, Column);
if (Update)
	vLcdRefreshAll(); // Update LCD with info from shadow memory
if (Clear)
	vLcdClear(); 		// Clean sheet
}
/* Read CHAR from UART */
PUBLIC bool_t bUartIO_GetChar (char *pcGetChar) /**< Character received pointer */
{
    bool_t bReturn = FALSE;                    /* Removed from queue */

    /* Is there data in the UART ? */
    if (u8AHI_UartReadLineStatus(u8Uart) & E_AHI_UART_LS_DR)
    {
        /* Read direct from UART */
        *pcGetChar = (char) u8AHI_UartReadData(u8Uart);
        /* Note we read a byte */
        bReturn = TRUE;
    }

    return bReturn;
}

