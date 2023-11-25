// CMODEthCommsProcessor.cpp: 
//
// Mod_RSSim (c) Embedded Intelligence Ltd. 1993,2009
// AUTHOR: Conrad Braam.  http://www.plcsimulator.org
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// implementation of the Ethernet CMODEthCommsProcessor class.
//
// Ethernet:
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define MAX_MOD_MESSAGELENGTH  2048*4

//BOOL m_msgLenOK = FALSE;                        // nlrkooi todo: probably can remove (used for crc?)

// ------------------------- GetBCC ---------------------------------
// PURPOSE: Calculates a messages BCC value and returns it
// NOTE:
//    The message must be "[NNNNNNNN"
//    The bcc may be "cccc" or come from a valid message from the PLC.
//
LONG GetBCC(CHAR * bccString, DWORD msgLen)
{
LONG    accumulator = 0;

   return (accumulator);
} // GetBCC



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// constructor to create listen socket
CMODEthCommsProcessor::CMODEthCommsProcessor(int responseDelay,
                                             BOOL MOSCADchecks,
                                             BOOL modifyThenRespond,
                                             BOOL disableWrites,
                                             LONG PDUSize,
                                             WORD portNum) : CDDKSrvSocket(portNum)
{
CString description;
 m_protocolName = "MODBUS Eth.";

   InitializeCriticalSection(&stateCS);
   description.Format("Starting comms emulation : %s", "MODBUS TCP/IP [host]");
   SockDataMessage(description);
   if (m_SaveDebugToFile) CRS232Port::WriteToFile(description);  // Added statement on 2016-12-26 by DL to save to Log file

   m_responseDelay = 0;
   m_linger = FALSE;

   m_responseDelay = responseDelay;
   SetPDUSize(PDUSize);

   SetEmulationParameters(MOSCADchecks, modifyThenRespond, disableWrites);
   m_pWorkerThread->ResumeThread(); //start thread off here

}

// constructor to re-use the listen socket
CMODEthCommsProcessor::CMODEthCommsProcessor(int responseDelay,
                                             BOOL  MOSCADchecks,
                                             BOOL modifyThenRespond,
                                             BOOL disableWrites,
                                             LONG PDUSize,
                                             SOCKET * pServerSocket) : CDDKSrvSocket(0, 0, pServerSocket)
{
   InitializeCriticalSection(&stateCS);
   m_responseDelay = 0;
   m_linger = FALSE;

   m_responseDelay = responseDelay;
   SetPDUSize(PDUSize);

   SetEmulationParameters(MOSCADchecks, modifyThenRespond, disableWrites);
   m_pWorkerThread->ResumeThread(); //start thread off here

}

CMODEthCommsProcessor::~CMODEthCommsProcessor()
{

}

// ------------------------------- SockDataMessage ------------------------------
void CMODEthCommsProcessor::SockDataMessage(LPCTSTR msg)
{
   EnterCriticalSection(&stateCS);
   OutputDebugString("##");
   if (NULL!=pGlobalDialog)
      pGlobalDialog->AddCommsDebugString(msg);
   LeaveCriticalSection(&stateCS);
   
   CString msg2;
   msg2.Format("From CMODEthCommsProcessor::SockDataMessage \n");
   if (NULL!=pGlobalDialog)
      pGlobalDialog->AddCommsDebugString(msg2);
   LeaveCriticalSection(&stateCS);
}


// ------------------------------ SockDataDebugger ------------------------------
void CMODEthCommsProcessor::SockDataDebugger(const CHAR * buffer, LONG length, dataDebugAttrib att)
{
//   MessageBox(NULL, "MODETHComms", "DataDebugger", MB_OK);    // Left for Debug Purposes by DL on 2015-01-06
   CRS232Port::GenDataDebugger((BYTE*)buffer, length, att);
} // SockDataDebugger

// ------------------------------- SockStateChanged -----------------------
void CMODEthCommsProcessor::SockStateChanged(DWORD state)
{
   EnterCriticalSection(&stateCS);
   if (NULL != pGlobalDialog)
      pGlobalDialog->m_ServerSocketState = state;

   LeaveCriticalSection(&stateCS);
} // SockStateChanged

// ------------------------------- ActivateStationLED ---------------------------
void CMODEthCommsProcessor::ActivateStationLED(LONG stationID)
{
   if (stationID>=0 && stationID<STATIONTICKBOXESMAX)	// 2015-07-22 by DL added "=" for Active Station #0
   {
      //start the counter for this station at the beginning
      if (NULL != pGlobalDialog)
         pGlobalDialog->m_microTicksCountDown[stationID] = pGlobalDialog->GetAnimationOnPeriod();
      // it will count down untill it extinguishes
   }
} // ActivateStation

// ------------------------------- StationIsEnabled ---------------------------
// Return TRUE if station is enabled
BOOL CMODEthCommsProcessor::StationIsEnabled(LONG stationID)
{
   if (!pGlobalDialog)
      return(FALSE);
   if (stationID>=0 && stationID<STATIONTICKBOXESMAX)	// 2015-07-22 by DL added "=" for Active Station #0
   {
      return (pGlobalDialog->StationEnabled(stationID));
   }
   return TRUE;
} // StationIsEnabled


