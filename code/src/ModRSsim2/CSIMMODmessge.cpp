/////////////////////////////////////////////////////////////////////////////
//
// ------------------------------ BuildMessagePreamble -------------------------
// PURPOSE: Builds the STN,FN and LEN bytes of the telegram.
// on completion dataPtr pointsto where the data must be packed in (if any)
CHAR * CMODMessage::BuildMessagePreamble(BOOL error, WORD errorCode)
{
BYTE *pWorkArea;
BYTE numBytesData;

   //
   pWorkArea = (BYTE*)buffer;
   *pWorkArea++ = (BYTE)stationID;
   if (error)
   { // error flag 80 + error meaning byte
      *pWorkArea++ = (BYTE)(functionCode|0x80);
      *pWorkArea++ = (BYTE)errorCode;
   }
   else
   {
      // normal processing
      *pWorkArea++ = (BYTE)functionCode;
      switch (functionCode)
      {
          case MOD_WRITE_HOLDING        : 
          case MOD_WRITE_EXTENDED       : 
             // HF fixed the return address.
             *pWorkArea++ = HIBYTE(address);
             *pWorkArea++ = LOBYTE(address);
             *pWorkArea++ = HIBYTE(byteCount/2);
             *pWorkArea++ = LOBYTE(byteCount/2);
             break;
          case MOD_WRITE_SINGLEHOLDING :
             *pWorkArea++ = HIBYTE(address); // CDB fixed return address rev 7.0
             *pWorkArea++ = LOBYTE(address);
             break;
          case MOD_MASKEDWRITE_HOLDING:
             *pWorkArea++ = HIBYTE(address);
             *pWorkArea++ = LOBYTE(address);
             //add the masks
             *pWorkArea++ = HIBYTE(m_andMask);
             *pWorkArea++ = LOBYTE(m_andMask);
             *pWorkArea++ = HIBYTE(m_orMask);
             *pWorkArea++ = LOBYTE(m_orMask);

             break;
          case MOD_WRITE_MULTIPLE_COILS : 
             *pWorkArea++ = HIBYTE(address);
             *pWorkArea++ = LOBYTE(address);
             *pWorkArea++ = HIBYTE(byteCount);  // # bits actually
             *pWorkArea++ = LOBYTE(byteCount);

             break;
          case MOD_WRITE_SINGLE_COIL    :
             *pWorkArea++ = HIBYTE(address);  // fixed thanks to Joan Lluch-Zorrilla
             *pWorkArea++ = LOBYTE(address);
             break;
          case MOD_READ_DIGITALS  : // in
          case MOD_READ_COILS     : // out
             numBytesData = (BYTE)ceil((float)byteCount/8.0);  // # registers*2
             *pWorkArea++ = numBytesData; 
             break;
         case MOD_READ_EXCEPTION : // 07 Added 2015-01-13 by DL
         case MOD_READ_DIAGNOSTIC : // 08 Added 2015-Dec-03 by DL
		 case MOD_READ_SLAVEID :    // 11 Added 2015-Dec-15 by DL
             break;                // 07 End of Add 2015-01-20 by DL
          case MOD_READ_REGISTERS : 
          case MOD_READ_HOLDING   :
          case MOD_READ_EXTENDED  : 
             numBytesData = byteCount*2;  // # registers*2
             *pWorkArea++ = numBytesData; 
             break;
      }
   }   
   dataPtr = pWorkArea; // must now point to 1st byte of data

   return (buffer);
} // BuildMessagePreamble

// ----------------------------- SetEthernetFrames --------------------------
// supply FALSE for normal serial 232 frames
BOOL CMODMessage::SetEthernetFrames(BOOL ethernetFrames/* = TRUE*/)
{
BOOL oldV = m_protocolEthernet;
   m_protocolEthernet = ethernetFrames;
   return (m_protocolEthernet);
}

// ------------------------------ BuildMessageEnd -------------------------------
// PURPOSE: glue a CRC onto the end of the message
// totalLen must be = the full telegram length (+CRC) when this is called.
CHAR * CMODMessage::BuildMessageEnd()
{
WORD length;
BYTE *pCrcStart = (BYTE*)buffer;
WORD crc = 0xFFFF;
BYTE *crcPtr;

   // Add the CRC bytes
   length = totalLen - MODBUS_CRC_LEN; //calc the CRC of all bytes but the 2 CRC bytes

   CalcCRC(pCrcStart, length, &crc);
   crcPtr = (BYTE*)&buffer[length];
   *(WORD *)crcPtr = crc;

   return (buffer);
} // BuildMessageEnd


