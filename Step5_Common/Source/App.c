/*****************************************************************************/
/*!
 *\MODULE              Application
 *
 *\COMPONENT           $HeadURL: http://svn/apps/Application_Notes/JN-AN-1085-JenNet-Tutorial/Tags/Release_1v4-Public/Step5_Common/Source/App.c $
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
#include <AppHardwareApi.h>
#include <Jenie.h>
#include <JPI.h>		/* Jenie Peripheral Interface {v2} */
#include <Printf.h>		/* Basic Printf to UART0-19200-8-NP-1 {v2} */
#include <LedControl.h>	/* Led Interface {v2} */
#include <Button.h>		/* Button Interface {v3} */

#include "App.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define BUTTON_P_MASK (BUTTON_3_MASK << 1)  /* Mask for program button {v3} */
#define TIMEOUT_LONG_PRESS  200  /* Long button press timeout (in 10ms) {v4} */
#define TIMEOUT_IDLE_ASLEEP	   2	/* Idle timeout when sleeping (in 10ms) */
#define TIMEOUT_IDLE_AWAKE   500	   /* Idle timeout when awake (in 10ms) */
#define PERIOD_SLEEP 		5000					/* Sleep period (in ms) */

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
PRIVATE uint8    u8Button; 							   /* Button state {v3} */
PRIVATE uint64  u64Parent; 							 /* Parent address {v3} */
PRIVATE uint64  u64Last;				 /* Last address to send to us {v3} */
PRIVATE uint16 au16ButtonTimer[2];		   /* Timer for button presses {v4} */
PRIVATE uint32  u32Reg;							/* Registered services {v4} */
PRIVATE bool_t    bReg;			   /* Successfully registered services {v4} */
PRIVATE bool_t    bBound;							 /* Bound services {v4} */
PRIVATE bool_t    bSentService;				   /* Data sent to service {v4} */
PRIVATE bool_t    bEndDevice;			    /* Device is an end device {v5} */
PRIVATE bool_t    bSleep;		     /* End device is in sleeping mode {v5} */
PRIVATE uint16  u16IdleTimer;	     		     /* Timer for idleness {v5} */

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

