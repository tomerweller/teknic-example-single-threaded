///*******************************************************************************
// $Archive: /ClearPath SC/Examples and Testing/sdk examples/Example-SingleThreaded/Example-SingleThreaded.cpp $
// $Revision: 15 $ $Date: 12/22/16 10:49a $
/**
	\file
	\brief Basic Operations Example

	The main program for single threaded ClearPath-SC example. The only command
	line argument is the port number where the network is attached. This
	main function opens the port, prints some basic information about the
	nodes that are found, checks that they are all in full-access mode,
	then creates the Axis objects to run the nodes individually.
**/
//******************************************************************************

#include <stdio.h>
#include <ctime>
#include <string>
#include <iostream>
#include "Axis.h"

// Send message and wait for newline
void msgUser(const char *msg) {
	std::cout << msg;
	getchar();
}

/*****************************************************************************
*  Function Name: InitializeSysManager
*	Description:	This function takes a list of COM ports and
*					initializes the sysManager object with those
*					ports, opening the ports for communication and,
*					optionally, printing out statistics for each
*					node found in the system
*	Parameters:
*	    In/Out:		sysMgr			- a pointer to a sysManager object
*		Input:		numPorts		- the number of ports in the system
*					listOfPorts		- the list of COM ports in the system
*					printNodeStats	- print stats about each node?
*		Return:		true if the system initialized properly
*****************************************************************************/
bool InitializeSysManager(SysManager *sysMgr,
	unsigned numPorts, char *listOfPorts[NET_CONTROLLER_MAX],
	bool printNodeStats) {

	// Create all the ports in the system manager
#if defined(_WIN32) || defined(_WIN64)
	for (size_t iPort = 0; iPort < numPorts; iPort++) {
		printf("Initializing port %d, port number %d\n", int(iPort), atoi(listOfPorts[iPort]));
		sysMgr->ComHubPort(iPort, atoi(listOfPorts[iPort]));
	}
#else
	for (int iPort = 0; iPort < (int)numPorts; iPort++) {
		printf("Initializing port %d, port number %s\n", int(iPort), listOfPorts[iPort]);
		sysMgr->ComHubPort(iPort, listOfPorts[iPort]);
	}
#endif

	try {
		// Attempt to open the ports that were specified
		printf("Opening ports...\n");
		sysMgr->PortsOpen((size_t)numPorts);
		printf("  ... ports are open\n");
	}
	catch (mnErr theErr) {
		fprintf(stderr, "Error - Port setup issue\n");
		fprintf(stderr, " Caught error: addr=%d, err=0x%0x\n  msg=%s\n",
			theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		printf(" Caught error: addr=%d, err=0x%0x\n  msg=%s\n",
			theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		return false;
	}

	// If the caller wants stats about each node, print them out before returning
	if (printNodeStats) {
		for (size_t iPort = 0; iPort < numPorts; iPort++) {
			IPort &myPort = sysMgr->Ports(iPort);
			printf(" Port[%d]: state=%d, nodes=%d\n", myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());

			for (size_t iNode = 0; iNode < myPort.NodeCount(); iNode++) {
				INode &theNode = myPort.Nodes(iNode);
				printf("   Node[%d]: type=%d\n", int(iNode), theNode.Info.NodeType());

				printf("            userID: %s\n", theNode.Info.UserID.Value());
				printf("        FW version: %s\n", theNode.Info.FirmwareVersion.Value());
				printf("        HW version: %s\n", theNode.Info.HardwareVersion.Value());
				printf("          Serial #: %d\n", theNode.Info.SerialNumber.Value());
				printf("             Model: %s\n", theNode.Info.Model.Value());
			}
		}
	}

	// Indicate that the system was initialized 
	return true;
}

/*****************************************************************************
*  Function Name: main
*	Description:	The main function for this single threaded example. 
*					One command-line argument is expected, which is the COM
*					port number for where the SC network is attached.
*		Return:		0 if successful; non-zero if there was a problem
*****************************************************************************/
int main(int argc, char* argv[])
{
	// Assume we will be successful
	int returnVal = 0;
	// The network manager
	SysManager myMgr;

	try {

		// Make sure the port number is given as a command-line argument
		if (argc != 2) {
			printf(" USAGE: Example-SingleThreaded.exe <Port#>\n");
			printf("   Given Args: count: %d\n", argc);
			printf("                argv: ");
			for (int i = 0; i < argc; i++)
				printf("%s ", argv[i]);
			printf("\n");
			msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
			return(-1);
		}

		// This example works on only a single port; the COM port number
		// is given as a command line argument
		unsigned numPorts = 1;
		char *portsList[NET_CONTROLLER_MAX];
		portsList[0] = argv[1];	// save the command-line argument in our list

		bool initialized = InitializeSysManager(&myMgr, numPorts, portsList, true);
		if (!initialized) {
			printf("Error: Unable to initialize the system\n");
			msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
			return(-2);
		}

		// Create a list of axes - there will be one Axis per node
		vector<Axis*> listOfAxes;

		// Assume that the nodes are of the right type and that this app has full control
		bool nodeTypesGood = true, accessLvlsGood = true;

		for (size_t iPort = 0; iPort < numPorts; iPort++) {
			// Get a reference to the port, to make accessing it easier
			IPort &myPort = myMgr.Ports(iPort);

			// Uncomment and fill-out the following line to configure a group shutdown
			//myPort.GrpShutdown.ShutdownWhen(/*Node's Index*/, /*Shutdown Info*/);

			// Print out some information about the port
			printf(" Port[%d]: state=%d, nodes=%d\n", myPort.NetNumber(), myPort.OpenState(), myPort.NodeCount());

			for (unsigned iNode = 0; iNode < myPort.NodeCount(); iNode++) {
				// Get a reference to the node, to make accessing it easier
				INode &theNode = myPort.Nodes(iNode);

				// Make sure we are talking to a ClearPath SC (advanced or basic model will work)
				if (theNode.Info.NodeType() != IInfo::CLEARPATH_SC_ADV
					&& theNode.Info.NodeType() != IInfo::CLEARPATH_SC) {
					printf("---> ERROR: Uh-oh! Node %d is not a ClearPath-SC Motor\n", iNode);
					nodeTypesGood = false;
				}

				if (nodeTypesGood) {
					// Create an axis for this node
					listOfAxes.push_back(new Axis(&theNode));

					// Make sure we have full access
					if (!theNode.Setup.AccessLevelIsFull()) {
						printf("---> ERROR: Oh snap! Access level is not good for node %u\n", iNode);
						accessLvlsGood = false;
					}
				}
			}
		}

		// If we have full access to the nodes and they are all ClearPath-SC nodes, 
		// then continue with the example
		if (nodeTypesGood && accessLvlsGood) {
			for (Uint16 iAxis = 0; iAxis < listOfAxes.size(); iAxis++) {
				// Tell each axis to do its thing
				listOfAxes.at(iAxis)->AxisMain();
			}
		}
		else {
			// If something is wrong with the nodes, tell the user about it
			if (!nodeTypesGood) {
				printf("\n\tFAILURE: Please attach only ClearPath-SC nodes.\n\n");
				returnVal = -5;
			}
			else if (!accessLvlsGood) {
				printf("\n\tFAILURE: Please get full access on all your nodes.\n\n");
				returnVal = -6;
			}
		}

		// Delete the list of axes that were created
		for (size_t iAxis = 0; iAxis < listOfAxes.size(); iAxis++) {
			delete listOfAxes.at(iAxis);
		}

		// Close down the ports
		myMgr.PortsClose();

	}
	catch (mnErr theErr) {
		fprintf(stderr, "Caught error: addr=%d, err=0x%0x\nmsg=%s\n",
			theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		printf("Caught error: addr=%d, err=0x%08x\nmsg=%s\n",
			theErr.TheAddr, theErr.ErrorCode, theErr.ErrorMsg);
		returnVal = -3;
	}
	catch (...) {
		fprintf(stderr, "Error generic caught\n");
		printf("Generic error caught\n");
		returnVal = -4;
	}

	// Good-bye
	msgUser("Press any key to continue."); //pause so the user can see the error message; waits for user to press a key
	return returnVal;
}
//																			   *
//******************************************************************************