// ------------------------------ GetAddressArea --------------------
// Returns:    A supported MEM area index for any MOD address class
// Parameter:  A modbus command (e.g. 3 =read holding register)
//
WORD CMODMessage::GetAddressArea(WORD classCode //  modbus command byte
                                )
{
   switch(classCode)
   {
      // read commands 
      case MOD_READ_COILS      : return(0); break;
      case MOD_READ_DIGITALS   : return(1); break;
      case MOD_READ_REGISTERS  : return(2); break;   
      case MOD_READ_HOLDING    : return(3); break;
      case MOD_READ_EXTENDED   : return(4); break;
      case MOD_READ_EXCEPTION  : return(0); break;      // Added 2015-01-13 by DL
	  case MOD_READ_SLAVEID    : return(0); break;      // Added 2015-Dec-15 by DL
      case MOD_READ_DIAGNOSTIC : return(0); break;      // Added 2015-Dec-19 by DL
      // write commands      
      case MOD_WRITE_HOLDING        : return(3); break;
      case MOD_WRITE_SINGLEHOLDING  : return(3); break;
      case MOD_MASKEDWRITE_HOLDING  : return(3); break;
      case MOD_WRITE_SINGLE_COIL    : return(0); break;
      case MOD_WRITE_MULTIPLE_COILS : return(0); break;
      case MOD_WRITE_EXTENDED       : return(4); break;
   }
   return(3); //Default here for now, Should never get here anyway!

} // GetAddressArea

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
IMPLEMENT_DYNAMIC( CMOD232CommsProcessor, SimulationSerialPort);

//////////////////////////////////////////////////////////////////////
// constructor to open port
CMOD232CommsProcessor::CMOD232CommsProcessor(LPCTSTR portNameShort, 
                                     DWORD  baud, 
                                     DWORD byteSize, 
                                     DWORD parity, 
                                     DWORD stopBits,
                                     DWORD rts,
                                     int   responseDelay,
                                     BOOL  MOSCADchecks,
                                     BOOL modifyThenRespond,
                                     BOOL disableWrites,
									 BOOL longTimeouts) : SimulationSerialPort()
{
CString description;
 m_protocolName = "MODBUS RTU";

   InitializeCriticalSection(&stateCS);
   
   m_noiseLength = 0;
   
   description.Format("Starting comms emulation : %s", "MODBUS RS-232");
   RSDataMessage(description);
   if (m_SaveDebugToFile) CRS232Port::WriteToFile(description);  // Added statement on 2016-12-26 by DL to save to Log file
   
   // open the port etc...
   if (OpenPort(portNameShort))
   {
      ConfigurePort(baud, byteSize, parity, stopBits, rts, (NOPARITY==parity?FALSE:TRUE), longTimeouts);
   }
   m_responseDelay = responseDelay;

   SetEmulationParameters(MOSCADchecks, modifyThenRespond, disableWrites);
   m_pWorkerThread->ResumeThread(); //start thread off here

}


CMOD232CommsProcessor::~CMOD232CommsProcessor()
{

}

void CMOD232CommsProcessor::SetEmulationParameters(BOOL moscadChecks, 
                                                BOOL modifyThenRespond, 
                                                BOOL disableWrites)
{
   m_MOSCADchecks = moscadChecks;
   m_modifyThenRespond = modifyThenRespond;
   m_disableWrites = disableWrites;
}

// ------------------------------ RSDataDebugger ------------------------------
void CMOD232CommsProcessor::RSDataDebugger(const BYTE * buffer, LONG length, int transmit)
{
   CRS232Port::GenDataDebugger(buffer,length,transmit);
} // RSDataDebugger

// ------------------------------- RSStateChanged -----------------------
void CMOD232CommsProcessor::RSStateChanged(DWORD state)
{
   EnterCriticalSection(&stateCS);
   if (NULL==pGlobalDialog)
      return;
   pGlobalDialog->m_ServerRSState = state;
   LeaveCriticalSection(&stateCS);
} // RSStateChanged

// ------------------------------ RSDataMessage ------------------------------
void CMOD232CommsProcessor::RSDataMessage(LPCTSTR msg)
{
   EnterCriticalSection(&stateCS);
   OutputDebugString("##");
   if (NULL!=pGlobalDialog)
      pGlobalDialog->AddCommsDebugString(msg);
   LeaveCriticalSection(&stateCS);
}

