Version 8.21.2.7    03/06/18
  Fix handling of PDU size so that it is treated at 16-bit words instead of 8-bit bytes.
  Changed malformed C++ Variable names for header guards to be compliant with standard.


Version 8.21.2.6    12/03/18
  Experimental Support for Ethernet Function Code 23 Read/Write Holding Registers.
  Changed Station Number Scroll Bar to Up and Down Buttons for better ReactOS support.
  Fix for Writing Multiple Coils overwriting one more than desired.

Version 8.21.2.5    01/19/18
  Changed Toolbar location code so that left side tracks Protocol Selection Box right side in OnSize.
  Problem reported by Rob Gustavsson and fixed with his very helpful feedback and testing.

Version 8.21.2.4    01/08/17
  Fixed layout and auto-sizing to work better with different screen resolutions. Problem reported by Godra.
  Changed "Use Seperate Set of Registers for Each Station" to start at Station #1 not Station #0.

Version 8.21.2.3    01/02/17
  Added <CTRL>+<INS> into to HTML help file and About box. It sends Comms Debug text to the Windows Clipboard.
  Better readability on narrow widths by compressing main form items and putting Clone above the Log checkbox.
  New checkbox for "Log Decode" is now shown below the "Show Times" checkbox in the Comms Debugging screen.
  This is an added option for Logging function decode explanations to the Disk Log File for troubleshooting.
  With this option selected most of what is shown in the Comms Debugging window goes to the "DEBUG.TXT" file.
  Because of the greatly increased disk activity, some data may be missed in the Log file under heavy loads.
  Conversion of SaveDebugInfo function to CRS232Port::WriteToFile class with built-in check for time display.
  Made default PDU Size 256 for serial and 2048 for Ethernet and both can be overridden once selection made.
  Changed Modbus Exception Codes from 03 to 02 to reflect actual tests on Modbus PLC for out-of-range reads.
  Tests added for Modbus Exception Code 03 around PDU Size and Byte Count mismatch on write multiples.
  Next to last button tooltip changed from "PLC: Emulation Settings" to "Adv. PLC Emul. Settings"
  <F1> Help file "Protocols" changed to "Protocols - Advanced Emulation Settings" for better description.

Version 8.21.2.2    11/06/16
  Added option "Use Long Timeouts" changing RS-232 Timeouts from default of 100ms/500ms to 250ms/1000ms.
  Revised built-in <F1> Help to show the new "Use Long Timeouts" checkbox and give explanation of its use.
  Removed legacy code that checked for saved settings in HKLM. Now we only use HKCU for saving these.
  Removed some Registration Reminder Screen code to begin cleanup of code involving registration.
  Added RS-232 Serial debugging messages for Bad Parity, Overflows, Overruns, Framing Errors, etc.
  RS-232 Modem Status check with GetCommModemStatus now has error checking which quiets spam output.

Version 8.21.2.1    09/22/16
  Check for Socket Close or Reset from Client side and close our socket. Neither was previously handled.
  Change SO_LINGER which previously had 128 seconds of linger to 2 seconds.

Version 8.21.2.0    08/20/16
  Added Error Messages to Comms Window for Load/Save All Regs. Ex: No MODDATA.DAT file or Read-Only File.
  Added MessageBoxes for Load All Registers and Save All Registers to display any error messages.
  Changed Loading and Saving registers to use only 640K instead of 768K of disk space. Rest was not used.
  Increased ABDF1 File Number maximum from 254 to 255 because 255 is available in Allen-Bradley SLC's.
  Allow blanking script file name so built-in animation in CMOD_simDlg::DoPlantSimulation can run.
  Main Dialog font changed from 8->10 & rearranged second selection row for better viewing. Hex->H & Dec->D.
  Reworked IDD_TI_SIM_DIALOG & IDB_TOOLBARID location for better layout and viewing after font change to 10.
  Column auto resize for registers added when resizing main window which evenly divides the 10 columns.
  Testing & changes to "HTML Output GUI" for types of DECIMAL, HEX, WORD16, WORD32, LONG32, FLOAT and CHAR's.
  In AnimationSettings.cpp removed "UpdateData(TRUE);" because it was giving double errors on invalid data.
  Updated Help file to include "HTML Output GUI" Example and some explanations.