// --------------------------------- ProcessSimData -----------------------------
// The buffer passed in is all of the data available on the socket, so we could have 
// multiple requests if the other end is timing out or sends too fast.
BOOL CMODEthCommsProcessor::ProcessSimData(SOCKET openSocket, const CHAR *pBuffer, const DWORD numBytes)
{
    CString msg;
    //Expect words 
    if (numBytes % 2)
    {
        msg.Format("Did not reciece even number of bytes \n");
        SockDataMessage(msg);
   //     return FALSE;
    }
    BOOL DEBUG_SIM = TRUE;
    if (DEBUG_SIM)
    {
        msg.Format("DEBUG1 SIM MESSAGE \n");
        SockDataMessage(msg);
    }

    CHAR  telegramBuffer[MAX_MOD_MESSAGELENGTH];
    CHAR  responseBuffer[MAX_MOD_MESSAGELENGTH];
    CHAR  debugStr[MAX_MOD_MESSAGELENGTH];
    WORD  responseLen;
    BYTE  *pDataPortion;
    int   i=0;

    WORD  requestMemArea;   // telegram read/write are being referenced=0..MAX_MOD_MEMTYPES


    WORD  startRegister, endRegister, MBUSerrorCode=0;
    WORD  seperationOffset; // offset added to each address, due to stations having seperate reg.s
    BOOL  MBUSError = FALSE;
    WORD  numBytesInReq;
    WORD  numRegs;
    CString deb;
    BOOL transmitted = TRUE;



    BYTE ReadStartHigh;
    BYTE ReadStartLow;
    BYTE ReadLengthHigh;
    BYTE ReadLengthLow;
    BYTE WriteStartHigh;
    BYTE WriteStartLow;
    BYTE WriteDataByteCount;
    WORD ReadDataWordCount;
    BYTE WriteLengthLow;
    BYTE WriteLengthHigh;
    WORD WriteDataWordCount;
    WORD ReadStartAddress;
    WORD WriteStartAddress;


   m_debuggerStep = 100;
   // inc counter
   pGlobalDialog->PacketsReceivedInc();

   responseBuffer[0] = '\0';

   // copy the Rx'd telegram neatly
   memcpy(telegramBuffer, pBuffer, numBytes);
   telegramBuffer[numBytes] = '\0';

   if (pGlobalDialog->m_Unloading)
      return(TRUE);           // stop processing during shutdown of the socket array to allow easy closing.


   CSIMMessage  modMsg(telegramBuffer, numBytes);

    if (DEBUG_SIM)
    {
        msg.Format("DEBUG2 message.actionCode = %04X \n" , modMsg.actionCode);
        SockDataMessage(msg);
        msg.Format("DEBUG2 message.adress = %04X \n" , modMsg.address);
        SockDataMessage(msg);
        msg.Format("DEBUG2 message.byteCount = %04X \n", modMsg.byteCount);
        SockDataMessage(msg);
        msg.Format("DEBUG2 message.bitCount = %04X \n", modMsg.bitCount);
        SockDataMessage(msg);
        msg.Format("DEBUG2 message.overalLen = %04X \n", modMsg.overalLen);
        SockDataMessage(msg);
 //       if (modMsg.actionCode == 0x0102) {
 //           msg.Format("DEBUG2 no real byte swap \n");
 //           SockDataMessage(msg);
 //       }
 //       if (modMsg.actionCode == 0x0201) {
 //           msg.Format("DEBUG2 byte swap \n");
 //           SockDataMessage(msg);
 //       }
 //       msg.Format("DEBUG2 message.adress = %02X%02X \n", HIBYTE(modMsg.address), LOBYTE(modMsg.address) );
 //       SockDataMessage(msg);
    }


   // 2. parse 

   // Get memory area which we have to update or retrieve from
   requestMemArea = modMsg.GetAddressArea(modMsg.actionCode);//.type);
   if (DEBUG_SIM)
   {
       msg.Format("DEBUG3 requestMemArea = %02d \n", requestMemArea);
       SockDataMessage(msg);
   }


   if (requestMemArea >= MAX_MOD_MEMTYPES)
   {
      // TO DO!
      // handle the error
      Beep(2000,200);
      requestMemArea = 3;  // for now just default to "Holding"!
   }

   //
   // Validate that the request is a valid command code
   //
   startRegister = modMsg.address;
   //endRegister = startRegister + modMsg.byteCount/2;
                                                 
    if ((modMsg.actionCode == SIM_READ_COILS)||          // 01
        (modMsg.actionCode == SIM_READ_DIGITALS)||       // 02
	    (modMsg.actionCode == SIM_WRITE_COILS)) // 15
	    endRegister = startRegister + modMsg.wordCount;    // Previous code correct for bits TODO: how this works! looks like coil registers are "bytes?)
    // is endRegister last register OR last register + 1?  
    // endRegister is used for error checking(to make sure inside PLC memory area. -- > looks like current implimentation will wrongly generate and error if trying to write to last byteCount
    // conclusion: ignor -> not planning on writing to last bytes anyway
    else
        endRegister = startRegister + modMsg.wordCount;  // Added "/2" on 2015-07-18 by DL for regs
                                            
   if ((modMsg.actionCode == SIM_READ_COILS)||      // 01
       (modMsg.actionCode == SIM_READ_DIGITALS)||   // 02
       (modMsg.actionCode == SIM_READ_REGISTERS)||  // 04
       (modMsg.actionCode == SIM_READ_HOLDING)||    // 03
	   (modMsg.actionCode == SIM_WRITE_COILS) ||       // 07  Added 2015-01-13 by DL
       (modMsg.actionCode == SIM_WRITE_DIGITALS) ||    // 08 Added 2015-Dec-17 by DL
	   (modMsg.actionCode == SIM_WRITE_REGISTERS) ||   // 17  Added 2015-Dec-15 by DL
       (modMsg.actionCode == SIM_WRITE_HOLDING)        // 20
      )
   {
      // Check the request length against our PDU size.
      switch (modMsg.actionCode)
      {
      case SIM_READ_COILS:      // 01
      case SIM_READ_DIGITALS:   // 02
         numBytesInReq = modMsg.bitCount; // # bits
         break;
      default:
         numBytesInReq = modMsg.wordCount; // # registers
         break;
      }
      if (numBytesInReq > m_PDUSize)
      {
         MBUSError = TRUE;
         MBUSerrorCode = MOD_EXCEPTION_ILLEGALVALUE;   // too long data field
         if (DEBUG_SIM)
         {
             msg.Format("EXCEPTION numBytesInReq > m_PDUSize \n");
             SockDataMessage(msg);
         }
      }

      else
         MBUSError = FALSE;
   }
   else
   {
      MBUSError = TRUE;
      MBUSerrorCode = MOD_EXCEPTION_ILLEGALFUNC;   // 01
      if (DEBUG_SIM)
      {
          msg.Format("EXCEPTION MOD_EXCEPTION_ILLEGALFUNC \n");
          SockDataMessage(msg);
      }
   }

   if (modMsg.m_packError)
   {
      // request message has a corrupted field somewhere
      MBUSError = TRUE;
      MBUSerrorCode = MOD_EXCEPTION_ILLEGALVALUE;   // too long data field
	  //MessageBox(NULL, "Illegal Value", "Error", MB_OK);  // Added for testing by DL & Deleted on 2016-09-14
      if (DEBUG_SIM)
      {
          msg.Format("EXCEPTION MOD_EXCEPTION_ILLEGALVALUE \n");
          SockDataMessage(msg);
      }
   }

   if (DEBUG_SIM)
   {
       msg.Format("next step: build responce \n");
       SockDataMessage(msg);
   }
   // 3. build response
   CSIMMessage  responseModMsg(modMsg); //Call copy constructor

   if (DEBUG_SIM)
   {
       msg.Format("test test test ---------------- \n");
       SockDataMessage(msg);
   }

   // do a address+length range check too
   // Added tests for WRITE_MULTIPLE_COILS & WRITE_HOLDING on 2017-01-01 by DL
   if ((!MBUSError)&& ((modMsg.actionCode == SIM_WRITE_COILS) || (modMsg.actionCode == SIM_WRITE_HOLDING) || (modMsg.actionCode == SIM_WRITE_REGISTERS)))
   {
      if (PLCMemory[requestMemArea].GetSize() < endRegister)
      {
         MBUSError = TRUE;
		 // Next line added 2017-01-01 by DL because of bad Exception Code and following two lines commented out
		 MBUSerrorCode = MOD_EXCEPTION_ILLEGALADDR;
         //MBUSerrorCode = (PLCMemory[requestMemArea].GetSize() < startRegister ?
         //                    MOD_EXCEPTION_ILLEGALADDR:MOD_EXCEPTION_ILLEGALVALUE);   // 02:03
         if (DEBUG_SIM)
         {
             msg.Format("EXCEPTION MOD_EXCEPTION_ILLEGALADDR \n");
             SockDataMessage(msg);
         }
      }
   }

   if (MBUSError)
   {
   CString msg;
      msg.Format("Modbus message in error, Exception code x%02X\n", MBUSerrorCode);
      OutputDebugString(msg);
      SockDataMessage(msg);
	  if (m_SaveDebugToFile) CRS232Port::WriteToFile(msg);  // Added statement on 2016-12-26 by DL to save to Log file
   }


  
// dont use this
      seperationOffset = 0;


   //
   // Request message seems error free, process it.
   //
                        // 1st 3 bytes + any others up to data get 
                        // added in at this time 
   responseModMsg.BuildMessagePreamble(MBUSError,
                                       MBUSerrorCode);

//   responseModMsg.totalLen = ((WORD)responseModMsg.dataPtr-(LONG)responseModMsg.buffer);

   // If a write is done then the copy constructor will have done all required for ack
   // else a read must now pack into dataPtr onwards! and calc len etc!

   // A read must now pack into dataPtr onwards! and calc len etc!
   //   writes must update our mem areas accordingly.
   if (!MBUSError)
   {
      // 4. fill in the data portion of telegram
      switch (modMsg.actionCode)
      {
      case SIM_READ_COILS     : // 01 READ
      case SIM_READ_DIGITALS  : // 02 READ
         pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data
         if (MAX_MOD_MEMWORDS >= modMsg.address + modMsg.byteCount)
         {
         WORD memValueTemp;
         WORD bitOffset;

//#ifdef _COMMS_DEBUGGING
		 if (modMsg.actionCode == SIM_READ_COILS)           // Added 2015-01-08 by DL
            deb.Format("Read Output Coils from %d for %d bits.\n", modMsg.address, modMsg.byteCount);// Added 2015-01-08 by DL
		 if (modMsg.actionCode == SIM_READ_DIGITALS)        // Added 2015-01-08 & Modified 2015-01-13 by DL
			 deb.Format("Read Inputs from %d for %d bits.\n", modMsg.address, modMsg.byteCount);// Added 2015-01-08 by DL
//         deb.Format("Read In/output from %d for %d bits.\n", modMsg.address, modMsg.byteCount); // Removed 2015-01-08 by DL
		    OutputDebugString(deb);
            SockDataMessage(deb);
			if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
//#endif

            // pack SIM memory: one WORD of sim memory for every BIT in the data.
            // Ie the sim uses one word for each I/O bit in modbus.
            numBytesInReq = modMsg.byteCount/8;
            if (modMsg.byteCount%8)  // if we overflow the byte
               numBytesInReq++;
            for (i=0; i <numBytesInReq;i++)
            {
               // grab the memory now
               memValueTemp = 0;
               for (bitOffset=0;bitOffset<8;bitOffset++)
               {
                  if ((i*8)+bitOffset < modMsg.byteCount)
                     if (PLCMemory[requestMemArea][(seperationOffset + modMsg.address)+(i*8)+bitOffset])
                        memValueTemp += (0x01<<bitOffset);
                     // else bit is off
               }
               *(BYTE*)pDataPortion =  (BYTE)memValueTemp;
               pDataPortion +=1;
            }

         }
         else
         {
		  //nlrkooi TODO
            // pack the exception code into the message  
            responseModMsg.buffer[1] |= 0x80;
            responseModMsg.buffer[2] = 0x02;    // exception code here (could also use 0x03)
         }
         break;

      case SIM_READ_REGISTERS : 
      case SIM_READ_HOLDING   :
         pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data
         if (MAX_MOD_MEMWORDS >= modMsg.address + modMsg.byteCount)
         {
            WORD memValueTemp;

//#ifdef _COMMS_DEBUGGING
		 if (modMsg.actionCode == SIM_READ_REGISTERS)           // Added 2015-01-09 by DL
            deb.Format("Read Input Regs from %d for %d.\n", modMsg.address, modMsg.byteCount);  // Added 2015-01-09 by DL
		 else                                                 // Added 2015-01-08 by DL
			deb.Format("Read Holding Regs from %d for %d.\n", modMsg.address, modMsg.byteCount);// Added 2015-01-09 by DL
//          deb.Format("Read Register from %d for %d .\n", modMsg.address, modMsg.byteCount);   // Removed 2015-01-09 by DL
            OutputDebugString(deb);
            SockDataMessage(deb);
			if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
//#endif
            for (i=0; i <modMsg.byteCount;i++)
            {
               // grab the memory now
               memValueTemp = PLCMemory[requestMemArea][(seperationOffset + modMsg.address)+i];
               *(WORD*)pDataPortion =  SwapBytes( memValueTemp );
               pDataPortion += 2;
            }
         }
         else
         {
		 // TODO!!!!!!!!!!!!
            // pack the exception code into the message
            responseModMsg.buffer[1] |= 0x80;
            responseModMsg.buffer[2] = 0x02;    // exception code here (could also use 0x03)

            deb.Format("Read register past %d error x%02X!\n", MAX_MOD_MEMWORDS, (BYTE)responseModMsg.buffer[2]);
            OutputDebugString(deb);
            SockDataMessage(deb);
			if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
         }
         break;
      default :
          //Writes acks are all built in copy constructor
          //But the update is done here!
          if (MAX_MOD_MEMWORDS >= modMsg.address + modMsg.byteCount)
          {
          // lock memory for writting
          CMemWriteLock lk(PLCMemory.GetMutex());
      
              pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data

              if (!lk.IsLocked())
                 //...Update
                 switch (modMsg.actionCode)
                 { 
                 case SIM_WRITE_COILS  :
                    // unpack into the SIM memory on WORD of sim memory for every BIT in the data 
                    //WORD numBytes;

                     numBytesInReq = modMsg.bitCount/8;
                     if (modMsg.bitCount%8)  // if we overflow a byte
                        numBytesInReq++;

                     {
                     CString deb;
                        deb.Format("Write multiple outputs coils from %d for %d bits.\n", modMsg.address, modMsg.bitCount);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
                     }
                     numRegs = numBytesInReq * 8;   // repaint X bits // 2015-01-19 Multiplied by 8 to get total BITS by DL

                     for (i=0;i<numBytesInReq;i++)
                     {
                     WORD bitOffset;
                        for (bitOffset=0;bitOffset<8;bitOffset++)
                        {
                           if ((i*8)+bitOffset < modMsg.bitCount)  // Changed <= to only < by DL on 2018-11-27
                           {
                              if (*(BYTE*)modMsg.dataPtr & (0x01<<bitOffset))
                                 PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address+(i*8)+bitOffset, 1);
                              else
                                 PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address+(i*8)+bitOffset, 0);
                           }
                        }
                        modMsg.dataPtr++;
                     }
                     break;
                 case SIM_WRITE_HOLDING : //WRITE multiple holdings
                     //PLCMemory[requestMemArea][modMsg.address] = SwapBytes(*(WORD*)modMsg.dataPtr);
                     //break;

                     numRegs = modMsg.byteCount/2;

                     {
                     CString deb;
                        deb.Format("Write multiple registers from %d for %d registers.\n", modMsg.address, numRegs);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
                     }
                     for (i=0;i<numRegs;i++)
                     {
                        PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address + i, SwapBytes(*(WORD*)modMsg.dataPtr));
                        modMsg.dataPtr +=2;   // inc pointer by 2 bytes
                     }
                     break;
                 }
          }
		  else
		  {
		    // Print Debugging Information
		    deb.Format("Attempted to address beyond register maximum limits with FC = %d.\n", modMsg.actionCode);
		    OutputDebugString(deb);
            SockDataMessage(deb);
		    if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);
		  }

          // we can only call on the GUI thread once we have un-locked
          if (pGlobalDialog)
          {
			 if (!pGlobalDialog->m_seperateRegisters)			// If not seperate registers redraw only small areas
			 {
             int cols = pGlobalDialog->GetListDisplayedWidth();
             pGlobalDialog->RedrawListItems(modMsg.GetAddressArea(modMsg.actionCode), 
                                            seperationOffset + modMsg.address/(cols),               // Start Row to Redraw
                                            (seperationOffset + modMsg.address+(numRegs-1))/(cols)  // End Row to Redraw
                                           ); // repaint only the needed rows
			 }
			 else												// If seperate registers need to redraw screen
			 {
			  // repaint all of the screen areas registers by DL on 2015-07-18
				pGlobalDialog->RedrawListItems(modMsg.GetAddressArea(modMsg.actionCode),0,6554);
			 }
          }
          break;
      } //end switch
   } 
   else
   { // error occurred
      pDataPortion = responseModMsg.dataPtr; // Get offset to fill in data
   }
   
   // THERE IS no CRC, so do not call BuildMessageEnd
   responseModMsg.totalLen = (WORD)((LONG)pDataPortion-(LONG)responseModMsg.buffer);






   // insert the frame info
   //responseModMsg
   if (responseModMsg.totalLen > MAX_MOD_MESSAGELENGTH)         // Added on 2015-01-11 by DL because one failure was 13412
      return (FALSE);                         // This was the Bit Write to 65536 that caused a crash in mod_RSsim
   memmove(&responseModMsg.buffer[ETH_PREAMBLE_LENGTH], responseModMsg.buffer, responseModMsg.totalLen);
   responseLen = ETH_PREAMBLE_LENGTH + responseModMsg.totalLen; //SwapBytes(*(WORD*)(responseModMsg.preambleLenPtr)) + 6; //hdr;
   memset(responseModMsg.buffer, 0 , ETH_PREAMBLE_LENGTH);

   *((WORD*)&responseModMsg.buffer[4]) = SwapBytes(responseModMsg.totalLen);
   // 6. send it back
   m_debuggerStep = 102;
   sprintf(debugStr, "Send %d bytes\n", responseLen);
   OutputDebugString(debugStr);

   SockStateChanged(SOCKETCURRENTLY_WRITTING);


   m_debuggerStep = 103;
   if (transmitted)
   {
      // inc counter
      pGlobalDialog->PacketsSentInc();
   }

   // If there are still more bytes in the data we read, process them recursively
   if ((WORD)(modMsg.m_frameLength + 6) < numBytes)
   {
      SockDataMessage("## Processing queued data bytes...");
      SockDataDebugger(&telegramBuffer[modMsg.m_frameLength + 6], numBytes - (modMsg.m_frameLength + 6), dataDebugOther);
      // recursive call using the data from our own stack-space
      ProcessData(openSocket, &telegramBuffer[modMsg.m_frameLength + 6], numBytes - (modMsg.m_frameLength + 6));
   }
   return (TRUE);
}

