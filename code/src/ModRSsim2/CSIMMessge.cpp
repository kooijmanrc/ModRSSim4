

// SIM protocol frame:
// <PLCMemoryArea>  0 = OUTPUT , INPUT ...  //word
// <ActionCode>  similar to function code   //word 1 read / 2 = write
// <WordCount> number of bytes to read or write //word
// <Adress>  //word
// <Data> data to write
// ----------------------- constructor -----------------------------------
CSIMMessage::CSIMMessage(const CHAR * pMessageRX, DWORD len)
{
   WORD *pTelePtr;

   m_packError = FALSE;
   
   // break it down
   pTelePtr = (WORD*)pMessageRX;
   totalLen = (WORD)len;
   count = 0;
   actionCode = *(WORD*)pTelePtr;  
   pTelePtr += sizeof(WORD);
   address = *(WORD*)pTelePtr;
   pTelePtr += sizeof(WORD);
   dataPtr = (BYTE*)pTelePtr;



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
   this->dataPtr = (BYTE*)buffer; //Nice an fresh pointer to the beginning!
}

// ------------------------------ BuildMessagePreamble -------------------------
// PURPOSE: Builds the actionCode,PLCMemoryArea and LEN words of the telegram.
// on completion dataPtr pointsto where the data must be packed in (if any)
CHAR * CSIMMessage::BuildMessagePreamble(BOOL error, WORD errorCode)
{
    
	WORD *pWorkArea;
	WORD numBytesData;

   //
   pWorkArea = (WORD*)buffer;
   *pWorkArea++ = (WORD)actionCode;
   *pWorkArea++ = (WORD)address;   
   dataPtr = (BYTE*)pWorkArea; // must now point to 1st byte of data


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
   return(3); //Default here for now, Should never get here anyway!

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