Version 8.21.1.9    08/02/16
  If enabled, add "Using Seperate Registers" to New Connection info line showing IP Address and Port.
  Also added MessageBox when program started to warn of "Using Seperate Registers for Each Station".
  Changed m_lampRX.SetupLamp/m_lampTX.SetupLamp to 600 because sometimes lamps were not showing activity.
  EditMemoryDlg edited to do some input verification for DECIMAL, LONG, HEX, WORD, CHAR & DWORD entries.
  Modified the Easter Egg display click to open the WEB address for the new forked site at SourceForge.
  Changes to Update Check because Internet Provider for www.douglyons.us increased header size requirements.
  Changed update URL to SourceForge Project Wiki page, then douglyons.us, then SourceForge Readme.txt.
  Added "R-off" when using RS-232 RTS=off to Title to show RTS state. Previously this displayed as blank.

Version 8.21.1.8    04/23/16
  Modified GetLastVisibleStation to allow station number of 255 (FF Hex) to show in Active Station display.
  Fix RS232 Writing Multiple Holding Regs showing as Writing Multiple Extended Registers in Comms display.
  Added Script Function clarifications about parenthesis requirements and examples in <F1> Help.
  Corrected Script Station Number range check to use ">=" instead of ">" when checking STATIONTICKBOXESMAX.
  Changed Script Function DisplayAbout to Sleep as delay test and reverted because this stopped comms.
  Modified Splash screen text for better readability and changed the WEB address to new forked site.
  Removed Version History from <F1> Help Display. This is available from the websites.

Version 8.21.1.7    12/19/15
  Added Support for Modbus Function Code 8 Subfunction 00 - Return Query Data (Loopback/Echo) for Ethernet.
  Added Support for Modbus Function Code 17 - Report Slave ID for RS232 and Ethernet as Modicon A984-131.
  Update Help file to include new "Modbus Function Code 17 - Report Slave ID" in Function Codes Supported.

Version 8.21.1.6    12/05/15

  Re-enable Noise Testing for Modbus RS232 (Beta Only) but not for Modbus Ethernet yet.
  Added Support for Modbus Function Code 8 Subfunction 00 - Return Query Data (Loopback/Echo) for RS232.
  Update Help file to include new Modbus Function Code 8 Subfunction 00 in listing.

Version 8.21.1.5    11/27/15

  Provided a Status Box for CSV Import that will warn about long times for importing large files.
  Update Status Box instead of using a MessageBox to display lines successfully manually imported.
  Removed automatic Window Minimize when Right Click Read-Only toggle when Read-Only is set.
  Corrected some lost characters in the Help file for "Load" and "Automation" lines.

Version 8.21.1.4    11/21/15    

  Revised CSV Import for more flexibility to set register type, number and value one-per-line.
  Improved CSV Automatic Importer to allow updates every minute. Previously this was every 15 minutes.
  On CSV Importer Dialog Box moved "Load File.." to show up prominently at the top left of the form.
  Fixed "Load Register Values.." & "Units are all..." to save settings on "OK" from Ethernet Settings box.
  Update to Help file to show Modbus Supported Commands and document CSV Importing Functionality.
  Suppressed MessageBox on Automatic CSV Import that showed Rows Imported which interrupted automation.
  Fix editing of 4-digit hex value (Total column) for 16 Coils/Inputs at once in correct bit order.

Version 8.21.1.3    11/03/2015

  Changed web reference for Update testing to www.douglyons.us.
  Added MessageBox on starting to warn user if another Modbus TCP/IP server may already be connected.