BOOL CMODEthCommsProcessor::ProcessData(SOCKET openSocket, const CHAR *pBuffer, const DWORD numBytes)
{
	if (m_protocol == PROTOCOL_SELSIM)
	{
        return ProcessSimData(openSocket, pBuffer, numBytes);
	}
	else
	{		
        return ProcessMODData(openSocket, pBuffer, numBytes);
	}
}
// --------------------------------- ProcessData -----------------------------
// The buffer passed in is all of the data available on the socket, so we could have 
// multiple requests if the other end is timing out or sends too fast.
BOOL CMODEthCommsProcessor::ProcessMODData(SOCKET openSocket, const CHAR *pBuffer, const DWORD numBytes)
{
CHAR  telegramBuffer[MAX_MOD_MESSAGELENGTH];
CHAR  responseBuffer[MAX_MOD_MESSAGELENGTH];
CHAR  debugStr[MAX_MOD_MESSAGELENGTH];
WORD  responseLen;
BYTE  *pDataPortion;
int   i=0;
//WORD  guardword1=1;
WORD  requestMemArea;   // telegram read/write are being referenced=0..MAX_MOD_MEMTYPES
//WORD  guardword2=2;
WORD  startRegister, endRegister, MBUSerrorCode=0;
WORD  seperationOffset; // offset added to each address, due to stations having seperate reg.s
BOOL  MBUSError = TRUE;
WORD  numBytesInReq;
WORD  numRegs;
CString deb;
BOOL transmitted = TRUE;

BYTE ReadStartHigh;
BYTE ReadStartLow;
BYTE ReadLengthHigh;
BYTE ReadLengthLow;
BYTE WriteStartHigh;
BYTE WriteStartLow;
BYTE WriteDataByteCount;
WORD ReadDataWordCount;
BYTE WriteLengthLow;
BYTE WriteLengthHigh;
WORD WriteDataWordCount;
WORD ReadStartAddress;
WORD WriteStartAddress;

//char buffer[10];
//MessageBox(NULL, _itoa(numBytes, buffer, 10), "ByteCount", MB_OK);
//   MessageBox(NULL, "Process Data MODEth ","FC", MB_OK);    // Added 2015-01-13 for testing only by DL

   m_debuggerStep = 100;
   // inc counter
   pGlobalDialog->PacketsReceivedInc();

   responseBuffer[0] = '\0';

   // copy the Rx'd telegram neatly
   memcpy(telegramBuffer, pBuffer, numBytes);
   telegramBuffer[numBytes] = '\0';
   

   // simulate the I/O and network delays
   Sleep(m_responseDelay);
   if (pGlobalDialog->m_Unloading)
      return(TRUE);           // stop processing during shutdown of the socket array to allow easy closing.

   // parse the telegram

   // 1. break up the telegram
   if (pGlobalDialog->m_selectedProtocol == PROTOCOL_SELMODETH_RTU){
       CMODMessage::SetEthernetFrames(FALSE);
   }
   else {
       CMODMessage::SetEthernetFrames();
   }
   CMODMessage  modMsg(telegramBuffer, numBytes);

   //check the station #
   ActivateStationLED(modMsg.stationID);
   if (!StationIsEnabled(modMsg.stationID))
   {
   CString msg;
      // station off-line
      msg.Format("Station ID %d off-line, no response sent\n", telegramBuffer[6]);
      SockDataMessage(msg);
	  if (m_SaveDebugToFile) CRS232Port::WriteToFile(msg);  // Added statement on 2016-12-26 by DL to save to Log file
      return(TRUE);
   }
   // 2. parse 

   // Get memory area which we have to update or retrieve from
   requestMemArea = modMsg.GetAddressArea(modMsg.functionCode);//.type);
   if (requestMemArea >= MAX_MOD_MEMTYPES)
   {
      // TO DO!
      // handle the error
      Beep(2000,200);
      requestMemArea = 3;  // for now just default to "Holding"!
   }

   //
   // Validate that the request is a valid command code
   //
   startRegister = modMsg.address;
   //endRegister = startRegister + modMsg.byteCount/2;
   if ((MOD_WRITE_SINGLE_COIL == modMsg.functionCode) || (MOD_READ_EXCEPTION == modMsg.functionCode) // 2015-01-13 DL
	   || (MOD_READ_SLAVEID == modMsg.functionCode) || (MOD_READ_DIAGNOSTIC == modMsg.functionCode)) // Added 2015-Dec-15 & 17 by DL
      endRegister = startRegister;
   else
      //endRegister = startRegister + modMsg.byteCount/2;  // Moved below inside IF on 2016-12-28 by DL
   {                                                          // Code section added 2016-12-28 by DL
	   if ((modMsg.functionCode == MOD_READ_COILS)||          // 01
           (modMsg.functionCode == MOD_READ_DIGITALS)||       // 02
		   (modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS)) // 15
		   endRegister = startRegister + modMsg.byteCount;    // Previous code correct for bits
	   else
           endRegister = startRegister + modMsg.byteCount/2;  // Added "/2" on 2015-07-18 by DL for regs
   }                                                          // End of Code section added 2016-12-28 by DL
   if ((modMsg.functionCode == MOD_READ_COILS)||      // 01
       (modMsg.functionCode == MOD_READ_DIGITALS)||   // 02
       (modMsg.functionCode == MOD_READ_REGISTERS)||  // 04
       (modMsg.functionCode == MOD_READ_HOLDING)||    // 03
	   (modMsg.functionCode == MOD_READ_EXCEPTION) || // 07  Added 2015-01-13 by DL
       (modMsg.functionCode == MOD_READ_DIAGNOSTIC) || // 08 Added 2015-Dec-17 by DL
	   (modMsg.functionCode == MOD_READ_SLAVEID) ||   // 17  Added 2015-Dec-15 by DL
       (modMsg.functionCode == MOD_READ_EXTENDED)||   // 20
       (modMsg.functionCode == MOD_WRITE_SINGLE_COIL)||     // 05
       (modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS)||  // 15
       (modMsg.functionCode == MOD_WRITE_HOLDING)||         // 16
       (modMsg.functionCode == MOD_WRITE_SINGLEHOLDING)||   // 06 
       (modMsg.functionCode == MOD_MASKEDWRITE_HOLDING)||   // 22
	   (modMsg.functionCode == MOD_WRITE_EXTENDED) ||       // 21
	   (modMsg.functionCode == MOD_READWRITE_HOLDING)      // 23
      )
   {
      // Check the request length against our PDU size.
      switch (modMsg.functionCode)
      {
      case MOD_READ_COILS:      // 01
      case MOD_READ_DIGITALS:   // 02
         numBytesInReq = modMsg.byteCount/8; // # bits
         break;
	  case MOD_READ_EXCEPTION:  // 07  Added 2015-01-13 by DL
		 modMsg.byteCount = 8;	// # of Bits
         numBytesInReq = (WORD)ceil((double)modMsg.byteCount/8); // # bits
         break;
	  case MOD_READ_SLAVEID:    // 11 Added 2015-Dec-15 by DL
		 modMsg.byteCount = 9;  // Byptes in Reply
		 numBytesInReq = 0;     // No Bytes involved
		 break;
	  case MOD_READ_DIAGNOSTIC: // 08 Added 2015-Dec-17 by DL
		 numBytesInReq = 0;     // No Bytes involved
		 break;
      case MOD_WRITE_MULTIPLE_COILS:  // 0F
         numBytesInReq = (WORD)ceil((double)modMsg.byteCount/8); // # bits
         break;
      case MOD_WRITE_SINGLE_COIL:
         numBytesInReq = 1;
		 modMsg.byteCount = 1;      // Addded 2015-01-11 by DL to correct failure of writing bit 65536
         break;
      default:
         numBytesInReq = modMsg.byteCount*2; // # registers
         break;
      }
      if (numBytesInReq > m_PDUSize)
      {
         MBUSError = TRUE;
         MBUSerrorCode = MOD_EXCEPTION_ILLEGALVALUE;   // too long data field
      }
      else
         MBUSError = FALSE;
   }
   else
   {
      MBUSError = TRUE;
      MBUSerrorCode = MOD_EXCEPTION_ILLEGALFUNC;   // 01
   }

   if (modMsg.m_packError)
   {
      // request message has a corrupted field somewhere
      MBUSError = TRUE;
      MBUSerrorCode = MOD_EXCEPTION_ILLEGALVALUE;   // too long data field
	  //MessageBox(NULL, "Illegal Value", "Error", MB_OK);  // Added for testing by DL & Deleted on 2016-09-14
   }
   
   // 3. build response
   CMODMessage  responseModMsg(modMsg); //Call copy constructor

   //
   // Do some more message validation tests etc.
   //
   if (!MBUSError)
   {
      if ((m_MOSCADchecks)&& // Is a (Analog/holding/extended register)
          ((requestMemArea == 2)||(requestMemArea == 3)||(requestMemArea == 4))
         )
      {
      WORD startTable,endTable;     // table #
      WORD startCol,endCol;         // col #
   
         endTable = MOSCADTABLENUMBER(endRegister); // MOSCAD specify register # on the wire for the formula
         endCol = MOSCADCOLNUMBER(endRegister);
         startTable = MOSCADTABLENUMBER(startRegister);
         startCol = MOSCADCOLNUMBER(startRegister);
         // test that this request does not bridge 2 columns.
         // , else we cannot job/request them together.
         if ((endTable != startTable) ||
             (endCol != startCol))
         {
            MBUSError = TRUE;
            MBUSerrorCode = MOD_EXCEPTION_ILLEGALADDR;   // 02
         }
      }
   }

   if (!MBUSError)
   {
      // if we want to disable all writes
      if ((m_disableWrites) &&
          ((modMsg.functionCode == MOD_WRITE_SINGLE_COIL) ||
           (modMsg.functionCode == MOD_WRITE_SINGLEHOLDING) ||
           (modMsg.functionCode == MOD_MASKEDWRITE_HOLDING) ||
           (modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS) ||
           (modMsg.functionCode == MOD_WRITE_HOLDING) ||
           (modMsg.functionCode == MOD_WRITE_EXTENDED) 
          )
         )
      {
      CString deb;
         MBUSError = TRUE;
         MBUSerrorCode = MOD_EXCEPTION_ILLEGALFUNC;   // 02
         deb.Format("Writing to registers or I/O is disabled!\n"); // Writting=>Writing 2016-12-29 by DL
         OutputDebugString(deb);
         SockDataMessage(deb);
		 if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
      }
   }
   // do a address+length range check too
   // Added tests for WRITE_MULTIPLE_COILS & WRITE_HOLDING on 2017-01-01 by DL
   if ((!MBUSError)&& ((modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS) || (modMsg.functionCode == MOD_WRITE_HOLDING)))
   {
      if (PLCMemory[requestMemArea].GetSize() < endRegister)
      {
         MBUSError = TRUE;
		 // Next line added 2017-01-01 by DL because of bad Exception Code and following two lines commented out
		 MBUSerrorCode = MOD_EXCEPTION_ILLEGALADDR;
         //MBUSerrorCode = (PLCMemory[requestMemArea].GetSize() < startRegister ?
         //                    MOD_EXCEPTION_ILLEGALADDR:MOD_EXCEPTION_ILLEGALVALUE);   // 02:03
      }
   }

   if (MBUSError)
   {
   CString msg;
      msg.Format("Modbus message in error, Exception code x%02X\n", MBUSerrorCode);
      OutputDebugString(msg);
      SockDataMessage(msg);
	  if (m_SaveDebugToFile) CRS232Port::WriteToFile(msg);  // Added statement on 2016-12-26 by DL to save to Log file
   }

   if (pGlobalDialog->m_seperateRegisters)
   {
      seperationOffset = (WORD)(pGlobalDialog->m_numSeperate * (modMsg.stationID-1)); // Added -1 on 2017-01-06 by DL
      if ((PLCMemory[requestMemArea].GetSize() < seperationOffset+endRegister) ||
          (endRegister > pGlobalDialog->m_numSeperate))
      {
         MBUSError = TRUE;
		 // Next line added 2017-01-01 by DL because of bad Exception Code and following two lines commented out
		 MBUSerrorCode = MOD_EXCEPTION_ILLEGALADDR;
         //MBUSerrorCode = (PLCMemory[requestMemArea].GetSize() < seperationOffset + startRegister ?
         //                    MOD_EXCEPTION_ILLEGALADDR:MOD_EXCEPTION_ILLEGALVALUE);   // 02:03
      }
   }
   else
      seperationOffset = 0;


   //
   // Request message seems error free, process it.
   //
                        // 1st 3 bytes + any others up to data get 
                        // added in at this time 
   responseModMsg.BuildMessagePreamble(MBUSError,
                                       MBUSerrorCode);

//   responseModMsg.totalLen = ((WORD)responseModMsg.dataPtr-(LONG)responseModMsg.buffer);

   // If a write is done then the copy constructor will have done all required for ack
   // else a read must now pack into dataPtr onwards! and calc len etc!

   // A read must now pack into dataPtr onwards! and calc len etc!
   //   writes must update our mem areas accordingly.
   if (!MBUSError)
   {
      // 4. fill in the data portion of telegram
      switch (modMsg.functionCode)
      {
	  case MOD_READ_DIAGNOSTIC: // 08 READ DIAGNOSTIC - Start of Function Code 8 Subfunction 00 Changes on 2015-Dec-17 by DL
	  if (modMsg.address == 0)  // 08 Diagnostic Functiod Code 8 with Subfunction Code 00
	  {
		 pDataPortion = responseModMsg.dataPtr;   // Get offset to fill in data
         *(WORD*)pDataPortion = modMsg.address;   // This is Subfunction Code 00 here
         modMsg.dataPtr +=2;   // inc pointer by 2 bytes
         pDataPortion += 2;
		 *(WORD*)pDataPortion = SwapBytes(modMsg.byteCount); // This is the 2-byte Query Data field to be echoed back here
         modMsg.dataPtr +=2;   // inc pointer by 2 bytes
         pDataPortion += 2;
		 // Print Debugging Information
		 deb.Format("Respond to Function Code 8, Subfunction 00 with Echo with %d bytes.\n", 12); // Spelling fix by DL 2016-11-02
		 OutputDebugString(deb);
         SockDataMessage(deb);
		 if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
         break;
	  }                               // End of Function Code 8 Subfunction 00 Changes on 2015-Dec-17 by DL
      case MOD_READ_COILS     : // 01 READ
      case MOD_READ_DIGITALS  : // 02 READ
	  case MOD_READ_EXCEPTION : // 07 READ Added 2015-01-13 by DL
         pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data
         if (MAX_MOD_MEMWORDS >= modMsg.address + modMsg.byteCount)
         {
         WORD memValueTemp;
         WORD bitOffset;

//#ifdef _COMMS_DEBUGGING
		 if (modMsg.functionCode == MOD_READ_COILS)           // Added 2015-01-08 by DL
            deb.Format("Read Output Coils from %d for %d bits.\n", modMsg.address, modMsg.byteCount);// Added 2015-01-08 by DL
		 if (modMsg.functionCode == MOD_READ_DIGITALS)        // Added 2015-01-08 & Modified 2015-01-13 by DL
			 deb.Format("Read Inputs from %d for %d bits.\n", modMsg.address, modMsg.byteCount);// Added 2015-01-08 by DL
//         deb.Format("Read In/output from %d for %d bits.\n", modMsg.address, modMsg.byteCount); // Removed 2015-01-08 by DL
		 if (modMsg.functionCode == MOD_READ_EXCEPTION)      // Added 2015-01-13 by DL
			   deb.Format("Read Outputs as Exception from %d for %d bits.\n", modMsg.address, modMsg.byteCount);// Added 2015-01-13 by DL
		    OutputDebugString(deb);
            SockDataMessage(deb);
			if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
//#endif

            // pack SIM memory: one WORD of sim memory for every BIT in the data.
            // Ie the sim uses one word for each I/O bit in modbus.
            numBytesInReq = modMsg.byteCount/8;
            if (modMsg.byteCount%8)  // if we overflow the byte
               numBytesInReq++;
            for (i=0; i <numBytesInReq;i++)
            {
               // grab the memory now
               memValueTemp = 0;
               for (bitOffset=0;bitOffset<8;bitOffset++)
               {
                  if ((i*8)+bitOffset < modMsg.byteCount)
                     if (PLCMemory[requestMemArea][(seperationOffset + modMsg.address)+(i*8)+bitOffset])
                        memValueTemp += (0x01<<bitOffset);
                     // else bit is off
               }
               *(BYTE*)pDataPortion =  (BYTE)memValueTemp;
               pDataPortion +=1;
            }

         }
         else
         {
            // pack the exception code into the message
            responseModMsg.buffer[1] |= 0x80;
            responseModMsg.buffer[2] = 0x02;    // exception code here (could also use 0x03)
         }
         break;

      case MOD_READ_REGISTERS : 
      case MOD_READ_HOLDING   :
      case MOD_READ_EXTENDED  :
         pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data
         if (MAX_MOD_MEMWORDS >= modMsg.address + modMsg.byteCount)
         {
            WORD memValueTemp;

//#ifdef _COMMS_DEBUGGING
		 if (modMsg.functionCode == MOD_READ_REGISTERS)           // Added 2015-01-09 by DL
            deb.Format("Read Input Regs from %d for %d.\n", modMsg.address, modMsg.byteCount);  // Added 2015-01-09 by DL
		 else                                                 // Added 2015-01-08 by DL
			deb.Format("Read Holding Regs from %d for %d.\n", modMsg.address, modMsg.byteCount);// Added 2015-01-09 by DL
//          deb.Format("Read Register from %d for %d .\n", modMsg.address, modMsg.byteCount);   // Removed 2015-01-09 by DL
            OutputDebugString(deb);
            SockDataMessage(deb);
			if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
//#endif
            for (i=0; i <modMsg.byteCount;i++)
            {
               // grab the memory now
               memValueTemp = PLCMemory[requestMemArea][(seperationOffset + modMsg.address)+i];
               *(WORD*)pDataPortion =  SwapBytes( memValueTemp );
               pDataPortion += 2;
            }
         }
         else
         {
            // pack the exception code into the message
            responseModMsg.buffer[1] |= 0x80;
            responseModMsg.buffer[2] = 0x02;    // exception code here (could also use 0x03)

            deb.Format("Read register past %d error x%02X!\n", MAX_MOD_MEMWORDS, (BYTE)responseModMsg.buffer[2]);
            OutputDebugString(deb);
            SockDataMessage(deb);
			if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
         }
         break;
      default :
          //Writes acks are all built in copy constructor
          //But the update is done here!
          if (MAX_MOD_MEMWORDS >= modMsg.address + modMsg.byteCount)
          {
          // lock memory for writting
          CMemWriteLock lk(PLCMemory.GetMutex());
      
              pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data

              if (!lk.IsLocked())
                 //...Update
                 switch (modMsg.functionCode)
                 { 
                 case MOD_WRITE_SINGLE_COIL     :
                     {
                     CString deb;
                        deb.Format("Write single output coil %d.\n", modMsg.address);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
                     }
                     //data gets copied in now
                     if (m_modifyThenRespond)
                        PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address, (*(WORD*)modMsg.dataPtr?1:0));

                     pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data
                     *pDataPortion++ = (PLCMemory[requestMemArea][(seperationOffset + modMsg.address)+i] ? 0xFF : 0x00);
                     *pDataPortion++ = 0x00;
                     if (!m_modifyThenRespond)
                        PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address, (*(WORD*)modMsg.dataPtr?1:0));
                     
                     numRegs = 1;   // repaint 1 item

                     break;
                 case MOD_WRITE_MULTIPLE_COILS  :
                    // unpack into the SIM memory on WORD of sim memory for every BIT in the data 
                    //WORD numBytes;

                     numBytesInReq = modMsg.count/8;
                     if (modMsg.count%8)  // if we overflow a byte
                        numBytesInReq++;

                     {
                     CString deb;
                        deb.Format("Write multiple outputs coils from %d for %d bits.\n", modMsg.address, modMsg.count);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
                     }
                     numRegs = numBytesInReq * 8;   // repaint X bits // 2015-01-19 Multiplied by 8 to get total BITS by DL

                     for (i=0;i<numBytesInReq;i++)
                     {
                     WORD bitOffset;
                        for (bitOffset=0;bitOffset<8;bitOffset++)
                        {
                           if ((i*8)+bitOffset < modMsg.count)  // Changed <= to only < by DL on 2018-11-27
                           {
                              if (*(BYTE*)modMsg.dataPtr & (0x01<<bitOffset))
                                 PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address+(i*8)+bitOffset, 1);
                              else
                                 PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address+(i*8)+bitOffset, 0);
                           }
                        }
                        modMsg.dataPtr++;
                     }
                     break;
                 case MOD_WRITE_HOLDING : //WRITE multiple holdings
                 case MOD_WRITE_EXTENDED:
                     //PLCMemory[requestMemArea][modMsg.address] = SwapBytes(*(WORD*)modMsg.dataPtr);
                     //break;

                     numRegs = modMsg.byteCount/2;

                     {
                     CString deb;
                        deb.Format("Write multiple registers from %d for %d registers.\n", modMsg.address, numRegs);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
                     }
                     for (i=0;i<numRegs;i++)
                     {
                        PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address + i, SwapBytes(*(WORD*)modMsg.dataPtr));
                        modMsg.dataPtr +=2;   // inc pointer by 2 bytes
                     }
                     break;
                 case MOD_WRITE_SINGLEHOLDING : //WRITE single holding reg.
                     {
                     CString deb;
                     WORD memValueTemp;
                        deb.Format("Write single register %d.\n", modMsg.address);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
                     
                        numRegs = 1;   //repaint 1 register

                        if (m_modifyThenRespond)
                           PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address , SwapBytes(*(WORD*)modMsg.dataPtr));
                        //   PLCMemory.SetAt(requestMemArea,   modMsg.address + i, SwapBytes(*(WORD*)modMsg.dataPtr));
                        memValueTemp = PLCMemory[requestMemArea][(seperationOffset +modMsg.address)];
                        *(WORD*)pDataPortion =  SwapBytes( memValueTemp );

                        if (!m_modifyThenRespond)
                           PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address , SwapBytes(*(WORD*)modMsg.dataPtr));
                        modMsg.dataPtr +=2;   // inc pointer by 2 bytes
                        pDataPortion += 2;
                     }
                     break;
                 case MOD_MASKEDWRITE_HOLDING : // WRITE with MASK single holding reg.
                     {
                     CString deb;
                     WORD memValueTemp, memValueResult;
                     WORD orMask, andMask;

                        deb.Format("Write Mask register %d.\n", modMsg.address);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
                        // fetch it
                        memValueTemp = PLCMemory[requestMemArea][(seperationOffset +modMsg.address)];
                        // do the changes requested
						modMsg.dataPtr -= sizeof(WORD) * 2;			// DL Pointer Added Fix 2014-12-28
                        andMask = SwapBytes(*(WORD*)modMsg.dataPtr);
                        modMsg.dataPtr+= sizeof(WORD);
                        orMask = SwapBytes(*(WORD*)modMsg.dataPtr);
                        modMsg.dataPtr+= sizeof(WORD);
                        memValueResult = ( memValueTemp & andMask) | ( orMask & (~andMask));

                        deb.Format("In=%04X And=%04X Or=%04X Out=%04X.\n", memValueTemp, andMask, orMask, memValueResult);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file

                        numRegs = 1;   //repaint 1 register

                        PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address , memValueResult);  // DL Fix 2014-12-28
						// Changed SwapBytes(memValueResult) to take out the SwapBytes
                     }
                     break;
                 case MOD_READWRITE_HOLDING : // READ & WRITE Holding Registers
		           {
			         memcpy(telegramBuffer, pBuffer, numBytes);
                     telegramBuffer[numBytes] = '\0';
		             deb.Format("Received Function Code 23 Read/Write Holding Registers with %d bytes.\n", numBytes);  // Debug Only
		             OutputDebugString(deb);
                     SockDataMessage(deb);
					 if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);

                     ReadStartHigh = telegramBuffer[8];
                     ReadStartLow = telegramBuffer[9];
                     ReadLengthHigh = telegramBuffer[10];
                     ReadLengthLow = telegramBuffer[11];
                     WriteStartHigh = telegramBuffer[12];
                     WriteStartLow = telegramBuffer[13];
					 WriteLengthHigh = telegramBuffer[14];
					 WriteLengthLow = telegramBuffer[15];
                     WriteDataByteCount = telegramBuffer[16];

                     ReadStartAddress = ReadStartHigh * 256 + ReadStartLow;
					 ReadDataWordCount = ReadLengthHigh * 256 + ReadLengthLow;
					 WriteStartAddress = WriteStartHigh * 256 + WriteStartLow;
					 WriteDataWordCount = WriteLengthHigh * 256 + WriteLengthLow;

					 numRegs = WriteDataByteCount/2;
					 modMsg.address = WriteStartAddress;

					 if ((ReadDataWordCount >=1) && (ReadDataWordCount <= 125) &&
						 (WriteDataWordCount >=1) && (WriteDataWordCount <= 121) &&
						 (WriteDataByteCount == WriteDataWordCount * 2))
                     {
                     if ((MAX_MOD_MEMWORDS >= WriteStartAddress + WriteDataWordCount) &&
						(MAX_MOD_MEMWORDS >= ReadStartAddress + ReadDataWordCount))
					 {
                        CString deb;
                        deb.Format("Write Holding registers from %d for %d registers.\n", WriteStartAddress, WriteDataWordCount);
                        OutputDebugString(deb);
                        SockDataMessage(deb);
						if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);
                        for (i=0;i<WriteDataWordCount;i++)
                        {
                           PLCMemory.SetAt(requestMemArea, seperationOffset + modMsg.address + i, SwapBytes(*(WORD*)modMsg.dataPtr));
                           modMsg.dataPtr +=2;   // inc pointer by 2 bytes
                        }
                        pDataPortion = responseModMsg.dataPtr; //Get offset to fill in data
				        modMsg.address = ReadStartAddress;

				        modMsg.byteCount = ReadDataWordCount ;  // Count is in Words
                       if (MAX_MOD_MEMWORDS >= modMsg.address + ReadDataWordCount)
                       {
                         WORD memValueTemp;

			             deb.Format("Read Holding Regs from %d for %d registers.\n", ReadStartAddress, ReadDataWordCount);
                         OutputDebugString(deb);
                         SockDataMessage(deb);
			             if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
					     *(BYTE*)pDataPortion = modMsg.byteCount * 2 ; // Count is in Words
					     pDataPortion +=1;   // inc pointer by 1 byte
                         for (i=0; i <modMsg.byteCount;i++)
                          {
                            // grab the memory now
                            memValueTemp = PLCMemory[requestMemArea][(seperationOffset + modMsg.address)+i];
                            *(WORD*)pDataPortion =  SwapBytes( memValueTemp );
                            pDataPortion += 2;
                          }
		                }
				        // We need to set this back to get the screen to update correctly
                        numRegs = WriteDataByteCount/2;
					    modMsg.address = WriteStartAddress;
					 }
					 else
					 {
                       // pack the exception code 02 into the message
                       responseModMsg.buffer[1] |= 0x80;   // OR with 80H
                       responseModMsg.buffer[2] = 0x02;    // exception code here
					   pDataPortion ++;                    // Extend write buffer to add error code

                       deb.Format("Read/Write Address Error x%02X!\n", (BYTE)responseModMsg.buffer[2]);
                       OutputDebugString(deb);
                       SockDataMessage(deb);
			           if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);
					 }
			         }
					 else
					 {
                       // pack the exception code 03 into the message
                       responseModMsg.buffer[1] |= 0x80;   // OR with 80H
                       responseModMsg.buffer[2] = 0x03;    // exception code here
					   pDataPortion ++;                    // Extend write buffer to add error code

                       deb.Format("Read/Write Counts Error x%02X!\n", (BYTE)responseModMsg.buffer[2]);
                       OutputDebugString(deb);
                       SockDataMessage(deb);
			           if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);
					 }
		           }
	               break;
	             case MOD_READ_SLAVEID : // READ Slave ID.
	               {
		           pDataPortion = responseModMsg.dataPtr;   // Get offset to fill in data
                   *pDataPortion = 9;   // # of Bytes
                   modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
	               *pDataPortion = 9;   // 9 for 984
	               modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
                   *pDataPortion = 255;   // Running
                   modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
	               *pDataPortion = 1;   // 4k sectors of Page 0 memory
		           modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
                   *pDataPortion = 2;   // 1k sectors of state RAM
                   modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
	               *pDataPortion = 1;   // Count of segments of User Logic
	               modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
                   *pDataPortion = 1;   // Bitmapped
                   modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
		           *pDataPortion = 160; // Bitmapped
		           modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
		           *pDataPortion = 0;   // Bitmapped Error Code
                   modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
		           *pDataPortion = 0;   // Bitmapped Error Code
		           modMsg.dataPtr ++;   // inc pointer
                   pDataPortion ++;
		           // Print Debugging Information
		           deb.Format("Respond to Function Code 17 with %d bytes.\n", 12);
		           OutputDebugString(deb);
                   SockDataMessage(deb);
		           if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
	               }
	               break;
                 }
          }
		  else
		  {
		    // Print Debugging Information
		    deb.Format("Attempted to address beyond register maximum limits with FC = %d.\n", modMsg.functionCode);
		    OutputDebugString(deb);
            SockDataMessage(deb);
		    if (m_CommsDecodeShow) CRS232Port::WriteToFile(deb);
		  }

          // we can only call on the GUI thread once we have un-locked
          if (pGlobalDialog)
          {
			 if (!pGlobalDialog->m_seperateRegisters)			// If not seperate registers redraw only small areas
			 {
             int cols = pGlobalDialog->GetListDisplayedWidth();
             pGlobalDialog->RedrawListItems(modMsg.GetAddressArea(modMsg.functionCode), 
                                            seperationOffset + modMsg.address/(cols),               // Start Row to Redraw
                                            (seperationOffset + modMsg.address+(numRegs-1))/(cols)  // End Row to Redraw
                                           ); // repaint only the needed rows
			 }
			 else												// If seperate registers need to redraw screen
			 {
			  // repaint all of the screen areas registers by DL on 2015-07-18
				pGlobalDialog->RedrawListItems(modMsg.GetAddressArea(modMsg.functionCode),0,6554);
			 }
          }
          break;
      } //end switch
   } 
   else
   { // error occurred
      pDataPortion = responseModMsg.dataPtr; // Get offset to fill in data
   }
   
   // THERE IS no CRC, so do not call BuildMessageEnd
   responseModMsg.totalLen = (WORD)((LONG)pDataPortion-(LONG)responseModMsg.buffer);
