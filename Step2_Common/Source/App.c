/*****************************************************************************/
/*!
 *\MODULE              Application
 *
 *\COMPONENT           $HeadURL: http://svn/apps/Application_Notes/JN-AN-1085-JenNet-Tutorial/Tags/Release_1v4-Public/Step2_Common/Source/App.c $
 *
 *\VERSION			   $Revision: 5394 $
 *
 *\REVISION            $Id: App.c 5394 2010-02-15 14:15:22Z mlook $
 *
 *\DATED               $Date: 2010-02-15 14:15:22 +0000 (Mon, 15 Feb 2010) $
 *
 *\AUTHOR              $Author: mlook $
 *
 *\DESCRIPTION         Appliication - implementation.
 */
/*****************************************************************************
 *
 * This software is owned by Jennic and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on Jennic products. You, and any third parties must reproduce
 * the copyright and warranty notice and any other legend of ownership on each
 * copy or partial copy of the software.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
 * BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
 * INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
 *
 * Copyright Jennic Ltd 2010. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <Jenie.h>
#include <JPI.h>			/* Jenie Peripheral Interface {v2} */
#include <AppHardwareApi.h> /* Hardware peripherals not in JPI.h {v2} */
#include <Printf.h>			/* Basic Printf to UART0-19200-8-NP-1 {v2} */
#include <LedControl.h>		/* Led Interface {v2} */

#include "App.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE bool_t    bNetworkUp;							 /* Network up {v2} */
PRIVATE uint8   au8Led[2];								 /* Led states {v2} */
PRIVATE uint8    u8Tick;							         /* Ticker {v2} */

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

    /* Initialise LEDs */
    vLedInitRfd();

    /* Turn off LEDs */
    au8Led[0] = 0;
    au8Led[1] = 0;
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
    /* Regular watchdog reset */
    #ifdef WATCHDOG_ENABLED
       vAHI_WatchdogRestart();
    #endif

	/* Network is down ? */
	if (! bNetworkUp)
	{
		/* Flash LED0 quickly while we wait for the network to come up */
		au8Led[0] = 0x02;
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
	/* Which event occurred ? */
    switch (eEventType)
    {
		/* Indicates stack is up and running */
		case E_JENIE_NETWORK_UP:
		{
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(NETWORK_UP)\n");
			/* Network is now up */
			bNetworkUp = TRUE;
		}   break;

		/* Indicates stack has reset */
		case E_JENIE_STACK_RESET:
		{
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(STACK_RESET)\n");
			/* Network is now down */
			bNetworkUp = FALSE;
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
    uint8   u8Led;			/* LED loop variable */

    /* Is this the tick timer ? */
	if (u32DeviceId == E_JPI_DEVICE_TICK_TIMER)
    {
        /* Increment our ticker */
        u8Tick++;

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