PUBLIC void vApp_CbInit(bool_t bWarmStart,
						bool_t bInitEndDevice) /* Initialising end device {v5} */
{
    /* Is this a cold start ? */
    if (! bWarmStart)
    {
        /* Network won't be up */
        bNetworkUp = FALSE;
        /* We are not in sleep mode {v5} */
        bSleep = FALSE;
    }

    /* Initialise LEDs and buttons {v3} */
    vLedInitRfd();
    vButtonInitRfd();

    /* Reset button timers {v4} */
    au16ButtonTimer[0] = 0;
    au16ButtonTimer[1] = 0;

    /* Turn off LEDs */
    au8Led[0] = 0;
    au8Led[1] = 0;

    /* Don't run idle timer {v5} */
    u16IdleTimer = 0;

    /* Note if we are an end device {v5} */
    bEndDevice = bInitEndDevice;

	/* Is this an end device {v5} ? */
	if (bEndDevice)
	{
		/* Warm start and the network is up ? */
		if (bWarmStart && bNetworkUp)
		{
			/* Turn on LEDs indicating we've woken up */
			au8Led[0] = 0xFF;
			au8Led[1] = 0xFF;
			/* Have we woken because of a button press ? */
			if (u8ButtonReadRfd())
			{
				/* Make sure we are not in sleep mode */
				bSleep = FALSE;
			}
			/* Set idle timer dependent upon sleep mode */
			if (bSleep) u16IdleTimer = TIMEOUT_IDLE_ASLEEP;
			else		u16IdleTimer = TIMEOUT_IDLE_AWAKE;
		}
    	/* Set interrupts on buttons to wake us up */
		vJPI_DioWake(BUTTON_ALL_MASK_RFD_PIN, 0, 0, BUTTON_ALL_MASK_RFD_PIN);
		/* Set sleep period */
		eJenie_SetSleepPeriod(PERIOD_SLEEP);
    }
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
	/* Network up and permit join is on {v3} ? */
	else if (bJenie_GetPermitJoin())
	{
		/* Flash LED0 quickish while we are allowing joining */
		au8Led[0] = 0x04;
	}
	/* Network up and permit join off and services registered {v4} */
	else if (bReg)
	{
		/* Flash LED0 slowish while we have the service registered */
		au8Led[0] = 0x08;
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
				(uint32)(psNwkStartUp->u64LocalAddress  &  0xFFFFFFFF),
				psNwkStartUp->u16Depth,
				psNwkStartUp->u16PanID,
    			psNwkStartUp->u8Channel);

			/* Network is now up */
			bNetworkUp = TRUE;
			/* Note our parent address {v3} */
			u64Parent = psNwkStartUp->u64ParentAddress;

			/* End device ? */
			if (bEndDevice)
			{
				/* Set idle timer dependent upon sleep mode */
				if (bSleep) u16IdleTimer = TIMEOUT_IDLE_ASLEEP;
				else		u16IdleTimer = TIMEOUT_IDLE_AWAKE;
			}

			/* Turn on permit joining {v3} (for coord and routers) {v5} */
			eStatus = eJenie_SetPermitJoin(! bEndDevice);
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

			/* End device - stop idle timer running */
			if (bEndDevice) u16IdleTimer = 0;

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

		/* Service registered response {v4} */
		case E_JENIE_REG_SVC_RSP:
		{
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(REG_SVC_RSP)\n");

			/* Note if we have services registered or not */
			if (u32Reg == 0) bReg = FALSE;
			else 			 bReg = TRUE;
		}   break;

		/* Service request response {v4} */
		case E_JENIE_SVC_REQ_RSP:
		{
			/* Get pointer to correct primitive structure */
			tsSvcReqRsp *psSvcReqRsp = (tsSvcReqRsp *) pvEventPrim;
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(SVC_REQ_RSP, %x:%x, %x)\n",
				(uint32)(psSvcReqRsp->u64SrcAddress >> 32),
				(uint32)(psSvcReqRsp->u64SrcAddress &  0xFFFFFFFF),
				psSvcReqRsp->u32Services);

			/* Does this device support the service we requested ? */
			if (psSvcReqRsp->u32Services & SERVICE_MASK)
			{
				/* Try to bind the service */
				eStatus = eJenie_BindService(SERVICE_NUMBER, psSvcReqRsp->u64SrcAddress, SERVICE_NUMBER);
				/* Output to UART */
				vPrintf("eJenie_BindService(%d, %x:%x, %d) = %d\n",
					SERVICE_NUMBER,
					(uint32)(psSvcReqRsp->u64SrcAddress >> 32),
					(uint32)(psSvcReqRsp->u64SrcAddress &  0xFFFFFFFF),
					SERVICE_NUMBER,
					eStatus);

				/* Can we bind to the service ? */
				if (E_JENIE_SUCCESS == eStatus)
				{
					/* Note we have bound */
					bBound = TRUE;
					/* Turn off LED 1 */
					au8Led[1] = 0;
				}
			}
		}   break;

		/* Packet sent {v4} */
		case E_JENIE_PACKET_SENT:
		{
			/* Output to UART */
			vPrintf("vApp_CbStackMgmtEvent(PACKET_SENT)\n");

			/* Have we just sent to a service ? */
			if (bSentService)
			{
				/* Turn off LED1 */
				au8Led[1] = 0;
				/* Send to service is no longer pending */
				bSentService = FALSE;
			}
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
			vPrintf("vApp_CbStackDataEvent(DATA, %x:%x, %x, %d, '%s')\n",
				(uint32)(psData->u64SrcAddress >> 32),
				(uint32)(psData->u64SrcAddress &  0xFFFFFFFF),
				 psData->u8MsgFlags,
				 psData->u16Length,
				(psData->pau8Data[psData->u16Length-1] == 0) ? (char *) psData->pau8Data : "");

			/* Is this a button 0 message ? */
			if (psData->u16Length   ==  3  &&
				psData->pau8Data[0] == 'B' &&
				psData->pau8Data[1] == '0')
			{
				/* Remember who sent this data to us */
				u64Last = psData->u64SrcAddress;
				/* Toggle LED0 */
				if      (au8Led[0] == 0)    au8Led[0] = 0xFF;
				else if (au8Led[0] == 0xFF) au8Led[0] = 0;
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

			/* Turn off LED0 */
			au8Led[0] = 0;
		}   break;

		/* Incoming data to service {v4} */
		case E_JENIE_DATA_TO_SERVICE:
		{
			/* Get pointer to correct primitive structure */
			tsDataToService *psDataToService = (tsDataToService *) pvEventPrim;
			/* Output to UART */
			vPrintf("vApp_CbStackDataEvent(DATA_TO_SERVICE, %x:%x, %d, %d, %x, %d, '%s')\n",
				(uint32)(psDataToService->u64SrcAddress >> 32),
				(uint32)(psDataToService->u64SrcAddress &  0xFFFFFFFF),
				 psDataToService->u8SrcService,
				 psDataToService->u8DestService,
				 psDataToService->u8MsgFlags,
				 psDataToService->u16Length,
				(psDataToService->pau8Data[psDataToService->u16Length-1] == 0) ? (char *) psDataToService->pau8Data : "");

			/* Is this to our service ? */
			if (psDataToService->u8DestService == SERVICE_NUMBER)
			{
				/* Is this a button 1 message ? */
				if (psDataToService->u16Length   ==  3  &&
					psDataToService->pau8Data[0] == 'B' &&
					psDataToService->pau8Data[1] == '1')
				{
					/* Toggle LED1 */
					if      (au8Led[1] == 0)    au8Led[1] = 0xFF;
					else if (au8Led[1] == 0xFF) au8Led[1] = 0;
				}
			}
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
    uint8   		 u8Led;			/* LED loop variable */
    uint8   		 u8ButtonRead; 	/* New button reading {v3} */
	uint64 			u64Address;		/* Address to send data to {v3} */
	teJenieStatusCode eStatus; 		/* Jenie status code {v3} */

    /* Is this the tick timer ? */
	if (u32DeviceId == E_JPI_DEVICE_TICK_TIMER)
    {
        /* Increment our ticker */
        u8Tick++;

		/* Is the network up {v3} ? */
		if (bNetworkUp)
		{
			/* Read standard buttons */
			u8ButtonRead = u8ButtonReadRfd();
			/* If the SPI bus is in use - reuse the last value from the program button */
			if      (bJPI_SpiPollBusy())                u8ButtonRead |= (u8Button & BUTTON_P_MASK);
			/* SPI bus not in use and program button is pressed - set mask for program button UNDOCUMENTED */
			else if ((u8JPI_PowerStatus() & 0x10) == 0) u8ButtonRead |= BUTTON_P_MASK;

			/* Is button 0 down {v4}? */
			if (u8ButtonRead & BUTTON_0_MASK)
			{
				/* Are we just starting the timer - but allow it to be on a short time for a waking end device {v5} ? */
				if (au16ButtonTimer[0] == 2)
				{
					/* If LED0 is permamently on - turn it off so we can see feedback on the LED */
					if (au8Led[0] == 0xFF) au8Led[0] = 0;
				}
				/* Increment the button timer */
				au16ButtonTimer[0]++;
				/* Has the button been down a long time ? */
				if (au16ButtonTimer[0] == TIMEOUT_LONG_PRESS)
				{
					/* Toggle registered services */
					if (u32Reg == 0) u32Reg = SERVICE_MASK;
					else			 u32Reg = 0;

					/* Try to register services */
					eStatus = eJenie_RegisterServices(u32Reg);
					/* Output to UART */
					vPrintf("eJenie_RegisterServices(%x) = %d\n", u32Reg, eStatus);

					/* Did they register immediately ? */
					if (E_JENIE_SUCCESS == eStatus)
					{
						/* Note if we have services registered or not */
						if (u32Reg == 0) bReg = FALSE;
						else 			 bReg = TRUE;
					}
				}
			}

			/* Is button 1 down {v4} ? */
			if (u8ButtonRead & BUTTON_1_MASK)
			{
				/* Are we just starting the timer - but allow it to be on a short time for a waking end device {v5}  ? */
				if (au16ButtonTimer[1] == 2)
				{
					/* If LED0 is permamently on - turn it off so we can see feedback on the LED */
					if (au8Led[1] == 0xFF) au8Led[1] = 0;
				}
				/* Increment the button timer */
				au16ButtonTimer[1]++;
				/* Has the button been down a long time ? */
				if (au16ButtonTimer[1] == TIMEOUT_LONG_PRESS)
				{
					/* Try to request the services */
					eStatus = eJenie_RequestServices(SERVICE_MASK, FALSE);
					/* Output to UART */
					vPrintf("eJenie_RequestServices(%x, FALSE) = %d\n", SERVICE_MASK, eStatus);

					/* Error free ? */
					if (E_JENIE_DEFERRED == eStatus)
					{
						/* Light LED1 */
						au8Led[1] = 0xFF;
					}
				}
			}

			/* Have the buttons changed ? */
			if (u8ButtonRead != u8Button)
			{
				/* Has the program button been released ? */
				if ((u8ButtonRead & BUTTON_P_MASK) == 0 && (u8Button & BUTTON_P_MASK) != 0)
				{
					/* End device {v5} ? */
					if (bEndDevice)
					{
						/* We are going into sleep mode */
						bSleep = TRUE;
						/* Start idle timer for entering sleep mode */
						u16IdleTimer = TIMEOUT_IDLE_ASLEEP;
					}
					/* Coordinator or router {v5} ? */
					else
					{
						/* Toggle permit join setting */
						eStatus = eJenie_SetPermitJoin(! bJenie_GetPermitJoin());
						/* Output to UART */
						vPrintf("eJenie_SetPermitJoin(%d) = %d\n",
							bJenie_GetPermitJoin(),
							eStatus);
					}
				}

				/* Has button 0 been released ? */
				if ((u8ButtonRead & BUTTON_0_MASK) == 0 && (u8Button & BUTTON_0_MASK) != 0)
				{
					/* Was the button not down a long time {v4} ? */
					if (au16ButtonTimer[0] < TIMEOUT_LONG_PRESS)
					{
						/* Initialise data for transmission */
						uint8 au8Data[3] = "B0";
						/* Choose an address to send data to */
						if (u64Last != 0ULL) u64Address = u64Last;
						else                 u64Address = u64Parent;

						/* Try to send data */
						eStatus = eJenie_SendData(u64Address, au8Data, 3, TXOPTION_ACKREQ);
						/* Output to UART */
						vPrintf("eJenie_SendData(%x:%x, '%s', %d, %x) = %d\n",
							(uint32)(u64Address >> 32),
							(uint32)(u64Address &  0xFFFFFFFF),
							au8Data,
							3,
							TXOPTION_ACKREQ,
							eStatus);
						/* End device and not in sleep mode - reset a long wake timer {v5} */
						if (bEndDevice && ! bSleep) u16IdleTimer = TIMEOUT_IDLE_AWAKE;

						/* No errors ? */
						if (E_JENIE_DEFERRED == eStatus)
						{
							/* Light LED0 */
							au8Led[0] = 0xFF;
						}
					}
					/* Reset the button timer {v4} */
					au16ButtonTimer[0] = 0;
				}

				/* Has button 1 been released ? */
				if ((u8ButtonRead & BUTTON_1_MASK) == 0 && (u8Button & BUTTON_1_MASK) != 0)
				{
					/* Was the button not down a long time {v4} ? */
					if (au16ButtonTimer[1] < TIMEOUT_LONG_PRESS)
					{
						/* Have we bound services yet ? */
						if (bBound)
						{
							/* Initialise data for transmission */
							uint8 au8Data[3] = "B1";

							/* Try to send data */
							eStatus = eJenie_SendDataToBoundService(SERVICE_NUMBER, au8Data, 3, 0);
							/* Output to UART */
							vPrintf("eJenie_SendDataToBoundService(%d, '%s', %d, %x) = %d\n",
								SERVICE_NUMBER,
								au8Data,
								3,
								0,
								eStatus);
							/* End device and not in sleep mode - reset a long wake timer {v5} */
							if (bEndDevice && ! bSleep) u16IdleTimer = TIMEOUT_IDLE_AWAKE;

							/* Can we send data ? */
							if (E_JENIE_DEFERRED == eStatus)
							{
								/* Light LED1 */
								au8Led[1] = 0xFF;
								/* Note we've sent data to a service */
								bSentService = TRUE;
							}
						}
					}
					/* Reset the button timer {v4}*/
					au16ButtonTimer[1] = 0;
				}

				/* Note the current button reading */
				u8Button = u8ButtonRead;
			}
		}

		/* Idle timer running {v5} ? */
		if (u16IdleTimer > 0)
		{
			/* Decrement the idle timer */
			u16IdleTimer--;
			/* Idle timer expired and we are and end device and network is up ? */
			if (u16IdleTimer == 0 && bEndDevice && bNetworkUp)
			{
				/* Are we in sleep mode ? */
				if (bSleep)
				{
					/* Turn off LEDs as we're going to sleep */
					au8Led[0] = 0;
					au8Led[1] = 0;
					/* Output to UART (before sleeping) */
					vPrintf("eJenie_Sleep(E_JENIE_SLEEP_OSCON_RAMON)\n");
					/* Wait for tx fifo and shift register to be empty */
					while ((u8JPI_UartReadLineStatus(E_JPI_UART_0) & (E_JPI_UART_LS_THRE | E_JPI_UART_LS_TEMT)) !=
						(E_JPI_UART_LS_THRE | E_JPI_UART_LS_TEMT));
					/* Go to sleep holding memory */
					(void) eJenie_Sleep(E_JENIE_SLEEP_OSCON_RAMON);
				}
				else
				{
					/* Initialise data for transmission */
					uint8 au8Data[2] = "I";

					/* Send to our parent to let it know we are awake */
					eStatus = eJenie_SendData(u64Parent, au8Data, 2, 0);
					/* Output to UART */
					vPrintf("eJenie_SendData(%x:%x, '%s', %d, %x) = %d\n",
						(uint32)(u64Parent >> 32),
						(uint32)(u64Parent &  0xFFFFFFFF),
						au8Data,
						2,
						0,
						eStatus);

					/* Restart a long idle timer */
					u16IdleTimer = TIMEOUT_IDLE_AWAKE;
				}
			}
		}

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