//   if (!(m_protocolEthernet))
	if(pGlobalDialog->m_selectedProtocol == PROTOCOL_SELMODETH_RTU) //todo nlrkoi
      responseModMsg.totalLen += MODBUS_CRC_LEN;   // Add CRC






   // insert the frame info
   //responseModMsg
   if (responseModMsg.totalLen > MAX_MOD_MESSAGELENGTH)         // Added on 2015-01-11 by DL because one failure was 13412
      return (FALSE);                         // This was the Bit Write to 65536 that caused a crash in mod_RSsim
//  if (!(m_protocolEthernet)){
	if(pGlobalDialog->m_selectedProtocol == PROTOCOL_SELMODETH_RTU){ //todo nlrkooi
    responseLen = 0 + responseModMsg.totalLen; //SwapBytes(*(WORD*)(responseModMsg.preambleLenPtr)) + 6; //hdr;
    responseModMsg.buffer[2] = 0x00; //set message length to zero  // why [2] and not [3].. (2nd byte?) this is setting 3rd byte to 0.
   // Append the CRC
   responseModMsg.BuildMessageEnd();
  }
  else{
   memmove(&responseModMsg.buffer[ETH_PREAMBLE_LENGTH], responseModMsg.buffer, responseModMsg.totalLen);
   responseLen = ETH_PREAMBLE_LENGTH + responseModMsg.totalLen; //SwapBytes(*(WORD*)(responseModMsg.preambleLenPtr)) + 6; //hdr;
   memset(responseModMsg.buffer, 0 , ETH_PREAMBLE_LENGTH);
   WORD tn = responseModMsg.GetEthernetTransNum();
   *((WORD*)&responseModMsg.buffer[0]) = tn;
   *((WORD*)&responseModMsg.buffer[4]) = SwapBytes(responseModMsg.totalLen);
  }
   // 6. send it back
   m_debuggerStep = 102;
   sprintf(debugStr, "Send %d bytes\n", responseLen);
   OutputDebugString(debugStr);

   SockStateChanged(SOCKETCURRENTLY_WRITTING);

   // disabled in v7.7
   m_NoiseSimulator.m_pSocketObj = (CDDKSrvSocket*)this;
   if (responseModMsg.stationID != 0)					// Added 2015-07-22 by DL to handle Station #0 no response
   transmitted = m_NoiseSimulator.InjectErrors((CDDKSrvSocket*)this, 
                                               openSocket, 
                                               (BYTE*)responseModMsg.buffer, 
                                               responseLen, 
                                               debugStr
                                              );
   else													// Added 2015-07-22 by DL to handle Station #0 no response
   {
	CString description;
	description.Format("Station #0 so no response allowed.\n");
	SockDataMessage(description);
	if (m_CommsDecodeShow) CRS232Port::WriteToFile(description);  // Added statement on 2016-12-26 by DL to save to Log file
   }													// End of Added 2015-07-22 by DL to handle Station #0 no response

   m_debuggerStep = 103;
   if (transmitted)
   {
      // inc counter
      pGlobalDialog->PacketsSentInc();
   }

   // If there are still more bytes in the data we read, process them recursively
   if ((WORD)(modMsg.m_frameLength + 6) < numBytes)
   {
      SockDataMessage("## Processing queued data bytes...");
      SockDataDebugger(&telegramBuffer[modMsg.m_frameLength + 6], numBytes - (modMsg.m_frameLength + 6), dataDebugOther);
      // recursive call using the data from our own stack-space
      ProcessData(openSocket, &telegramBuffer[modMsg.m_frameLength + 6], numBytes - (modMsg.m_frameLength + 6));
   }
   return (TRUE);
}

