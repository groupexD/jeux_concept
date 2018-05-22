
#include <cvirte.h>
#include <stdio.h>
#include <stdlib.h>
#include <tcpsupp.h>
#include <string.h>
#include <utility.h>
#include <userint.h>
#include "server.h"




int CVICALLBACK ServerTCPCB (unsigned handle, int event, int error,
							 void *callbackData);
static void ReportTCPError (void);


static unsigned int g_hconversation;
static int          g_hmainPanel = 0;
static int			g_TCPError = 0;


int main (int argc, char *argv[])
{
	int      portNum;
	char     tempBuf[256] = {0};
	int		 registered = 0;

	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;
	if ((g_hmainPanel = LoadPanel(0, "server.uir", MAINPNL)) < 0)
		return -1;
	DisableBreakOnLibraryErrors();

	/* Prompt for the port number on which to receive connections */

	portNum = 1999;

	/* Attempt to register as a TCP server... */
	SetWaitCursor (1);
	if (RegisterTCPServer (portNum, ServerTCPCB, 0) < 0)
		MessagePopup("TCP Server", "Server registration failed!");
	else
		{
		registered = 1;
		SetWaitCursor (0);
		
		/* We are successfully registered -- gather info */
		SetCtrlVal (g_hmainPanel, MAINPNL_ONLINE, 1);
		if (GetTCPHostAddr (tempBuf, 256) >= 0)
	        SetCtrlVal (g_hmainPanel, MAINPNL_SERVER_IP, tempBuf);
		if (GetTCPHostName (tempBuf, 256) >= 0)
	        SetCtrlVal (g_hmainPanel, MAINPNL_SERVER_NAME, tempBuf);
		SetCtrlAttribute (g_hmainPanel, MAINPNL_STRING, ATTR_DIMMED, 1);
		SetCtrlVal (g_hmainPanel, MAINPNL_CONNECTED, 0); 
		
		/* Display the panel and run the UI */
		DisplayPanel (g_hmainPanel);
		SetActiveCtrl (g_hmainPanel, MAINPNL_STRING);
		RunUserInterface ();
		}
	
	if (registered)
		UnregisterTCPServer (portNum);
		
	/* Free resources and return */
	DiscardPanel (g_hmainPanel);
	CloseCVIRTE ();
	return 0;
}


int CVICALLBACK TransmitCB (int panelHandle, int controlID, int event,
							void *callbackData, int eventData1, int eventData2)
{
	char transmitBuf[512] = {0};

	switch (event)
		{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, MAINPNL_STRING, transmitBuf);
			strcat (transmitBuf, "\n");
			SetCtrlVal (panelHandle, MAINPNL_TRANSMIT, transmitBuf);
			SetCtrlVal (panelHandle, MAINPNL_STRING, "");
			if (ServerTCPWrite (g_hconversation, transmitBuf,
								strlen (transmitBuf), 1000) < 0)
				SetCtrlVal (panelHandle, MAINPNL_TRANSMIT,
							"Transmit Error\n");
			break;
		}
	return 0;
}


int CVICALLBACK ServerTCPCB (unsigned handle, int event, int error,
							 void *callbackData)
{
	char receiveBuf[256] = {0};
	ssize_t dataSize        = sizeof (receiveBuf) - 1;
	char addrBuf[31];

	switch (event)
		{
		case TCP_CONNECT:
			if (g_hconversation)
				{
				/* We already have one client, don't accept another... */
				
				sprintf (receiveBuf, "-- Refusing conection request from "
									 "%s --\n", addrBuf); 
				SetCtrlVal (g_hmainPanel, MAINPNL_RECEIVE, receiveBuf);
				;
				}
			else
				{
				/* Handle this new client connection */
				g_hconversation = handle;
				SetCtrlVal (g_hmainPanel, MAINPNL_CONNECTED, 1);
				
				SetCtrlVal (g_hmainPanel, MAINPNL_CLIENT_IP, addrBuf);
				
				SetCtrlVal (g_hmainPanel, MAINPNL_CLIENT_NAME, receiveBuf);
				sprintf (receiveBuf, "-- New connection from %s --\n",
						 addrBuf); 
				SetCtrlVal (g_hmainPanel, MAINPNL_RECEIVE, receiveBuf);
				SetCtrlAttribute (g_hmainPanel, MAINPNL_STRING, ATTR_DIMMED,
								  0);
		
				}
			break;
		case TCP_DATAREADY:
			if ((dataSize = ServerTCPRead (g_hconversation, receiveBuf,
										   dataSize, 1000))
				< 0)
				{
				SetCtrlVal (g_hmainPanel, MAINPNL_RECEIVE, "Receive Error\n");
				}
			else
				{
				receiveBuf[dataSize] = '\0';
				SetCtrlVal (g_hmainPanel, MAINPNL_RECEIVE, receiveBuf);
				}
			break;
		case TCP_DISCONNECT:
			if (handle == g_hconversation)
				{
				/* The client we were talking to has disconnected... */
				SetCtrlVal (g_hmainPanel, MAINPNL_CONNECTED, 0);
				g_hconversation = 0;
				SetCtrlVal (g_hmainPanel, MAINPNL_CLIENT_IP, "");
				SetCtrlVal (g_hmainPanel, MAINPNL_CLIENT_NAME, "");
				SetCtrlVal (g_hmainPanel, MAINPNL_RECEIVE,
							"-- Client disconnected --\n");
				SetCtrlAttribute (g_hmainPanel, MAINPNL_STRING, ATTR_DIMMED,
								  1);
				
				}
			break;
	}
	
Done:    
	return 0;
}

int CVICALLBACK ClearScreenCB (int panel, int control, int event,
							   void *callbackData, int eventData1,
							   int eventData2)
{
	if (event == EVENT_COMMIT)
		ResetTextBox (panel, MAINPNL_RECEIVE, "");
	return 0;
}

int CVICALLBACK MainPanelCB (int panel, int event, void *callbackData,
							 int eventData1, int eventData2)
{
	if (event == EVENT_CLOSE)
		QuitUserInterface (0);
	return 0;
}