Version 8.21.1.2    07/28/2015

  Removed PortA.gif used for Background in Help index.html file for easier reading.
  Updated index.html used for help to include more information on Advanced Emulation with Separate Registers.
  Advanced Emulation now displays alternate Address in popup edit window instead of I/O when using Separate Registers.
  Change for Modbus Function Code 7 using Serial port to match Ethernet code.
  Fix to update screen register display when writes done remotely with using Advanced Emulation & Separate Registers.
  Added "Load Register Values at Startup" & "Units are all off at startup" to Ethernet Settings dialog.
  Correction to Allow Station #0 messages to illuminate in Active Station Display. There was no indication previously.
  Stop reply messages if the Station ID is zero (0) which is the Modbus broadcast address. Slaves should not respond.
  Added "Rx:" and "Tx:" labels to Receive and Transmit LED's at the top center of the form.
  Revised About | Update to use SourceForge-ModRSsim2 for downloading the README.txt file for latest version check.

Version 8.21.1.1    07/15/2015

  Changes to the Station Numbers for better visibility and scroll changes so first station is always multiple of 50.

Version 8.21.1.0    07/15/2015

  Changes to layout of the form so that it displays better in Windows 7.
  Default Modbus maximum register counts changed from 65535 to 65536.

Version 8.21.0.9    02/15/2015

  Fix crash when trying to open a script. This fix was thanks to James Slootweg from the SourceForge Support site.
  He created a support comment on 2013-10-03. MemoryEditorList fix to allow editing CHAR and HEX data.
  Allow "Clone" to reverse Byte Order for Floating Point display. Fix Function Code 22 Masked Write for RS-232
  Implement Function Code 22 Masked Write for Modbus TCP/IP. Made Changing "Clone" always redraw display.
  Changed CSV Read Max Columns from 17 to 1 and read as float. Only one value per line followed by comma.
  Otherwise we were forced to write a minimum of the number of Max Columns regardless of fewer values on the line.
  Changed CSV Read to write to every other memory instead of skipping 4 for each value for 32-bit Floating Support.
  Added code to update display after registers are read into the values after CSV Import.
  Included Example of CSV Read in CSVReader.cpp in comments. Added saving of Data View Format ("Fmt") to registry.
  This means that once selected when you open the program again, it will remember your selection of Format.
  Added Windows Version Check to Minimum Form Width. Win7 was squeezing out Toolbar when width minimized.
  Made all Modbus addresses 6-digits and widened Address column to handle this. This fixes bad address ranges.
  Removed spaces for I/O Display to get a better display of the values. Now shows "Holding Regs (400000)".
  Made Floating Point default to normal Modicon A984-131 word/byte order and "Clone" reverses the words.
  Reversed "TX" and "RX" in Comms Debug Messages because these seem to have been reversed in some simulations.
  Changed the Address Column for Modbus Inputs (000001 - 065535) so that they show with leading zeroes.
  Changed the VIEWFORMAT_FLOAT from %f to %g for better readability of very large and very small numbers.
  Fixed SockDataDebugger Call from DDKSocket.cpp so that it forwards to CRS232Port::GenDataDebugger like RS232.
  Added Debug Comms output text to show Inputs, Outputs, Input Regs, or Holding Regs for RS-232 and Ethernet.
  Widened I/O Listbox and changed it to display examples of 6-digit addressing to make it match other changes.
  Added Hotkey <Ctrl-F> (for File) to toggle sending characters to the DEBUG.TXT output file. Added Hours to output.
  Added "Log" check box on the main screen to turn on and off the DEBUG output and show its status.
  Added the ability to turn on and off the Time Stamps to the Debug Log File like the Debug Window.
  Fixed Crash when writing to coil bit 65536 and increased all Modbus limits to a maximum of 65536.
  Added Support for Function Code 7 to output first 8 coils (1-8) similar to most Modicon PLC's.
  Modified the MOD_WRITE_MULTIPLE_COILS function to update 8 times the number of bytes to the screen (BITS).
  Changed the registry read checking order to try HKCU first and then HKLM because writes to registry go there.
  Added Socket Timeout Setting to Ethernet Settings Dialog Box & Added Connected IP Address:Port to debugger out.
  Fix sending "New Connection from IP Address.." to DEBUG.TXT when LOG not checked.
  Added my name into the About and Easter Egg Display with reference to Revision 8.21.
  This revision is by Doug Lyons without permission from the original author. He did not answer my emails.
  This revision is created in Visual Studio 2010 Professional for the first time. Previously it was in VS 2008.