// ----------------------------- LoadRegisters -----------------------------
// load binary dump of the register values from file.
BOOL CMODEthCommsProcessor::LoadRegisters()
{
BOOL retstatus;		// Return Value - True = Good. Added 2016-08-15 by DL
   // This complete function was revised to allow handling of an error message from the SaveRegistersIMP call by DL on 2016-08-15
   CString ret = CMOD232CommsProcessor::LoadRegistersIMP();

   if (ret=="")
   {
      SockDataMessage("Register values loaded OK\n");
	  if (m_SaveDebugToFile) CRS232Port::WriteToFile("Register values loaded OK\n");  // Added statement on 2016-12-26 by DL to save to Log file
	  retstatus = TRUE;
   }
   else
   {
	  SockDataMessage(ret);
	  //AfxMessageBox(description, MB_OK|MB_ICONINFORMATION);
	  AfxMessageBox(ret, MB_OK|MB_ICONINFORMATION);
	  retstatus = FALSE;
   }
   //return (ret);
   return retstatus;
   // This is the end of the changes for handling the error message from the SaveRegistersIMP call by DL on 2016-08-15
} // LoadRegisters

// ----------------------------- SaveRegisters -----------------------------
// Save binary dump of the register values from file.
BOOL CMODEthCommsProcessor::SaveRegisters()
{
BOOL retstatus;		// Return Value - True = Good. Added 2016-08-15 by DL
   // This complete function was revised to allow handling of an error message from the SaveRegistersIMP call by DL on 2016-08-15
   CString ret = CMOD232CommsProcessor::SaveRegistersIMP();

   if (ret=="")
   {
      SockDataMessage("Register values saved OK\n");
	  if (m_SaveDebugToFile) CRS232Port::WriteToFile("Register values saved OK\n");  // Added statement on 2016-12-26 by DL to save to Log file
	  retstatus = TRUE;
   }
   else
   {
	  SockDataMessage(ret);
	  AfxMessageBox(ret, MB_OK|MB_ICONINFORMATION);
	  retstatus=FALSE;
   }
   return (retstatus);
   // This is the end of the changes for handling the error message from the SaveRegistersIMP call by DL on 2016-08-15
} // SaveRegisters

