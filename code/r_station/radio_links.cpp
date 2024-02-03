/*
    MIT Licence
    Copyright (c) 2024 Petru Soroaga petrusoroaga@yahoo.com
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the organization nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.
        * Military use is not permited.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Julien Verneuil BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "radio_links.h"
#include "timers.h"
#include "shared_vars.h"
#include "../base/ctrl_interfaces.h"
#include "../base/ctrl_preferences.h"
#include "../base/hardware_radio.h"
#include "../base/hardware_radio_sik.h"
#include "../base/hw_procs.h"
#include "../base/radio_utils.h"
#include "../common/string_utils.h"
#include "../radio/radio_rx.h"
#include "../radio/radio_tx.h"

#include "packets_utils.h"
#include "links_utils.h"

int s_iFailedInitRadioInterface = -1;
pcap_t* s_pAuxiliaryLinks[MAX_RADIO_INTERFACES];
u32 s_uTimeLastCheckedAuxiliaryLinks = 0;
fd_set s_RadioAuxiliaryRxReadSet;

int radio_links_has_failed_interfaces()
{
   return s_iFailedInitRadioInterface;
}


void radio_links_reinit_radio_interfaces()
{
   char szComm[256];
   
   radio_links_close_rxtx_radio_interfaces();
   
   send_alarm_to_central(ALARM_ID_GENERIC_STATUS_UPDATE, ALARM_FLAG_GENERIC_STATUS_RECONFIGURING_RADIO_INTERFACE, 0);

   sprintf(szComm, "rm -rf %s", FILE_CURRENT_RADIO_HW_CONFIG);
   hw_execute_bash_command(szComm, NULL);

   hw_execute_bash_command("/etc/init.d/udev restart", NULL);
   hardware_sleep_ms(200);
   hw_execute_bash_command("sudo systemctl restart networking", NULL);
   hardware_sleep_ms(200);
   hw_execute_bash_command("sudo ifconfig -a", NULL);

   hardware_sleep_ms(50);

   hw_execute_bash_command("sudo systemctl stop networking", NULL);
   hardware_sleep_ms(200);
   hw_execute_bash_command("sudo ifconfig -a", NULL);

   hardware_sleep_ms(50);

   if ( NULL != g_pProcessStats )
   {
      g_TimeNow = get_current_timestamp_ms();
      g_pProcessStats->lastActiveTime = g_TimeNow;
      g_pProcessStats->lastIPCIncomingTime = g_TimeNow;
   }

   char szOutput[4096];
   szOutput[0] = 0;
   hw_execute_bash_command_raw("sudo ifconfig -a | grep wlan", szOutput);

   if ( NULL != g_pProcessStats )
   {
      g_TimeNow = get_current_timestamp_ms();
      g_pProcessStats->lastActiveTime = g_TimeNow;
      g_pProcessStats->lastIPCIncomingTime = g_TimeNow;
   }

   log_line("Reinitializing radio interfaces: found interfaces on ifconfig: [%s]", szOutput);
   sprintf(szComm, "rm -rf %s", FILE_CURRENT_RADIO_HW_CONFIG);
   hw_execute_bash_command(szComm, NULL);
   
   hw_execute_bash_command("ifconfig wlan0 down", NULL);
   hw_execute_bash_command("ifconfig wlan1 down", NULL);
   hw_execute_bash_command("ifconfig wlan2 down", NULL);
   hw_execute_bash_command("ifconfig wlan3 down", NULL);
   hardware_sleep_ms(200);

   hw_execute_bash_command("ifconfig wlan0 up", NULL);
   hw_execute_bash_command("ifconfig wlan1 up", NULL);
   hw_execute_bash_command("ifconfig wlan2 up", NULL);
   hw_execute_bash_command("ifconfig wlan3 up", NULL);
   
   sprintf(szComm, "rm -rf %s", FILE_CURRENT_RADIO_HW_CONFIG);
   hw_execute_bash_command(szComm, NULL);
   
   // Remove radio initialize file flag
   hw_execute_bash_command("rm -rf tmp/ruby/conf_radios", NULL);
   hw_execute_bash_command("./ruby_initradio", NULL);
   
   hardware_sleep_ms(100);
   hardware_reset_radio_enumerated_flag();
   hardware_enumerate_radio_interfaces();

   hardware_save_radio_info();
   hardware_sleep_ms(100);
 
   if ( NULL != g_pProcessStats )
   {
      g_TimeNow = get_current_timestamp_ms();
      g_pProcessStats->lastActiveTime = g_TimeNow;
      g_pProcessStats->lastIPCIncomingTime = g_TimeNow;
   }

   log_line("=================================================================");
   log_line("Detected hardware radio interfaces:");
   hardware_log_radio_info();

   radio_links_open_rxtx_radio_interfaces();

   send_alarm_to_central(ALARM_ID_GENERIC_STATUS_UPDATE, ALARM_FLAG_GENERIC_STATUS_RECONFIGURED_RADIO_INTERFACE, 0);
}

void radio_links_close_rxtx_radio_interfaces()
{
   log_line("Closing all radio interfaces (rx/tx).");

   radio_tx_mark_quit();
   hardware_sleep_ms(10);
   radio_tx_stop_tx_thread();

   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);
      if ( hardware_radio_is_sik_radio(pRadioHWInfo) )
         hardware_radio_sik_close(i);
   }

   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);
      if ( pRadioHWInfo->openedForWrite )
         radio_close_interface_for_write(i);

      if ( NULL != s_pAuxiliaryLinks[i] )
         radio_close_auxiliary_wfbohd_channel(s_pAuxiliaryLinks[i]);
      s_pAuxiliaryLinks[i] = NULL;
   }

   radio_close_interfaces_for_read();

   for( int i=0; i<MAX_RADIO_INTERFACES; i++ )
   {
      g_SM_RadioStats.radio_interfaces[i].openedForRead = 0;
      g_SM_RadioStats.radio_interfaces[i].openedForWrite = 0;
   }
   if ( NULL != g_pSM_RadioStats )
      memcpy((u8*)g_pSM_RadioStats, (u8*)&g_SM_RadioStats, sizeof(shared_mem_radio_stats));
   log_line("Closed all radio interfaces (rx/tx)."); 
}


void radio_links_open_rxtx_radio_interfaces_for_search( u32 uSearchFreq )
{
   log_line("");
   log_line("OPEN RADIO INTERFACES START =========================================================");
   log_line("Opening RX radio interfaces for search (%s), firmware: %s ...", str_format_frequency(uSearchFreq), str_format_firmware_type(g_uAcceptedFirmwareType));

   for( int i=0; i<MAX_RADIO_INTERFACES; i++ )
   {
      g_SM_RadioStats.radio_interfaces[i].openedForRead = 0;
      g_SM_RadioStats.radio_interfaces[i].openedForWrite = 0;
      s_pAuxiliaryLinks[i] = NULL;
   }

   s_iFailedInitRadioInterface = -1;

   int iCountOpenRead = 0;
   int iCountSikInterfacesOpened = 0;

   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);
      if ( NULL == pRadioHWInfo )
         continue;
      u32 flags = controllerGetCardFlags(pRadioHWInfo->szMAC);
      if ( (flags & RADIO_HW_CAPABILITY_FLAG_DISABLED) || controllerIsCardDisabled(pRadioHWInfo->szMAC) )
         continue;

      if ( 0 == hardware_radio_supports_frequency(pRadioHWInfo, uSearchFreq ) )
         continue;

      if ( flags & RADIO_HW_CAPABILITY_FLAG_CAN_RX )
      if ( flags & RADIO_HW_CAPABILITY_FLAG_CAN_USE_FOR_DATA )
      {
         if ( hardware_radio_is_sik_radio(pRadioHWInfo) )
         {
            if ( hardware_radio_sik_open_for_read_write(i) <= 0 )
               s_iFailedInitRadioInterface = i;
            else
            {
               g_SM_RadioStats.radio_interfaces[i].openedForRead = 1;
               iCountOpenRead++;
               iCountSikInterfacesOpened++;
            }
         }
         else
         {
            int iRes = 0;
            if ( g_uAcceptedFirmwareType == MODEL_FIRMWARE_TYPE_OPENIPC )
               iRes = radio_open_interface_for_read_wfbohd(i, (DEFAULT_WFBOHD_CHANNEL_ID<<8) + DEFAULT_WFBOHD_PORD_ID_VIDEO);
            else
               iRes = radio_open_interface_for_read(i, RADIO_PORT_ROUTER_DOWNLINK);
              
            if ( iRes > 0 )
            {
               log_line("Opened radio interface %d for read: USB port %s %s %s", i+1, pRadioHWInfo->szUSBPort, str_get_radio_type_description(pRadioHWInfo->typeAndDriver), pRadioHWInfo->szMAC);
               g_SM_RadioStats.radio_interfaces[i].openedForRead = 1;
               iCountOpenRead++;
            }
            else
               s_iFailedInitRadioInterface = i;
         }
      }
   }
   
   if ( 0 < iCountSikInterfacesOpened )
   {
      radio_tx_set_sik_packet_size(g_pCurrentModel->radioLinksParams.iSiKPacketSize);
      radio_tx_start_tx_thread();
   }

   // While searching, all cards are on same frequency, so a single radio link assigned to them.
   int iRadioLink = 0;
   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      if ( g_SM_RadioStats.radio_interfaces[i].openedForRead )
      {
         g_SM_RadioStats.radio_interfaces[i].assignedLocalRadioLinkId = iRadioLink;
         g_SM_RadioStats.radio_interfaces[i].assignedVehicleRadioLinkId = iRadioLink;
         //iRadioLink++;
      }
   }
   
   if ( NULL != g_pSM_RadioStats )
      memcpy((u8*)g_pSM_RadioStats, (u8*)&g_SM_RadioStats, sizeof(shared_mem_radio_stats));
   log_line("Opening RX radio interfaces for search complete. %d interfaces opened for RX:", iCountOpenRead);
   
   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      if ( g_SM_RadioStats.radio_interfaces[i].openedForRead )
      {
         radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);
         if ( NULL == pRadioHWInfo )
            log_line("   * Radio interface %d, name: %s opened for read.", i+1, "N/A");
         else
            log_line("   * Radio interface %d, name: %s opened for read.", i+1, pRadioHWInfo->szName);
      }
   }
   log_line("OPEN RADIO INTERFACES END =====================================================================");
   log_line("");
}

void radio_links_open_rxtx_radio_interfaces()
{
   log_line("");
   log_line("OPEN RADIO INTERFACES START =========================================================");

   if ( g_bSearching || (NULL == g_pCurrentModel) )
   {
      log_error_and_alarm("Invalid parameters for opening radio interfaces");
      return;
   }

   log_line("Opening RX/TX radio interfaces for current vehicle (firmware: %s)...", str_format_firmware_type(g_pCurrentModel->getVehicleFirmwareType()));

   int totalCountForRead = 0;
   int totalCountForWrite = 0;
   s_iFailedInitRadioInterface = -1;

   int countOpenedForReadForRadioLink[MAX_RADIO_INTERFACES];
   int countOpenedForWriteForRadioLink[MAX_RADIO_INTERFACES];
   int iCountSikInterfacesOpened = 0;

   for( int i=0; i<MAX_RADIO_INTERFACES; i++ )
   {
      countOpenedForReadForRadioLink[i] = 0;
      countOpenedForWriteForRadioLink[i] = 0;
      g_SM_RadioStats.radio_interfaces[i].openedForRead = 0;
      g_SM_RadioStats.radio_interfaces[i].openedForWrite = 0;
      s_pAuxiliaryLinks[i] = NULL;
   }

   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);

      if ( controllerIsCardDisabled(pRadioHWInfo->szMAC) )
         continue;

      int nVehicleRadioLinkId = g_SM_RadioStats.radio_interfaces[i].assignedVehicleRadioLinkId;
      if ( nVehicleRadioLinkId < 0 || nVehicleRadioLinkId >= g_pCurrentModel->radioLinksParams.links_count )
         continue;

      if ( g_pCurrentModel->radioLinksParams.link_capabilities_flags[nVehicleRadioLinkId] & RADIO_HW_CAPABILITY_FLAG_DISABLED )
         continue;

      // Ignore vehicle's relay radio links
      if ( g_pCurrentModel->radioLinksParams.link_capabilities_flags[nVehicleRadioLinkId] & RADIO_HW_CAPABILITY_FLAG_USED_FOR_RELAY )
         continue;

      if ( NULL != pRadioHWInfo )
      if ( ((pRadioHWInfo->typeAndDriver & 0xFF) == RADIO_TYPE_ATHEROS) ||
           ((pRadioHWInfo->typeAndDriver & 0xFF) == RADIO_TYPE_RALINK) )
      {
         int nRateTx = controllerGetCardDataRate(pRadioHWInfo->szMAC); // Returns 0 if radio link datarate must be used (no custom datarate set for this radio card);
         if ( (0 == nRateTx) && (NULL != g_pCurrentModel) )
         {
            nRateTx = compute_packet_uplink_datarate(nVehicleRadioLinkId, i, &(g_pCurrentModel->radioLinksParams));
            log_line("Current model uplink radio datarate for vehicle radio link %d (%s): %d, %u, uplink rate type: %d",
               nVehicleRadioLinkId+1, pRadioHWInfo->szName, nRateTx, getRealDataRateFromRadioDataRate(nRateTx),
               g_pCurrentModel->radioLinksParams.uUplinkDataDataRateType[nVehicleRadioLinkId]);
         }
         Preferences* pP = get_Preferences();
         radio_utils_set_datarate_atheros(NULL, i, nRateTx, pP->iDebugWiFiChangeDelay);
      }

      u32 cardFlags = controllerGetCardFlags(pRadioHWInfo->szMAC);

      if ( cardFlags & RADIO_HW_CAPABILITY_FLAG_CAN_RX )
      if ( (cardFlags & RADIO_HW_CAPABILITY_FLAG_CAN_USE_FOR_VIDEO) ||
           (cardFlags & RADIO_HW_CAPABILITY_FLAG_CAN_USE_FOR_DATA) )
      {
         if ( hardware_radio_is_sik_radio(pRadioHWInfo) )
         {
            if ( hardware_radio_sik_open_for_read_write(i) <= 0 )
               s_iFailedInitRadioInterface = i;
            else
            {
               g_SM_RadioStats.radio_interfaces[i].openedForRead = 1;
               countOpenedForReadForRadioLink[nVehicleRadioLinkId]++;
               totalCountForRead++;

               g_SM_RadioStats.radio_interfaces[i].openedForWrite = 1;
               countOpenedForWriteForRadioLink[nVehicleRadioLinkId]++;
               totalCountForWrite++;
               iCountSikInterfacesOpened++;
            }
         }
         else
         {
            int iRes = 0;
            if ( g_pCurrentModel->getVehicleFirmwareType() == MODEL_FIRMWARE_TYPE_OPENIPC )
               iRes = radio_open_interface_for_read_wfbohd(i, (DEFAULT_WFBOHD_CHANNEL_ID<<8) + DEFAULT_WFBOHD_PORD_ID_VIDEO);
            else
               iRes = radio_open_interface_for_read(i, RADIO_PORT_ROUTER_DOWNLINK);

            if ( iRes > 0 )
            {
               g_SM_RadioStats.radio_interfaces[i].openedForRead = 1;
               countOpenedForReadForRadioLink[nVehicleRadioLinkId]++;
               totalCountForRead++;
            }
            else
               s_iFailedInitRadioInterface = i;

            if ( g_pCurrentModel->getVehicleFirmwareType() == MODEL_FIRMWARE_TYPE_OPENIPC )
               s_pAuxiliaryLinks[i] = radio_open_auxiliary_wfbohd_channel(i, (DEFAULT_WFBOHD_CHANNEL_ID<<8) + DEFAULT_WFBOHD_PORD_ID_TELEMETRY);
         }
      }

      if ( g_pCurrentModel->getVehicleFirmwareType() == MODEL_FIRMWARE_TYPE_RUBY )
      if ( cardFlags & RADIO_HW_CAPABILITY_FLAG_CAN_TX )
      if ( (cardFlags & RADIO_HW_CAPABILITY_FLAG_CAN_USE_FOR_VIDEO) ||
           (cardFlags & RADIO_HW_CAPABILITY_FLAG_CAN_USE_FOR_DATA) )
      if ( ! hardware_radio_is_sik_radio(pRadioHWInfo) )
      {
         if ( radio_open_interface_for_write(i) > 0 )
         {
            g_SM_RadioStats.radio_interfaces[i].openedForWrite = 1;
            countOpenedForWriteForRadioLink[nVehicleRadioLinkId]++;
            totalCountForWrite++;
         }
         else
            s_iFailedInitRadioInterface = i;
      }
   }

   if ( NULL != g_pSM_RadioStats )
      memcpy((u8*)g_pSM_RadioStats, (u8*)&g_SM_RadioStats, sizeof(shared_mem_radio_stats));
   log_line("Opening RX/TX radio interfaces complete. %d interfaces opened for RX, %d interfaces opened for TX:", totalCountForRead, totalCountForWrite);

   if ( totalCountForRead == 0 )
   {
      log_error_and_alarm("Failed to find or open any RX interface for receiving data.");
      radio_links_close_rxtx_radio_interfaces();
      return;
   }

   if ( 0 == totalCountForWrite )
   if ( g_pCurrentModel->getVehicleFirmwareType() == MODEL_FIRMWARE_TYPE_RUBY )
   {
      log_error_and_alarm("Can't find any TX interfaces for sending data.");
      radio_links_close_rxtx_radio_interfaces();
      return;
   }

   for( int i=0; i<g_pCurrentModel->radioLinksParams.links_count; i++ )
   {
      if ( g_pCurrentModel->radioLinksParams.link_capabilities_flags[i] & RADIO_HW_CAPABILITY_FLAG_DISABLED )
         continue;
      
      // Ignore vehicle's relay radio links
      if ( g_pCurrentModel->radioLinksParams.link_capabilities_flags[i] & RADIO_HW_CAPABILITY_FLAG_USED_FOR_RELAY )
         continue;

      if ( 0 == countOpenedForReadForRadioLink[i] )
         log_error_and_alarm("Failed to find or open any RX interface for receiving data on vehicle's radio link %d.", i+1);
      if ( 0 == countOpenedForWriteForRadioLink[i] )
      if ( g_pCurrentModel->getVehicleFirmwareType() == MODEL_FIRMWARE_TYPE_RUBY )
         log_error_and_alarm("Failed to find or open any TX interface for sending data on vehicle's radio link %d.", i+1);

      //if ( 0 == countOpenedForReadForRadioLink[i] && 0 == countOpenedForWriteForRadioLink[i] )
      //   send_alarm_to_central(ALARM_ID_CONTROLLER_NO_INTERFACES_FOR_RADIO_LINK,i, 0);
   }

   log_line("Opened radio interfaces:");
   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);
      if ( NULL == pRadioHWInfo )
         continue;
      t_ControllerRadioInterfaceInfo* pCardInfo = controllerGetRadioCardInfo(pRadioHWInfo->szMAC);
   
      int nVehicleRadioLinkId = g_SM_RadioStats.radio_interfaces[i].assignedVehicleRadioLinkId;      
      int nLocalRadioLinkId = g_SM_RadioStats.radio_interfaces[i].assignedLocalRadioLinkId;      
      char szFlags[128];
      szFlags[0] = 0;
      u32 uFlags = controllerGetCardFlags(pRadioHWInfo->szMAC);
      str_get_radio_capabilities_description(uFlags, szFlags);

      char szType[128];
      strcpy(szType, pRadioHWInfo->szDriver);
      if ( NULL != pCardInfo )
         strcpy(szType, str_get_radio_card_model_string(pCardInfo->cardModel));

      if ( pRadioHWInfo->openedForRead && pRadioHWInfo->openedForWrite )
         log_line(" * Radio Interface %d, %s, %s, %s, local radio link %d, vehicle radio link %d, opened for read/write, flags: %s", i+1, pRadioHWInfo->szName, szType, str_format_frequency(pRadioHWInfo->uCurrentFrequencyKhz), nLocalRadioLinkId+1, nVehicleRadioLinkId+1, szFlags );
      else if ( pRadioHWInfo->openedForRead )
         log_line(" * Radio Interface %d, %s, %s, %s, local radio link %d, vehicle radio link %d, opened for read, flags: %s", i+1, pRadioHWInfo->szName, szType, str_format_frequency(pRadioHWInfo->uCurrentFrequencyKhz), nLocalRadioLinkId+1, nVehicleRadioLinkId+1, szFlags );
      else if ( pRadioHWInfo->openedForWrite )
         log_line(" * Radio Interface %d, %s, %s, %s, local radio link %d, vehicle radio link %d, opened for write, flags: %s", i+1, pRadioHWInfo->szName, szType, str_format_frequency(pRadioHWInfo->uCurrentFrequencyKhz), nLocalRadioLinkId+1, nVehicleRadioLinkId+1, szFlags );
      else
         log_line(" * Radio Interface %d, %s, %s, %s not used. Flags: %s", i+1, pRadioHWInfo->szName, szType, str_format_frequency(pRadioHWInfo->uCurrentFrequencyKhz), szFlags );
   }

   if ( 0 < iCountSikInterfacesOpened )
   {
      radio_tx_set_sik_packet_size(g_pCurrentModel->radioLinksParams.iSiKPacketSize);
      radio_tx_start_tx_thread();
   }

   if ( NULL != g_pSM_RadioStats )
      memcpy((u8*)g_pSM_RadioStats, (u8*)&g_SM_RadioStats, sizeof(shared_mem_radio_stats));
   log_line("Finished opening RX/TX radio interfaces.");
   log_line("OPEN RADIO INTERFACES END ===========================================================");
   log_line("");
}

pcap_t* radio_links_get_auxiliary_link(int iIndex)
{
   if ( (iIndex < 0) || (iIndex >= MAX_RADIO_INTERFACES) )
      return NULL;
   return s_pAuxiliaryLinks[iIndex];
}

// Returns positive is there are packets to read from auxiliary radio links
int radio_links_check_read_auxiliary_links()
{
   if ( g_TimeNow < s_uTimeLastCheckedAuxiliaryLinks + 50 )
      return 0;

   s_uTimeLastCheckedAuxiliaryLinks = g_TimeNow;
   
   struct timeval timeRXRadio;
   int iMaxFD = 0;
   int iCountFD = 0;
   
   FD_ZERO(&s_RadioAuxiliaryRxReadSet);
   timeRXRadio.tv_sec = 0;
   timeRXRadio.tv_usec = 100; // 0.1 miliseconds timeout
 
   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      if ( NULL == s_pAuxiliaryLinks[i] )
         continue;

      int iFD = pcap_get_selectable_fd(s_pAuxiliaryLinks[i]);
      if ( iFD <= 0 )
         continue;

      iCountFD++;
      FD_SET(iFD, &s_RadioAuxiliaryRxReadSet);
      if ( iFD > iMaxFD )
         iMaxFD = iFD;
   }

   int nResult = 0;
   if ( iCountFD > 0 )
      nResult = select(iMaxFD+1, &s_RadioAuxiliaryRxReadSet, NULL, NULL, &timeRXRadio);
   return nResult;
}

int radio_links_is_auxiliary_link_signaled(int iIndex)
{
   if ( (iIndex < 0) || (iIndex >= MAX_RADIO_INTERFACES) )
      return 0;
   if ( NULL == s_pAuxiliaryLinks[iIndex] )
      return 0;

   int iFD = pcap_get_selectable_fd(s_pAuxiliaryLinks[iIndex]);
   if ( iFD <= 0 )
      return 0;

   if ( 0 == FD_ISSET(iFD, &s_RadioAuxiliaryRxReadSet) )
      return 0;

   return 1;
}


bool radio_links_apply_settings(Model* pModel, int iRadioLink, type_radio_links_parameters* pRadioLinkParamsOld, type_radio_links_parameters* pRadioLinkParams)
{
   if ( (NULL == pModel) || (NULL == pRadioLinkParams) )
      return false;
   if ( (iRadioLink < 0) || (iRadioLink >= pModel->radioLinksParams.links_count) )
      return false;

   // Update HT20/HT40 if needed

   if ( (pRadioLinkParamsOld->link_radio_flags[iRadioLink] & RADIO_FLAG_HT40_CONTROLLER) != 
        (pRadioLinkParams->link_radio_flags[iRadioLink] & RADIO_FLAG_HT40_CONTROLLER) )
   {
      for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
      {
         if ( g_SM_RadioStats.radio_interfaces[i].assignedVehicleRadioLinkId != iRadioLink )
            continue;
         if ( iRadioLink != pModel->radioInterfacesParams.interface_link_id[i] )
            continue;
         radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);
         if ( NULL == pRadioHWInfo )
            continue;

         radio_utils_set_interface_frequency(pModel, i, iRadioLink, pModel->radioLinksParams.link_frequency_khz[iRadioLink], g_pProcessStats, 0);
      }
   }

   // Apply data rates

   // If uplink data rate for an Atheros card has changed, update it.
      
   for( int i=0; i<hardware_get_radio_interfaces_count(); i++ )
   {
      if ( g_SM_RadioStats.radio_interfaces[i].assignedVehicleRadioLinkId != iRadioLink )
         continue;
      radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(i);
      if ( NULL == pRadioHWInfo )
         continue;
      if ( ((pRadioHWInfo->typeAndDriver & 0xFF) != RADIO_TYPE_ATHEROS) &&
        ((pRadioHWInfo->typeAndDriver & 0xFF) != RADIO_TYPE_RALINK) )
         continue;

      //int nRateTx = pRadioLinkParams->uplink_datarate_data_bps[iRadioLink];
      int nRateTx = compute_packet_uplink_datarate(iRadioLink, i, pRadioLinkParams);
      update_atheros_card_datarate(pModel, i, nRateTx, g_pProcessStats);
      g_TimeNow = get_current_timestamp_ms();
   }

   // Radio flags are applied on the fly, when sending each radio packet
   
   return true;
}