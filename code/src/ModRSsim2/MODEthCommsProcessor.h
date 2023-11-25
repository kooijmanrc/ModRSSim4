// MODCommsProcessor.h: interface for the CMODEthCommsProcessor class.
//
// Ethernet :
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TICOMMSPROCESSOR_H__FDE99A84_435C_4094_826C_175DBDB6E61F__INCLUDED_)
#define AFX_TICOMMSPROCESSOR_H__FDE99A84_435C_4094_826C_175DBDB6E61F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DDKSrvSocket.h"

#define MAX_RX_MESSAGELENGTH   4096     // a big buffer

// reads
#define SIM_READ_COILS           0x0001     // protocol cmd 1
#define SIM_READ_DIGITALS        0x0002     // command 2 (read inputs)
#define SIM_READ_HOLDING         0x0003
#define SIM_READ_REGISTERS       0x0004     // analog input registers
// writes
#define SIM_WRITE_COILS          0x0005     // un-implemented?
#define SIM_WRITE_DIGITALS       0x0006     // tested, preset single register
#define SIM_WRITE_HOLDING        0x0007
#define SIM_WRITE_REGISTERS      0x0008

#define SIM_FRAME_LENGTH_MAX 4 //ACTION CODE + STARTARDRESS 

extern BOOL  m_CommsDecodeShow;			// Added 2016-12-28 by DL to allow use outside mod_RSsimDLG

class CMODEthCommsProcessor : public CDDKSrvSocket  
{
public:
	CMODEthCommsProcessor(int responseDelay,
                         BOOL  MOSCADchecks,
                         BOOL modifyThenRespond,
                         BOOL disableWrites,
                         LONG PDUSize,
                         WORD portNum);
						 
	CMODEthCommsProcessor(int responseDelay,
                         BOOL  MOSCADchecks,
                         BOOL modifyThenRespond,
                         BOOL disableWrites,
                         LONG PDUSize,
                         SOCKET * pServerSocket);
	virtual ~CMODEthCommsProcessor();

   BOOL ProcessData(SOCKET openSocket, 
                    const CHAR *pBuffer, 
                    const DWORD numBytes);
   BOOL ProcessMODData(SOCKET openSocket, 
                    const CHAR *pBuffer, 
                    const DWORD numBytes);					
   BOOL ProcessSimData(SOCKET openSocket, 
                    const CHAR *pBuffer, 
                    const DWORD numBytes);
					
   // overrridden functions
   void SockDataDebugger(const CHAR * buffer, LONG length, dataDebugAttrib att);
   void SockStateChanged(DWORD state);

   virtual void SockDataMessage(LPCTSTR msg);
   void ActivateStationLED(LONG stationID);
   BOOL StationIsEnabled(LONG stationID);


   void SetEmulationParameters(BOOL moscadChecks, 
                               BOOL modifyThenRespond, 
                               BOOL disableWrites);
   void SetPDUSize(DWORD size) {m_PDUSize = size;};
   BOOL LoadRegisters();
   BOOL SaveRegisters();


// data members   
   
   DWORD m_protocol;   // PROTOCOL_SELSIM//   PROTOCOL_SELMODETH // PROTOCOL_SELMODETH_RTU:
   UINT m_responseDelay;
   BOOL m_linger;
   //BOOL m_useEthernetStationID;
   //DWORD m_stationID;
   // noise/error sim
   BOOL  m_modifyThenRespond;
   BOOL  m_MOSCADchecks;      // MOSCAD
   BOOL  m_disableWrites;     // simple test

   LONG  m_noiseIndexValue;
   CEthernetNoise m_NoiseSimulator;
   //CRS232Noise m_NoiseSimulator;

   LPCTSTR ProtocolName() { return(m_protocolName);};

private:
 CString m_protocolName;
   DWORD m_PDUSize;
   CRITICAL_SECTION stateCS;

};

// ----------------------------------------------------------------------
// class SIMMessage declares a "SIM protcol frame".
// SIM protocol frame:
// <PLCMemoryArea>  0 = OUTPUT , INPUT ...  //word
// <ActionCode>  similar to function code   //word 1 read / 2 = write
// <WordCount> number of bytes to read or write //word
// <Adress>  //word
// <Data> data to write 
class CSIMMessage : public CObject
{
public:
   CRITICAL_SECTION stateCS;  //nlrkooi 
   virtual void SockDataMessage(LPCTSTR msg);

   CSIMMessage(const CHAR * pMessageRX, DWORD len);
   CSIMMessage(const CSIMMessage & oldSIMMessage); //copy constructor to assist in building the response message!



   WORD actionCode;  // 1 = read 2 = write
   WORD overalLen;      // telegram data length   //nlrkooi todo
   WORD address;
   WORD bitCount;          // # of data words?
   WORD byteCount;      // # of items, (if In/Outputs), then it is the # of bits.
   WORD wordCount;      // = byteCount/2
   BYTE coilByteCount;
   BYTE regByteCount;   // Added 2017-01-01 by DL for checking register byte counts
   WORD totalLen;
   // mask
   WORD m_andMask;
   WORD m_orMask;

   // Ethernet
   BYTE * preambleLenPtr; 
   BYTE * dataByteCountPtr;

   WORD  m_frameLength;  // the length as specified in the TCP headder field
   BOOL  m_packError;    // funny ASCII characters or bad data fields were found

   CHAR buffer[MAX_RX_MESSAGELENGTH + SIM_FRAME_LENGTH_MAX];     //TX/RX buffer
   BYTE* dataPtr;
   
   // methods
   CHAR * BuildMessagePreamble(BOOL error=FALSE,WORD errorCode=0);

   WORD GetAddressArea(WORD classCode); 

};


#endif // !defined(AFX_TICOMMSPROCESSOR_H__FDE99A84_435C_4094_826C_175DBDB6E61F__INCLUDED_)