// -------------------------- SetEmulationParameters ------------------------
void CMODEthCommsProcessor::SetEmulationParameters(BOOL moscadChecks, 
                                                BOOL modifyThenRespond, 
                                                BOOL disableWrites)
{
   m_MOSCADchecks = moscadChecks;
   m_modifyThenRespond = modifyThenRespond;
   m_disableWrites = disableWrites;
}

//#define BYTESWAP(w)         ((WORD)(((BYTE)(((DWORD_PTR)(LOBYTE(w))) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(HIBYTE(w))) & 0xff))) << 8))
#define BYTESWAP(w)         ((WORD)(((BYTE)(((DWORD_PTR)(HIBYTE(w))) & 0xff)) | ((WORD)((BYTE)(((DWORD_PTR)(LOBYTE(w))) & 0xff))) << 8))




// SIM protocol frame:
// <PLCMemoryArea>  0 = OUTPUT , INPUT ...  //word
// <ActionCode>  similar to function code   //word 1 read / 2 = write
// <WordCount> number of bytes to read or write //word
// <Adress>  //word
// <Data> data to write
// ----------------------- constructor -----------------------------------
CSIMMessage::CSIMMessage(const CHAR * pMessageRX, DWORD len)
{
   BYTE *pTelePtr;
   m_packError = FALSE;
   
   // break it down
   pTelePtr = (BYTE*)pMessageRX;
   totalLen = (WORD)len;
   bitCount = 0;
   byteCount = 0;

   actionCode = BYTESWAP(*(WORD*)pTelePtr);
   pTelePtr += sizeof(WORD);
   address = BYTESWAP(*(WORD*)pTelePtr);
   pTelePtr += sizeof(WORD);

   switch (actionCode)
   {
      case SIM_WRITE_COILS:
          bitCount = UnPackField(&pTelePtr, 2); // 2 chars, the count is in bits
          byteCount = (WORD)ceil(bitCount/8.0); //bytes to  write
         overalLen = byteCount;
         coilByteCount = *pTelePtr++; // increment past the #bytes byte which is the # bytes of data to expect (max 255) UnPackField also incriments pointer
         overalLen += 4;  // length message = actionCode (2) + adress (2)(bytes to wrtie) 
         break;
      case SIM_READ_COILS  : 
      case SIM_READ_DIGITALS  : 
      case SIM_READ_REGISTERS : 
      case SIM_READ_HOLDING   :
         // byteCount= # bytes to read
         wordCount = UnPackField(&pTelePtr, 2); // 2 chars, the count is in REGISTERS
         overalLen = 2;
    //     m_msgLenOK = TRUE;     // Added 2015-01-25 by DL-Assume good length since short
         break;
      case SIM_WRITE_REGISTERS:
      case SIM_WRITE_HOLDING:
         // byteCount=# bytes if a write, else # bytes to read
         wordCount = UnPackField(&pTelePtr, 2)*2; // 2 chars, the count is in bytes
         overalLen = wordCount;
		 regByteCount = *pTelePtr; // Added 2017-01-01 by DL for testing bad BYTE count
         pTelePtr++; // increment past the #bytes byte which is the # bytes of data to expect (max 255)
         overalLen+=3;  // skip the 3 bytes for the req. size (byte) and length/quantity word
//		 if (len == byteCount + 9)        // Added 2015-01-25 by DL to handle bad CRC
//             m_msgLenOK = TRUE;           // Added 2015-01-25 by DL to handle bad CRC
//         else                             // Added 2015-01-25 by DL to handle bad CRC
//             m_msgLenOK = FALSE;          // Added 2015-01-25 by DL to handle bad CRC
         break;
	  default   : //All other commands not supported
         //ASSERT (0);
         overalLen = 0;
         byteCount = 0;
         break;
   }

   //Now  (at last) pTelePtr points to the data to read/write
   dataPtr = pTelePtr;  // data starts here


} // CSIMMessage