// ------------------------------- RSModemStatus ---------------------------
void CMOD232CommsProcessor::RSModemStatus(DWORD status)
{
   EnterCriticalSection(&stateCS);
   if (NULL!=pGlobalDialog)
      pGlobalDialog->SendModemStatusUpdate(status);
   LeaveCriticalSection(&stateCS);
}


// ------------------------------ OnProcessData --------------------------------
BOOL CMOD232CommsProcessor::OnProcessData(const CHAR *pBuffer, DWORD numBytes, BOOL *discardData)
{
	if (*discardData && m_noiseLength)     // Added 2015-01-25 by DL to discard data after timeout
	{                                      // Added 2015-01-25 by DL
	   *discardData = TRUE;     // Added 2015-01-25 by DL
       m_noiseLength = 0;       // Added 2015-01-25 by DL
 
       CString msg;             // Added 2015-01-25 by DL
       msg.Format("Timeout on Message Receive. Dumping Data.\n");  // Added 2015-01-25 by DL
       OutputDebugString(msg);  // Added 2015-01-25 by DL
       RSDataMessage(msg);      // Added 2015-01-25 by DL
       if (m_SaveDebugToFile) CRS232Port::WriteToFile(msg);  // Added statement on 2016-12-26 by DL to save to Log file
       return(FALSE);           // Added 2015-01-25 by DL
	}                           // Added 2015-01-25 by DL - End of section to discard data after timeout
	// build noise telegram
   if (numBytes)
   { //append recieved bytes to the noise telegram and m_noiseLength if the length of the Accumulator buffer is OK
      if (m_noiseLength + numBytes >= sizeof(m_noiseBuffer))
      {
         RSDataMessage("OVERFLOW:Restarting interpretation.");
		 // Added statement on 2016-12-26 by DL to save to Log file
         if (m_SaveDebugToFile) CRS232Port::WriteToFile("OVERFLOW:Restarting interpretation.");

         m_noiseLength = 0;
         //SetEngineState(ENG_STATE_IDLE);
         return(TRUE);
      }
      memcpy(&m_noiseBuffer[m_noiseLength], pBuffer, numBytes);
      m_noiseLength += numBytes;
      *discardData = TRUE;
   }

//   if ((m_noiseLength < MODBUS_NORMAL_LEN)              // 2015-01-25 Deleted by DL for new check including FC7 below
   if ((numBytes == m_noiseLength) && (numBytes >=2))                                // If first bytes then try to get Function Code
   FC = (WORD)pBuffer[1];                                                            // Get Byte in buffer that is Function Code
   //if ((m_noiseLength < MODBUS_NORMAL_LEN) && (m_noiseLength == 4) && !(FC == 7))  // Test for Length unless FC7 or FV17 and len=4
   //if ((m_noiseLength < MODBUS_NORMAL_LEN) && !((FC == 17) && (m_noiseLength == 4)))// Test for Length unless FC7 and len=4
   if ((m_noiseLength < MODBUS_NORMAL_LEN) && !(((FC == 7) || (FC == 17)) && (m_noiseLength == 4)))// Test for Length unless FC7 and len=4
      return(FALSE);

   CMODMessage::SetEthernetFrames(FALSE);
   CMODMessage msg((char*)m_noiseBuffer, m_noiseLength);
   if (msg.CRCOK())
   {
   BOOL ret;
      // build a response etc
      ret = ProcessData((char*)m_noiseBuffer, msg.overalLen + 2);   //+2 for the CRC
      m_noiseLength = 0;                       // Resets the Accumulator buffer length
      return(ret);
   }
   else
   {
      // try to strip away leading byte "noise"?
// comment out the following 2 lines, from spaccabbomm [beta@mmedia.it]
/*
      m_noiseLength--;
      memmove(m_noiseBuffer, &m_noiseBuffer[1], m_noiseLength);
*/

   if (!m_msgLenOK)         // Added 2015-01-25 by DL to handle bad CRC messages
      return(TRUE);         // Added 2015-01-25 by DL
   else                     // Added 2015-01-25 by DL
   {                        // Added 2015-01-25 by DL
   *discardData = TRUE;     // Added 2015-01-25 by DL
   m_noiseLength = 0;       // Added 2015-01-25 by DL
 
   CString msg;             // Added 2015-01-25 by DL
   msg.Format("Malformed Modbus message for Function Code = x%02X\n", // Added 2015-01-25 by DL
	     (unsigned int)(FC & 0xFF));  // Revised for only two hex digits on 2016-12-28 by DL
   OutputDebugString(msg);  // Added 2015-01-25 by DL
   RSDataMessage(msg);      // Added 2015-01-25 by DL
   if (m_CommsDecodeShow) CRS232Port::WriteToFile(msg);  // Added statement on 2016-12-26 by DL to save to Log file

   return(FALSE);           // Added 2015-01-25 by DL - End of bad CRC message handling
   }

   }
   *discardData = FALSE;
   return(FALSE);
}