// --------------------------- CSIMMessage --------------------------
// PURPOSE: copy constructor used to build responses, does not actually 
// copy the message.
CSIMMessage::CSIMMessage(const CSIMMessage & oldMODMessage) 
{
   m_packError = FALSE;

   //Copy in common stuff from both messages here!
   this->actionCode = oldMODMessage.actionCode;
   this->address = oldMODMessage.address;       // where to copy data from
   this->byteCount = oldMODMessage.byteCount;   // length of data to copy
   this->wordCount = oldMODMessage.wordCount;   // length of data to copy
   this->bitCount = oldMODMessage.bitCount;   // length of data to copy
   this->dataPtr = (BYTE*)buffer; //Nice an fresh pointer to the beginning!
}

// ------------------------------ BuildMessagePreamble -------------------------
// PURPOSE: Builds the actionCode,PLCMemoryArea and LEN words of the telegram.
// on completion dataPtr pointsto where the data must be packed in (if any)
CHAR * CSIMMessage::BuildMessagePreamble(BOOL error, WORD errorCode)
{
    
	BYTE *pWorkArea;
	BYTE numBytesData;

   //
   pWorkArea = (BYTE*)buffer;
   if (error)
   { // error flag 80 + error meaning byte
      *pWorkArea++ = (BYTE)(0x80);
      *pWorkArea++ = (BYTE)LOBYTE(address);
      *pWorkArea++ = (BYTE)errorCode;
   }
   else
   {
      // normal processing
   *pWorkArea++ = HIBYTE(actionCode);
   *pWorkArea++ = LOBYTE(actionCode);
      switch (actionCode)
      {
          case SIM_WRITE_REGISTERS        : 
          case SIM_WRITE_HOLDING        : 
             // HF fixed the return address.
             *pWorkArea++ = HIBYTE(address);   
             *pWorkArea++ = LOBYTE(address);   
             *pWorkArea++ = HIBYTE(byteCount/2);
             *pWorkArea++ = LOBYTE(byteCount/2);
             break;
          case SIM_READ_DIGITALS  : // in
          case SIM_READ_COILS     : // out
             numBytesData = (BYTE)ceil((float)byteCount/8.0);  // # registers*2
             *pWorkArea++ = numBytesData; 
             break;
          case SIM_READ_REGISTERS : 
          case SIM_READ_HOLDING   :
             numBytesData = byteCount*2;  // # registers*2  TODO  what?? is numBytesData = numOfRegisters?
             *pWorkArea++ = numBytesData; 
             break;
      }
   }   
   dataPtr = pWorkArea; // must now point to 1st byte of data


   return (buffer);
} // BuildMessagePreamble



// ------------------------------ GetAddressArea --------------------
// Returns:    A supported MEM area index for any MOD address class
// Parameter:  A modbus command (e.g. 3 =read holding register)
//
WORD CSIMMessage::GetAddressArea(WORD classCode //  modbus command byte
                                )
{
   switch(classCode)
   {
      // read commands 
      case SIM_READ_COILS      : return(0); break;
      case SIM_READ_DIGITALS   : return(1); break;
      case SIM_READ_REGISTERS  : return(2); break;   
      case SIM_READ_HOLDING    : return(3); break;
      // write commands      
      case SIM_WRITE_COILS      : return(0); break;
      case SIM_WRITE_DIGITALS   : return(1); break;
      case SIM_WRITE_REGISTERS  : return(2); break;   
      case SIM_WRITE_HOLDING    : return(3); break;
   }
   return(9); //Default here for now, Should never get here anyway!  (9 = error)

} // GetAddressArea



void CSIMMessage::SockDataMessage(LPCTSTR msg)
{
   EnterCriticalSection(&stateCS);
   OutputDebugString("##");
   if (NULL!=pGlobalDialog)
      pGlobalDialog->AddCommsDebugString(msg);
   LeaveCriticalSection(&stateCS);
   
   CString msg2;
   msg2.Format("From CSIMMessage::SockDataMessage \n");
   if (NULL!=pGlobalDialog)
      pGlobalDialog->AddCommsDebugString(msg2);
   LeaveCriticalSection(&stateCS);
   
}