// --------------------------------- TestMessage ------------------------
//
BOOL CMOD232CommsProcessor::TestMessage(CMODMessage &modMsg, 
                                        WORD &startRegister, 
                                        WORD &endRegister, 
                                        WORD &MBUSerrorCode,
                                        WORD &requestMemArea,
                                        WORD &numBytesInReq
                                        )
{
BOOL MBUSError = FALSE;

   if (!modMsg.CRCOK())
   {
      // bail
   }

   //Get memory area which to update or retrieve from
   requestMemArea = modMsg.GetAddressArea(modMsg.functionCode);
   if (requestMemArea >= MAX_MOD_MEMTYPES)
   {
      // TO DO!
      // handle the error
      Beep(2000,200);
      requestMemArea = 3;  // for now just default to "Holding" for now!
   }

   // validate the request is a valid command code
   startRegister = modMsg.address;
   if ((MOD_WRITE_SINGLE_COIL == modMsg.functionCode) || (MOD_READ_EXCEPTION == modMsg.functionCode) // 2015-07-18 DL
	   || (MOD_READ_SLAVEID == modMsg.functionCode) || (MOD_READ_DIAGNOSTIC == modMsg.functionCode)) // Added 2016-12-28 by DL
	   endRegister = startRegister;					// 2015-07-18 DL
   else												// 2015-07-18 DL
   //endRegister = startRegister + modMsg.byteCount;    // Previously deleted by CDB (per DL on 2016-12-28)
   //endRegister = startRegister + modMsg.byteCount/2;  // CDB rev 7.0 // Deleted 2016-12-28 by DL

   {                                                          // Code section added 2016-12-28 by DL
	   if ((modMsg.functionCode == MOD_READ_COILS)||          // 01
           (modMsg.functionCode == MOD_READ_DIGITALS)||       // 02
		   (modMsg.functionCode == MOD_READ_EXCEPTION) ||     // 07
		   (modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS)) // 15
		   endRegister = startRegister + modMsg.byteCount;    // Old code was correct for bits but not regs
	   else
           endRegister = startRegister + modMsg.byteCount/2;  // Had rev to "/2" previously for all cases but correct for regs only
   }                                                          // End of Code section added 2016-12-28 by DL

   if ((modMsg.functionCode == MOD_READ_COILS)||       // 01
       (modMsg.functionCode == MOD_READ_DIGITALS)||    // 02
       (modMsg.functionCode == MOD_READ_REGISTERS)||   // 04
       (modMsg.functionCode == MOD_READ_HOLDING)||     // 03
       (modMsg.functionCode == MOD_READ_EXCEPTION) ||  // 07  // Added 2015-01-13 by DL
       (modMsg.functionCode == MOD_READ_DIAGNOSTIC) || // 08  // Added 2015-Dec-03 by DL
       (modMsg.functionCode == MOD_READ_EXTENDED)||    // 20
	   (modMsg.functionCode == MOD_READ_SLAVEID) ||    // 17  // Added 2015-Dec-15 by DL
       (modMsg.functionCode == MOD_WRITE_SINGLE_COIL)||     // 05
       (modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS)||  // 0F
       (modMsg.functionCode == MOD_WRITE_HOLDING)||         // 10
       (modMsg.functionCode == MOD_WRITE_SINGLEHOLDING)||   // 06
       (modMsg.functionCode == MOD_MASKEDWRITE_HOLDING)||   // 22
       (modMsg.functionCode == MOD_WRITE_EXTENDED) ||       // 21
	   (modMsg.functionCode == MOD_READWRITE_HOLDING)      // 23
      )
   {
	     CString deb;
         deb.Format("m_PDUSize is %d \n", m_PDUSize);
         OutputDebugString(deb);
         RSDataMessage(deb);


      // Check the request length against our PDU size.
      if ((modMsg.functionCode == MOD_READ_COILS)||      // 01
          (modMsg.functionCode == MOD_READ_DIGITALS)||   // 02
		  (modMsg.functionCode == MOD_READ_EXCEPTION) ||  // 07   Added 2015-01-13 by DL
          (modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS))  // 0F
         numBytesInReq = modMsg.byteCount/8; // # bits
      else
         numBytesInReq = modMsg.byteCount*2; // # registers
	  // Added "+5" on 2017-01-02 by DL to account for the 3 beginning bytes (ID, FC, Count) and 2 CRC ending bytes
	  // Also added cast "(DWORD)" to avoid compiler C4018 warning on 2017-01-02 by DL
      if (((DWORD)numBytesInReq + 5 > m_PDUSize * 2) && (modMsg.functionCode != MOD_READ_DIAGNOSTIC)) // Added MOD_READ_DIAGNOSTIC check 2015-Dec-03 by DL
      {
         MBUSError = TRUE;
         MBUSerrorCode = MOD_EXCEPTION_ILLEGALVALUE;   // too long data field (error > 62)
      }
      else
         MBUSError = FALSE;
   }
   else
   {
      MBUSError = TRUE;
      MBUSerrorCode = MOD_EXCEPTION_ILLEGALFUNC;   // 01
   }
   
/*   if (modMsg.m_packError)
   {
      // request message has a corrupted field somewhere
      MBUSError = TRUE;
      MBUSerrorCode = MOD_EXCEPTION_ILLEGALVALUE;   // too long data field
   }*/

   // 3. build response

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
      deb.Format("Writing data or I/O currently disabled, see Advanced Settings!\n");
      OutputDebugString(deb);
      RSDataMessage(deb);
	  if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
   }

   if (modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS) 
   {
      if (modMsg.coilByteCount != (modMsg.overalLen-7))
      {    
      CString deb;
         MBUSError = TRUE;
		 // Next line changed 2017-01-01 by DL
         MBUSerrorCode = MOD_EXCEPTION_ILLEGALADDR;   // 02 // Was MOD_EXCEPTION_ILLEGALFUNC should be MOD_EXCEPTION_ILLEGALADDR
         deb.Format("Invalid BYTE Count. Modbus Exception Code 02h\n");
         OutputDebugString(deb);
         RSDataMessage(deb);
		 if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);  // Added statement on 2016-12-26 by DL to save to Log file
		 return(MBUSError);  // Added 2017-01-01 by DL because we do not want the final Exception Code message to appear from here.
      }
   }

   if (modMsg.functionCode == MOD_WRITE_HOLDING)   // Section added 2017-01-01 by DL to detect bad value in write multiple holding regs
   {
      if (modMsg.regByteCount != (modMsg.overalLen-7))
      {    
      CString deb;
         MBUSError = TRUE;
		 // Next line changed 2017-01-01 by DL
         MBUSerrorCode = MOD_EXCEPTION_ILLEGALVALUE;   // 03 
         deb.Format("Invalid BYTE Count. Modbus Exception Code 03h\n");
         OutputDebugString(deb);
         RSDataMessage(deb);
		 if (m_SaveDebugToFile) CRS232Port::WriteToFile(deb);
		 return(MBUSError);
      }
   }                                               // End of Section added 2017-01-01 by DL

   // do an address+length range check too
   // Added tests for WRITE_MULTIPLE_COILS & WRITE_HOLDING on 2017-01-01 by DL
   if ((!MBUSError)&& ((modMsg.functionCode == MOD_WRITE_MULTIPLE_COILS) || (modMsg.functionCode == MOD_WRITE_HOLDING)))
      if ((PLCMemory[requestMemArea].GetSize() < endRegister) && (modMsg.functionCode != MOD_READ_DIAGNOSTIC)) // Added MOD_READ_DIAGNOSTIC check 2015-Dec-03 by DL
      {
         MBUSError = TRUE;
		 // Next line added 2017-01-01 by DL because of bad Exception Code and following two lines commented out
		 MBUSerrorCode = MOD_EXCEPTION_ILLEGALADDR;
         //MBUSerrorCode = (PLCMemory[requestMemArea].GetSize() < startRegister ?
         //                    MOD_EXCEPTION_ILLEGALADDR:MOD_EXCEPTION_ILLEGALVALUE);   // 02:03
      }

   if (MBUSError)
   {
   CString msg;
      msg.Format("Modbus message in error. Exception Code= x%02X\n", MBUSerrorCode);
      OutputDebugString(msg);
      RSDataMessage(msg);
	  if (m_CommsDecodeShow) CRS232Port::WriteToFile(msg);  // Added statement on 2016-12-26 by DL to save to Log file
   }
   return(MBUSError);
}

