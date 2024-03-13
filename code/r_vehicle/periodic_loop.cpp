#include "periodic_loop.h"

#include "../base/base.h"
#include "../base/config.h"
#include "../base/models.h"
#include "../base/hw_procs.h"
#ifdef HW_PLATFORM_RASPBERRY
#include "../base/hardware_i2c.h"
#endif
#include "../base/ruby_ipc.h"
#include "../common/radio_stats.h"

#include "../radio/radiopackets2.h"
#include "../radio/radiopacketsqueue.h"

#include "shared_vars.h"
#include "timers.h"
#include "ruby_rt_vehicle.h"
#include "test_link_params.h"
#include "packets_utils.h"
#include "processor_relay.h"
#include "utils_vehicle.h"
#include "launchers_vehicle.h"
#include "video_link_check_bitrate.h"

u32 s_LoopCounter = 0;
u32 s_debugFramesCount = 0;
u32 s_uTimeLastCheckForRelayedVehicleRubyTelemetryAlarm = 0;
u32 s_MinVideoBlocksGapMilisec = 1;
long s_lLastLiveLogFileOffset = -1;

extern u16 s_countTXVideoPacketsOutPerSec[2];
extern u16 s_countTXDataPacketsOutPerSec[2];
extern u16 s_countTXCompactedPacketsOutPerSec[2];

static void * _reinit_sik_thread_func(void *ignored_argument)
{
   log_line("[Router-SiKThread] Reinitializing SiK radio interfaces...");
   // radio serial ports are already closed at this point

   if ( g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex >= 0 )
   {
      log_line("[Router-SiKThread] Must reconfigure and reinitialize SiK radio interface %d...", g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex+1 );
      if ( ! hardware_radio_index_is_sik_radio(g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex) )
         log_softerror_and_alarm("[Router-SiKThread] Radio interface %d is not a SiK radio interface.", g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex+1 );
      else
      {
         radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex);
         if ( NULL == pRadioHWInfo )
            log_softerror_and_alarm("[Router-SiKThread] Failed to get radio hw info for radio interface %d.", g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex+1);
         else
         {
            u32 uFreqKhz = pRadioHWInfo->uHardwareParamsList[8];
            u32 uDataRate = DEFAULT_RADIO_DATARATE_SIK_AIR;
            u32 uTxPower = DEFAULT_RADIO_SIK_TX_POWER;
            u32 uLBT = 0;
            u32 uECC = 0;
            u32 uMCSTR = 0;

            if ( NULL != g_pCurrentModel )
            {
               int iRadioLink = g_pCurrentModel->radioInterfacesParams.interface_link_id[g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex];
               if ( (iRadioLink >= 0) && (iRadioLink < g_pCurrentModel->radioLinksParams.links_count) )
               {
                  uFreqKhz = g_pCurrentModel->radioLinksParams.link_frequency_khz[iRadioLink];
                  uDataRate = g_pCurrentModel->radioLinksParams.link_datarate_data_bps[iRadioLink];
                  uTxPower = g_pCurrentModel->radioInterfacesParams.txPowerSiK;
                  uECC = (g_pCurrentModel->radioLinksParams.link_radio_flags[iRadioLink] & RADIO_FLAGS_SIK_ECC)? 1:0;
                  uLBT = (g_pCurrentModel->radioLinksParams.link_radio_flags[iRadioLink] & RADIO_FLAGS_SIK_LBT)? 1:0;
                  uMCSTR = (g_pCurrentModel->radioLinksParams.link_radio_flags[iRadioLink] & RADIO_FLAGS_SIK_MCSTR)? 1:0;
               
                  bool bDataRateOk = false;
                  for( int i=0; i<getSiKAirDataRatesCount(); i++ )
                  {
                     if ( (int)uDataRate == getSiKAirDataRates()[i] )
                     {
                        bDataRateOk = true;
                        break;
                     }
                  }

                  if ( ! bDataRateOk )
                  {
                     log_softerror_and_alarm("[Router-SiKThread] Invalid radio datarate for SiK radio: %d bps. Revert to %d bps.", uDataRate, DEFAULT_RADIO_DATARATE_SIK_AIR);
                     uDataRate = DEFAULT_RADIO_DATARATE_SIK_AIR;
                  }
               }
            }
            int iRes = hardware_radio_sik_set_params(pRadioHWInfo, 
                   uFreqKhz,
                   DEFAULT_RADIO_SIK_FREQ_SPREAD, DEFAULT_RADIO_SIK_CHANNELS,
                   DEFAULT_RADIO_SIK_NETID,
                   uDataRate, uTxPower, 
                   uECC, uLBT, uMCSTR,
                   NULL);
            if ( iRes != 1 )
            {
               log_softerror_and_alarm("[Router-SiKThread] Failed to reconfigure SiK radio interface %d", g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex+1);
               if ( g_SiKRadiosState.iThreadRetryCounter < 3 )
               {
                  log_line("[Router-SiKThread] Will retry to reconfigure radio. Retry counter: %d", g_SiKRadiosState.iThreadRetryCounter);
               }
               else
               {
                  send_alarm_to_controller(ALARM_ID_GENERIC_STATUS_UPDATE, ALARM_FLAG_GENERIC_STATUS_RECONFIGURED_RADIO_INTERFACE_FAILED, 0, 10);
                  reopen_marked_sik_interfaces();
                  g_SiKRadiosState.bMustReinitSiKInterfaces = false;
                  g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex = -1;
                  g_SiKRadiosState.uSiKInterfaceIndexThatBrokeDown = MAX_U32;
               }
               log_line("[Router-SiKThread] Finished.");
               g_SiKRadiosState.bConfiguringSiKThreadWorking = false;
               return NULL;
            }
            else
            {
               log_line("[Router-SiKThread] Updated successfully SiK radio interface %d to txpower %d, airrate: %d bps, ECC/LBT/MCSTR: %d/%d/%d",
                   g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex+1,
                   uTxPower, uDataRate, uECC, uLBT, uMCSTR);
               radio_stats_set_card_current_frequency(&g_SM_RadioStats, g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex, uFreqKhz);
            }
         }
      }
   }
   else if ( 1 != hardware_radio_sik_reinitialize_serial_ports() )
   {
      log_line("[Router-SiKThread] Reinitializing of SiK radio interfaces failed (not the same ones are present yet).");
      // Will restart the thread to try again
      log_line("[Router-SiKThread] Finished.");
      g_SiKRadiosState.bConfiguringSiKThreadWorking = false;
      return NULL;
   }
   
   log_line("[Router-SiKThread] Reinitialized SiK radio interfaces successfully.");
   
   reopen_marked_sik_interfaces();
   if ( g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex >= 0 )
       send_alarm_to_controller(ALARM_ID_GENERIC_STATUS_UPDATE, ALARM_FLAG_GENERIC_STATUS_RECONFIGURED_RADIO_INTERFACE, 0, 10);
   else
       send_alarm_to_controller(ALARM_ID_RADIO_INTERFACE_REINITIALIZED, g_SiKRadiosState.uSiKInterfaceIndexThatBrokeDown, 0, 10); 

   g_SiKRadiosState.bMustReinitSiKInterfaces = false;
   g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex = -1;
   g_SiKRadiosState.uSiKInterfaceIndexThatBrokeDown = MAX_U32;
   log_line("[Router-SiKThread] Finished.");
   g_SiKRadiosState.bConfiguringSiKThreadWorking = false;
   return NULL;
}

int _check_reinit_sik_interfaces()
{
   if ( g_SiKRadiosState.bConfiguringToolInProgress && (g_SiKRadiosState.uTimeStartConfiguring != 0) )
   if ( g_TimeNow >= g_SiKRadiosState.uTimeStartConfiguring+500 )
   {
      if ( hw_process_exists("ruby_sik_config") )
      {
         g_SiKRadiosState.uTimeStartConfiguring = g_TimeNow - 400;
      }
      else
      {
         int iResult = -1;
         char szFile[128];
         strcpy(szFile, FOLDER_RUBY_TEMP);
         strcat(szFile, FILE_TEMP_SIK_CONFIG_FINISHED);
         FILE* fd = fopen(szFile, "rb");
         if ( NULL != fd )
         {
            if ( 1 != fscanf(fd, "%d", &iResult) )
               iResult = -2;
            fclose(fd);
         }
         log_line("SiK radio configuration completed. Result: %d.", iResult);
         sprintf(szFile, "rm -rf %s%s", FOLDER_RUBY_TEMP, FILE_TEMP_SIK_CONFIG_FINISHED);
         hw_execute_bash_command(szFile, NULL);
         g_SiKRadiosState.bConfiguringToolInProgress = false;
         reopen_marked_sik_interfaces();
         send_alarm_to_controller(ALARM_ID_GENERIC_STATUS_UPDATE, ALARM_FLAG_GENERIC_STATUS_RECONFIGURED_RADIO_INTERFACE, 0, 10);
         return 0;
      }
   }
   
   if ( (! g_SiKRadiosState.bMustReinitSiKInterfaces) && (g_SiKRadiosState.iMustReconfigureSiKInterfaceIndex == -1) )
      return 0;

   if ( g_SiKRadiosState.bConfiguringToolInProgress )
      return 0;

   if ( g_SiKRadiosState.bConfiguringSiKThreadWorking )
      return 0;
   
   if ( g_TimeNow < g_SiKRadiosState.uTimeLastSiKReinitCheck + g_SiKRadiosState.uTimeIntervalSiKReinitCheck )
      return 0;

   g_SiKRadiosState.uTimeLastSiKReinitCheck = g_TimeNow;
   g_SiKRadiosState.uTimeIntervalSiKReinitCheck += 200;
   g_SiKRadiosState.bConfiguringSiKThreadWorking = true;
   static pthread_t pThreadSiKReinit;
   if ( 0 != pthread_create(&pThreadSiKReinit, NULL, &_reinit_sik_thread_func, NULL) )
   {
      log_softerror_and_alarm("[Router] Failed to create worker thread to reinit SiK radio interfaces.");
      g_SiKRadiosState.bConfiguringSiKThreadWorking = false;
      return 0;
   }
   log_line("[Router] Created thread to reinit SiK radio interfaces.");
   if ( 0 == g_SiKRadiosState.iThreadRetryCounter )
      send_alarm_to_controller(ALARM_ID_GENERIC_STATUS_UPDATE, ALARM_FLAG_GENERIC_STATUS_RECONFIGURING_RADIO_INTERFACE, 0, 10);
   g_SiKRadiosState.iThreadRetryCounter++;
   return 1;
}

void _check_write_filesystem()
{
   static bool s_bRouterCheckedForWriteFileSystem = false;
   static bool s_bRouterWriteFileSystemOk = false;

   if ( ! s_bRouterCheckedForWriteFileSystem )
   {
      log_line("Checking the file system for write access...");
      s_bRouterCheckedForWriteFileSystem = true;
      s_bRouterWriteFileSystemOk = false;

      hw_execute_bash_command("rm -rf tmp/testwrite.txt", NULL);
      FILE* fdTemp = fopen("tmp/testwrite.txt", "wb");
      if ( NULL == fdTemp )
      {
         g_pCurrentModel->alarms |= ALARM_ID_VEHICLE_STORAGE_WRITE_ERRROR;
         s_bRouterWriteFileSystemOk = false;
      }
      else
      {
         fprintf(fdTemp, "test1234\n");
         fclose(fdTemp);
         fdTemp = fopen("tmp/testwrite.txt", "rb");
         if ( NULL == fdTemp )
         {
            g_pCurrentModel->alarms |= ALARM_ID_VEHICLE_STORAGE_WRITE_ERRROR;
            s_bRouterWriteFileSystemOk = false;
         }
         else
         {
            char szTmp[256];
            if ( 1 != fscanf(fdTemp, "%s", szTmp) )
            {
               g_pCurrentModel->alarms |= ALARM_ID_VEHICLE_STORAGE_WRITE_ERRROR;
               s_bRouterWriteFileSystemOk = false;
            }
            else if ( 0 != strcmp(szTmp, "test1234") )
            {
               g_pCurrentModel->alarms |= ALARM_ID_VEHICLE_STORAGE_WRITE_ERRROR;
               s_bRouterWriteFileSystemOk = false;
            }
            else
               s_bRouterWriteFileSystemOk = true;
            fclose(fdTemp);
            hw_execute_bash_command("rm -rf tmp/testwrite.txt", NULL);
         }
      }
      if ( ! s_bRouterWriteFileSystemOk )
         log_line("Checking the file system for write access: Failed.");
      else
         log_line("Checking the file system for write access: Succeeded.");
   }
   if ( s_bRouterCheckedForWriteFileSystem )
   {
      if ( ! s_bRouterWriteFileSystemOk )
        send_alarm_to_controller(ALARM_ID_VEHICLE_STORAGE_WRITE_ERRROR, 0, 0, 5);
   }      
}


void _send_radio_stats_to_controller()
{
   // Update lastDataRate for SiK radios and MCS links
   for( int i=0; i<g_pCurrentModel->radioInterfacesParams.interfaces_count; i++ )
   {
      int iLinkId = g_pCurrentModel->radioInterfacesParams.interface_link_id[i];
      if ( (iLinkId < 0) || (iLinkId >= g_pCurrentModel->radioLinksParams.links_count) )
         continue;
      if ( g_pCurrentModel->radioLinkIsSiKRadio(iLinkId) )
      {
         g_SM_RadioStats.radio_interfaces[i].lastRecvDataRate = g_pCurrentModel->radioLinksParams.link_datarate_data_bps[iLinkId];
         g_SM_RadioStats.radio_interfaces[i].lastRecvDataRateData = g_pCurrentModel->radioLinksParams.link_datarate_data_bps[iLinkId];
         g_SM_RadioStats.radio_interfaces[i].lastRecvDataRateVideo = 0;
      }
      else if ( g_pCurrentModel->radioLinksParams.link_datarate_video_bps[iLinkId] < 0 )
      {
         g_SM_RadioStats.radio_interfaces[i].lastRecvDataRate = g_pCurrentModel->radioLinksParams.link_datarate_video_bps[iLinkId];
         g_SM_RadioStats.radio_interfaces[i].lastRecvDataRateData = g_pCurrentModel->radioLinksParams.link_datarate_data_bps[iLinkId];
         g_SM_RadioStats.radio_interfaces[i].lastRecvDataRateVideo = g_pCurrentModel->radioLinksParams.link_datarate_video_bps[iLinkId];
      }
   }

   // Update time now
   for( int i=0; i<g_pCurrentModel->radioInterfacesParams.interfaces_count; i++ )
   {
      g_SM_RadioStats.radio_interfaces[i].timeNow = g_TimeNow;
   }

   t_packet_header PH;
   radio_packet_init(&PH, PACKET_COMPONENT_TELEMETRY, PACKET_TYPE_RUBY_TELEMETRY_VEHICLE_RX_CARDS_STATS, STREAM_ID_DATA);
   PH.vehicle_id_src = g_pCurrentModel->uVehicleId;
   PH.vehicle_id_dest = g_uControllerId;
   
   u8 packet[MAX_PACKET_TOTAL_SIZE];
   u8* pData = packet + sizeof(t_packet_header) + sizeof(u8);
   
   // Send all in single packet
   
   PH.packet_flags_extended |= PACKET_FLAGS_EXTENDED_BIT_SEND_ON_HIGH_CAPACITY_LINK_ONLY;
   PH.packet_flags_extended &= (~PACKET_FLAGS_EXTENDED_BIT_SEND_ON_LOW_CAPACITY_LINK_ONLY);
   PH.total_length = sizeof(t_packet_header) + sizeof(u8) + g_pCurrentModel->radioInterfacesParams.interfaces_count * sizeof(shared_mem_radio_stats_radio_interface);

   if ( PH.total_length <= MAX_PACKET_PAYLOAD )
   {
      u8 count = g_pCurrentModel->radioInterfacesParams.interfaces_count;
      memcpy(packet, (u8*)&PH, sizeof(t_packet_header));
      memcpy(packet + sizeof(t_packet_header), (u8*)&count, sizeof(u8));
      for( int i=0; i<g_pCurrentModel->radioInterfacesParams.interfaces_count; i++ )
      {
         memcpy(pData, &(g_SM_RadioStats.radio_interfaces[i]), sizeof(shared_mem_radio_stats_radio_interface));
         pData += sizeof(shared_mem_radio_stats_radio_interface);
      }
      packets_queue_add_packet(&g_QueueRadioPacketsOut, packet);
   }

   // Send rx stats, for each radio interface in individual single packets (to fit in small SiK packets)
   // Send shared_mem_radio_stats_radio_interface_compact
   if ( hardware_radio_has_low_capacity_links() )
   {
      PH.packet_flags_extended |= PACKET_FLAGS_EXTENDED_BIT_SEND_ON_LOW_CAPACITY_LINK_ONLY;
      PH.packet_flags_extended &= (~PACKET_FLAGS_EXTENDED_BIT_SEND_ON_HIGH_CAPACITY_LINK_ONLY);
      PH.total_length = sizeof(t_packet_header) + sizeof(u8) + sizeof(shared_mem_radio_stats_radio_interface_compact);
      
      static u8 uCardIndexRxStatsToSend = 0;
      uCardIndexRxStatsToSend++;
      if ( uCardIndexRxStatsToSend >= g_pCurrentModel->radioInterfacesParams.interfaces_count )
         uCardIndexRxStatsToSend = 0;

      shared_mem_radio_stats_radio_interface_compact statsCompact;
      
      statsCompact.lastDbm = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].lastDbm;
      statsCompact.lastDbmVideo = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].lastDbmVideo;
      statsCompact.lastDbmData = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].lastDbmData;
      statsCompact.lastRecvDataRate = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].lastRecvDataRate;
      statsCompact.lastRecvDataRateVideo = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].lastRecvDataRateVideo;
      statsCompact.lastRecvDataRateData = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].lastRecvDataRateData;

      statsCompact.totalRxBytes = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].totalRxBytes;
      statsCompact.totalTxBytes = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].totalTxBytes;
      statsCompact.rxBytesPerSec = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].rxBytesPerSec;
      statsCompact.txBytesPerSec = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].txBytesPerSec;
      statsCompact.totalRxPackets = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].totalRxPackets;
      statsCompact.totalRxPacketsBad = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].totalRxPacketsBad;
      statsCompact.totalRxPacketsLost = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].totalRxPacketsLost;
      statsCompact.totalTxPackets = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].totalTxPackets;
      statsCompact.rxPacketsPerSec = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].rxPacketsPerSec;
      statsCompact.txPacketsPerSec = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].txPacketsPerSec;
      statsCompact.timeLastRxPacket = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].timeLastRxPacket;
      statsCompact.timeLastTxPacket = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].timeLastTxPacket;
      statsCompact.timeNow = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].timeNow;
      statsCompact.rxQuality = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].rxQuality;
      statsCompact.rxRelativeQuality = g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].rxRelativeQuality;

      memcpy(statsCompact.hist_rxPacketsCount, g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].hist_rxPacketsCount, MAX_HISTORY_RADIO_STATS_RECV_SLICES * sizeof(u8));
      memcpy(statsCompact.hist_rxPacketsLostCount, g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].hist_rxPacketsLostCount, MAX_HISTORY_RADIO_STATS_RECV_SLICES * sizeof(u8));
      memcpy(statsCompact.hist_rxGapMiliseconds, g_SM_RadioStats.radio_interfaces[uCardIndexRxStatsToSend].hist_rxGapMiliseconds, MAX_HISTORY_RADIO_STATS_RECV_SLICES * sizeof(u8));

      memcpy(packet, (u8*)&PH, sizeof(t_packet_header));
      memcpy(packet + sizeof(t_packet_header), (u8*)&uCardIndexRxStatsToSend, sizeof(u8));
      memcpy(packet + sizeof(t_packet_header) + sizeof(u8), &(statsCompact), sizeof(shared_mem_radio_stats_radio_interface_compact));
      
      packets_queue_add_packet(&g_QueueRadioPacketsOut, packet);
   }

}

void _periodic_loop_check_ping()
{
   if ( g_pCurrentModel->relay_params.isRelayEnabledOnRadioLinkId >= 0 )
   if ( g_pCurrentModel->relay_params.uRelayedVehicleId != 0 )
   if ( g_TimeNow > s_uTimeLastCheckForRelayedVehicleRubyTelemetryAlarm + 200 )
   {
      s_uTimeLastCheckForRelayedVehicleRubyTelemetryAlarm = g_TimeNow;
      
      u32 uLastTimeRecvRubyTelemetry = relay_get_time_last_received_ruby_telemetry_from_relayed_vehicle();
      static u32 sl_uTimeLastSendRubyRelayedTelemetryLostAlarm = 0;
      static u32 sl_uTimeLastSendRubyRelayedTelemetryRecoveredAlarm = 0;
      if ( g_TimeNow > uLastTimeRecvRubyTelemetry + TIMEOUT_TELEMETRY_LOST )
      {
         if ( g_TimeNow > sl_uTimeLastSendRubyRelayedTelemetryLostAlarm + 10000 )
         {
            sl_uTimeLastSendRubyRelayedTelemetryLostAlarm = g_TimeNow;
            sl_uTimeLastSendRubyRelayedTelemetryRecoveredAlarm = 0;
            send_alarm_to_controller(ALARM_ID_GENERIC, ALARM_ID_GENERIC_TYPE_RELAYED_TELEMETRY_LOST, 0, 2);
         }
      }
      else
      {
         if ( g_TimeNow > sl_uTimeLastSendRubyRelayedTelemetryRecoveredAlarm + 10000 )
         {
            sl_uTimeLastSendRubyRelayedTelemetryRecoveredAlarm = g_TimeNow;
            sl_uTimeLastSendRubyRelayedTelemetryLostAlarm = 0;
            send_alarm_to_controller(ALARM_ID_GENERIC, ALARM_ID_GENERIC_TYPE_RELAYED_TELEMETRY_RECOVERED, 0, 2);
         }
      }
   }

   // Vehicle does not need to ping the relayed vehicle. Controller will.
   return;

   /*
   static u32 s_uTimeLastCheckSendPing = 0;
   static u8 s_uLastPingSentId = 0;

   if ( g_TimeNow < s_uTimeLastCheckSendPing+1000 )
      return;

   s_uTimeLastCheckSendPing = g_TimeNow;

   bool bMustSendPing = false;

   if ( g_pCurrentModel->relay_params.uCurrentRelayMode & RELAY_MODE_IS_RELAY_NODE )
   if ( g_pCurrentModel->relay_params.isRelayEnabledOnRadioLinkId >= 0 )
   if ( g_pCurrentModel->relay_params.uRelayedVehicleId != 0 )
      bMustSendPing = true;

   if ( ! bMustSendPing )
      return;

   s_uLastPingSentId++;
   u8 uRadioLinkId = g_pCurrentModel->relay_params.isRelayEnabledOnRadioLinkId;
   u8 uDestinationRelayFlags = g_pCurrentModel->relay_params.uRelayCapabilitiesFlags;
   u8 uDestinationRelayMode = g_pCurrentModel->relay_params.uCurrentRelayMode;

   t_packet_header PH;
   radio_packet_init(&PH, PACKET_COMPONENT_RUBY, PACKET_TYPE_RUBY_PING_CLOCK, STREAM_ID_DATA);
   PH.vehicle_id_src = g_pCurrentModel->uVehicleId;
   PH.vehicle_id_dest = g_pCurrentModel->relay_params.uRelayedVehicleId;
   PH.total_length = sizeof(t_packet_header) + 4*sizeof(u8);
   
   u8 packet[MAX_PACKET_TOTAL_SIZE];
   // u8 ping id, u8 radio link id, u8 relay flags for destination vehicle
   memcpy(packet, (u8*)&PH, sizeof(t_packet_header));
   memcpy(packet+sizeof(t_packet_header), &s_uLastPingSentId, sizeof(u8));
   memcpy(packet+sizeof(t_packet_header)+sizeof(u8), &uRadioLinkId, sizeof(u8));
   memcpy(packet+sizeof(t_packet_header)+2*sizeof(u8), &uDestinationRelayFlags, sizeof(u8));
   memcpy(packet+sizeof(t_packet_header)+3*sizeof(u8), &uDestinationRelayMode, sizeof(u8));
   
   relay_send_single_packet_to_relayed_vehicle(packet, PH.total_length);
   */
}


void _update_videobitrate_history()
{
   if ( g_bVideoPaused )
      return;
   if ( ! (g_pCurrentModel->osd_params.osd_flags3[g_pCurrentModel->osd_params.layout] & OSD_FLAG3_SHOW_VIDEO_BITRATE_HISTORY) )
      return;
   if ( g_TimeNow < g_SM_DevVideoBitrateHistory.uLastGraphSliceTime + g_SM_DevVideoBitrateHistory.uGraphSliceInterval )
      return;

   g_SM_DevVideoBitrateHistory.uLastGraphSliceTime = g_TimeNow;
   
   g_SM_DevVideoBitrateHistory.uQuantizationOverflowValue = video_link_get_oveflow_quantization_value();
   g_SM_DevVideoBitrateHistory.uCurrentTargetVideoBitrate = g_SM_VideoLinkStats.overwrites.currentSetVideoBitrate;

   for( int i=MAX_INTERVALS_VIDEO_BITRATE_HISTORY-1; i>0; i-- )
   {
      g_SM_DevVideoBitrateHistory.uHistMaxVideoDataRateMbps[i] = g_SM_DevVideoBitrateHistory.uHistMaxVideoDataRateMbps[i-1]; 
      g_SM_DevVideoBitrateHistory.uHistVideoQuantization[i] = g_SM_DevVideoBitrateHistory.uHistVideoQuantization[i-1]; 
      g_SM_DevVideoBitrateHistory.uHistVideoBitrateKb[i] = g_SM_DevVideoBitrateHistory.uHistVideoBitrateKb[i-1]; 
      g_SM_DevVideoBitrateHistory.uHistVideoBitrateAvgKb[i] = g_SM_DevVideoBitrateHistory.uHistVideoBitrateAvgKb[i-1]; 
      g_SM_DevVideoBitrateHistory.uHistTotalVideoBitrateAvgKb[i] = g_SM_DevVideoBitrateHistory.uHistTotalVideoBitrateAvgKb[i-1]; 
      g_SM_DevVideoBitrateHistory.uHistoryVideoSwitches[i] = g_SM_DevVideoBitrateHistory.uHistoryVideoSwitches[i-1]; 
   }
   g_SM_DevVideoBitrateHistory.uHistVideoQuantization[0] = g_SM_VideoLinkStats.overwrites.currentH264QUantization;
   g_SM_DevVideoBitrateHistory.uHistMaxVideoDataRateMbps[0] = get_last_tx_video_datarate_mbps();
   g_SM_DevVideoBitrateHistory.uHistVideoBitrateKb[0] = g_pProcessorTxVideo->getCurrentVideoBitrate()/1000;
   g_SM_DevVideoBitrateHistory.uHistVideoBitrateAvgKb[0] = g_pProcessorTxVideo->getCurrentVideoBitrateAverage()/1000;
   g_SM_DevVideoBitrateHistory.uHistTotalVideoBitrateAvgKb[0] = g_pProcessorTxVideo->getCurrentTotalVideoBitrateAverage()/1000;
   g_SM_DevVideoBitrateHistory.uHistoryVideoSwitches[0] = g_SM_VideoLinkStats.overwrites.currentProfileShiftLevel | (g_SM_VideoLinkStats.overwrites.currentVideoLinkProfile<<4);
   if ( (0 == get_video_capture_start_program_time()) || (g_TimeNow < get_video_capture_start_program_time() + 3000) )
      g_SM_DevVideoBitrateHistory.uHistVideoQuantization[0] = 0xFF;

   t_packet_header PH;
   radio_packet_init(&PH, PACKET_COMPONENT_TELEMETRY, PACKET_TYPE_RUBY_TELEMETRY_DEV_VIDEO_BITRATE_HISTORY, STREAM_ID_DATA);
   PH.vehicle_id_src = g_pCurrentModel->uVehicleId;
   PH.vehicle_id_dest = 0;
   PH.total_length = sizeof(t_packet_header) + sizeof(shared_mem_dev_video_bitrate_history);

   u8 packet[MAX_PACKET_TOTAL_SIZE];
   memcpy(packet, (u8*)&PH, sizeof(t_packet_header));
   memcpy(packet + sizeof(t_packet_header), (u8*)&g_SM_DevVideoBitrateHistory, sizeof(shared_mem_dev_video_bitrate_history));
   packets_queue_add_packet(&g_QueueRadioPacketsOut, packet);
}

// returns 1 if main loop should terminate

int periodicLoop()
{
   s_LoopCounter++;
   s_debugFramesCount++;

   _check_reinit_sik_interfaces();

   if ( test_link_is_in_progress() )
      test_link_loop();

   if ( ! g_bHasSentVehicleSettingsAtLeastOnce )
   if ( (g_TimeNow > g_TimeStart + 4000) )
   {
      g_bHasSentVehicleSettingsAtLeastOnce = true;

      log_line("Tell rx_commands to generate all model settings to send to controller.");
      t_packet_header PH;
      radio_packet_init(&PH, PACKET_COMPONENT_LOCAL_CONTROL, PACKET_TYPE_LOCAL_CONTROL_VEHICLE_SEND_MODEL_SETTINGS, STREAM_ID_DATA);
      PH.vehicle_id_src = PACKET_COMPONENT_RUBY;
      PH.total_length = sizeof(t_packet_header);

      ruby_ipc_channel_send_message(s_fIPCRouterToCommands, (u8*)&PH, PH.total_length);

      if ( NULL != g_pProcessStats )
         g_pProcessStats->lastIPCOutgoingTime = g_TimeNow;
      if ( NULL != g_pProcessStats )
         g_pProcessStats->lastActiveTime = get_current_timestamp_ms();

      _check_write_filesystem();
   }

   // Save lastest camera params to flash (on camera)
   if ( 0 != g_uTimeToSaveCameraParams )
   if ( g_TimeNow > g_uTimeToSaveCameraParams )
   {
      #ifdef HW_PLATFORM_RASPBERRY
      if ( g_pCurrentModel->isActiveCameraVeye() )
      {
         log_line("Saving Veye camera parameters to flash memory.");
         int nBus = hardware_get_i2c_device_bus_number(I2C_DEVICE_ADDRESS_CAMERA_VEYE);
         char szComm[256];
         if ( g_pCurrentModel->isActiveCameraVeye307() )
            sprintf(szComm, "current_dir=$PWD; cd %s/; ./cs_mipi_i2c.sh -w -f paramsave -b %d; cd $current_dir", VEYE_COMMANDS_FOLDER307, nBus);
         else if ( g_pCurrentModel->isActiveCameraVeye() )
            sprintf(szComm, "current_dir=$PWD; cd %s/; ./veye_mipi_i2c.sh -w -f paramsave -b %d; cd $current_dir", VEYE_COMMANDS_FOLDER, nBus);
         hw_execute_bash_command(szComm, NULL);
      }
      #endif
      g_uTimeToSaveCameraParams = 0;
   }

   if ( radio_stats_periodic_update(&g_SM_RadioStats, NULL, g_TimeNow) )
   {
      // Send them to controller if needed
      bool bSend = false;
      if ( g_pCurrentModel )
      if ( g_pCurrentModel->osd_params.osd_flags2[g_pCurrentModel->osd_params.layout] & OSD_FLAG2_SHOW_VEHICLE_RADIO_INTERFACES_STATS )
          bSend = true;
      //if ( (NULL != g_pCurrentModel) && g_pCurrentModel->bDeveloperMode )
      //    bSend = true;

      static u32 sl_uLastTimeSentRadioInterfacesStats = 0;
      u32 uSendInterval = g_SM_RadioStats.refreshIntervalMs;
      if ( g_SM_RadioStats.graphRefreshIntervalMs < (int)uSendInterval )
         uSendInterval = g_SM_RadioStats.graphRefreshIntervalMs;

      if ( uSendInterval < 100 )
         uSendInterval = 100;
      if ( g_TimeNow >= sl_uLastTimeSentRadioInterfacesStats + uSendInterval )
      if ( bSend )
      {
         _send_radio_stats_to_controller();
         sl_uLastTimeSentRadioInterfacesStats = uSendInterval;
      }
   }

   //_periodic_loop_check_ping();

#ifdef FEATURE_ENABLE_RC_FREQ_SWITCH
   if ( (s_iPendingFrequencyChangeLinkId >= 0) && (s_uPendingFrequencyChangeTo > 100) &&
        (s_uTimeFrequencyChangeRequest != 0) && (g_TimeNow > s_uTimeFrequencyChangeRequest + VEHICLE_SWITCH_FREQUENCY_AFTER_MS) )
   {
      log_line("Processing pending RC trigger to change frequency to: %s on link: %d", str_format_frequency(s_uPendingFrequencyChangeTo), s_iPendingFrequencyChangeLinkId+1 );
      g_pCurrentModel->compute_active_radio_frequencies(true);

      for( int i=0; i<g_pCurrentModel->nic_count; i++ )
      {
         if ( g_pCurrentModel->nic_flags[i] & NIC_FLAG_DISABLED )
            continue;
         if ( i == s_iPendingFrequencyChangeLinkId )
         {
            radio_utils_set_interface_frequency(g_pCurrentModel, i, g_pCurrentModel->radioInterfacesParams.interface_link_id[i], s_uPendingFrequencyChangeTo, g_pProcessStats, 0); 
            g_pCurrentModel->nic_frequency[i] = s_uPendingFrequencyChangeTo;
         }
      }
      hardware_save_radio_info();
      g_pCurrentModel->compute_active_radio_frequencies(true);
      saveCurrentModel();
      log_line("Notifying all other components of the new link frequency.");

      t_packet_header PH;
      radio_packet_init(&PH, PACKET_COMPONENT_LOCAL_CONTROL, PACKET_TYPE_LOCAL_CONTROL_LINK_FREQUENCY_CHANGED, STREAM_ID_DATA);
      PH.vehicle_id_src = PACKET_COMPONENT_RUBY;
      PH.vehicle_id_dest = 0;
      PH.total_length = sizeof(t_packet_header) + 2*sizeof(u32);
   
      u8 buffer[MAX_PACKET_TOTAL_SIZE];
      memcpy(buffer, (u8*)&PH, sizeof(t_packet_header));
      u32* pI = (u32*)((&buffer[0])+sizeof(t_packet_header));
      *pI = (u32)s_iPendingFrequencyChangeLinkId;
      pI++;
      *pI = s_uPendingFrequencyChangeTo;
      
      radio_packet_compute_crc(buffer, PH.total_length);

      if ( NULL != g_pProcessStats )
         g_pProcessStats->lastIPCOutgoingTime = g_TimeNow;  

      write(s_fPipeTelemetryUplink, buffer, PH.total_length);
      write(s_fPipeToCommands, buffer, PH.total_length);
      log_line("Done notifying all other components about the frequency change.");
      s_iPendingFrequencyChangeLinkId = -1;
      s_uPendingFrequencyChangeTo = 0;
      s_uTimeFrequencyChangeRequest = 0;
   }
#endif

   int iMaxRxQuality = 0;
   for( int i=0; i<g_pCurrentModel->radioInterfacesParams.interfaces_count; i++ )
      if ( g_SM_RadioStats.radio_interfaces[i].rxQuality > iMaxRxQuality )
         iMaxRxQuality = g_SM_RadioStats.radio_interfaces[i].rxQuality;
        
   if ( g_SM_VideoLinkGraphs.vehicleRXQuality[0] == 255 || (iMaxRxQuality < g_SM_VideoLinkGraphs.vehicleRXQuality[0]) )
      g_SM_VideoLinkGraphs.vehicleRXQuality[0] = iMaxRxQuality;


   if ( g_TimeNow >= g_TimeLastPacketsOutPerSecCalculation + 500 )
   {
      g_TimeLastPacketsOutPerSecCalculation = g_TimeNow;
      s_countTXVideoPacketsOutPerSec[1] = s_countTXVideoPacketsOutPerSec[0] = 0;
      s_countTXDataPacketsOutPerSec[1] = s_countTXDataPacketsOutPerSec[0] = 0;
      s_countTXCompactedPacketsOutPerSec[1] = s_countTXCompactedPacketsOutPerSec[0] = 0;

      s_countTXVideoPacketsOutPerSec[0] = s_countTXVideoPacketsOutTemp;
      s_countTXDataPacketsOutPerSec[0] = s_countTXDataPacketsOutTemp;
      s_countTXCompactedPacketsOutPerSec[0] = s_countTXCompactedPacketsOutTemp;

      s_countTXVideoPacketsOutTemp = 0;
      s_countTXDataPacketsOutTemp = 0;
      s_countTXCompactedPacketsOutTemp = 0;

      if ( g_iGetSiKConfigAsyncResult != 0 )
      {
         char szBuff[256];
         strcpy(szBuff, "SiK config: done.");

         if ( 1 == g_iGetSiKConfigAsyncResult )
         {
            hardware_radio_sik_save_configuration();
            hardware_save_radio_info();
            radio_hw_info_t* pRadioHWInfo = hardware_get_radio_info(g_iGetSiKConfigAsyncRadioInterfaceIndex);
         
            char szTmp[256];
            szTmp[0] = 0;
            for( int i=0; i<16; i++ )
            {
               char szB[32];
               sprintf(szB, "%u\n", pRadioHWInfo->uHardwareParamsList[i]);
               strcat(szTmp, szB);
            }
            strcpy(szBuff, szTmp);
         }
         else
            strcpy(szBuff, "Failed to get SiK configuration from device.");

         t_packet_header PH;
         radio_packet_init(&PH, PACKET_COMPONENT_RUBY, PACKET_TYPE_SIK_CONFIG, STREAM_ID_DATA);
         PH.vehicle_id_src = g_pCurrentModel->uVehicleId;
         PH.vehicle_id_dest = g_uControllerId;
         PH.total_length = sizeof(t_packet_header) + strlen(szBuff)+3*sizeof(u8);

         u8 uCommandId = 0;

         u8 packet[MAX_PACKET_TOTAL_SIZE];
         memcpy(packet, (u8*)&PH, sizeof(t_packet_header));
         memcpy(packet+sizeof(t_packet_header), &g_uGetSiKConfigAsyncVehicleLinkIndex, sizeof(u8));
         memcpy(packet+sizeof(t_packet_header) + sizeof(u8), &uCommandId, sizeof(u8));
         memcpy(packet+sizeof(t_packet_header) + 2*sizeof(u8), szBuff, strlen(szBuff)+1);
         packets_queue_add_packet(&g_QueueRadioPacketsOut, packet);

         log_line("Send back to radio Sik current config for vehicle radio link %d", (int)g_uGetSiKConfigAsyncVehicleLinkIndex+1);
         g_iGetSiKConfigAsyncResult = 0;
      }
   }


   if ( g_TimeNow >= g_TimeLastDebugFPSComputeTime + 1000 )
   {
      char szFile[128];
      strcpy(szFile, FOLDER_RUBY_TEMP);
      strcat(szFile, FILE_TEMP_REINIT_RADIO_REQUEST);
      if( access(szFile, R_OK) != -1 )
      {
         log_line("Received signal to reinitialize the radio modules.");
         reinit_radio_interfaces();
         return 1;
      }

      if ( NULL != g_pProcessStats )
         g_pProcessStats->lastActiveTime = g_TimeNow;

      //log_line("Loop FPS: %d", s_debugFramesCount);
      g_TimeLastDebugFPSComputeTime = g_TimeNow;
      s_debugFramesCount = 0;


      if (( g_TimeNow > g_TimeStart+50000 ) || g_bReceivedPairingRequest )
      {
          if ( (NULL != g_pCurrentModel) && (g_pCurrentModel->audio_params.has_audio_device) && (g_pCurrentModel->audio_params.enabled) )
          if ( ! g_pProcessorTxAudio->isAudioStreamOpened() )
          {
             vehicle_launch_audio_capture(g_pCurrentModel);
             g_pProcessorTxAudio->openAudioStream();
          }
      }

      s_MinVideoBlocksGapMilisec = 500/(1+s_debugVideoBlocksInCount);
      if ( s_debugVideoBlocksInCount >= 500 )
         s_MinVideoBlocksGapMilisec = 0;
      if ( s_MinVideoBlocksGapMilisec > 40 )
         s_MinVideoBlocksGapMilisec = 40;

      s_debugVideoBlocksInCount = 0;
   }

   if ( g_bRadioReinitialized )
   {
      if ( g_TimeNow < 5000 || g_TimeNow < g_TimeRadioReinitialized+5000 )
      {
         if ( (g_TimeNow/100)%2 )
            send_radio_reinitialized_message();
      }
      else
      {
         g_bRadioReinitialized = false;
         g_TimeRadioReinitialized = 0;
      }
   }

   if ( g_pCurrentModel->uDeveloperFlags & DEVELOPER_FLAGS_BIT_LIVE_LOG )
   if ( g_TimeNow > g_TimeLastLiveLogCheck + 100 )
   {
      g_TimeLastLiveLogCheck = g_TimeNow;
      char szFile[256];
      strcpy(szFile, FOLDER_LOGS);
      strcat(szFile, LOG_FILE_SYSTEM);
      FILE* fd = fopen(szFile, "rb");
      if ( NULL != fd )
      {
         fseek(fd, 0, SEEK_END);
         long lSize = ftell(fd);
         if ( -1 == s_lLastLiveLogFileOffset )
            s_lLastLiveLogFileOffset = lSize;

         while ( lSize - s_lLastLiveLogFileOffset >= 100 )
         {
            fseek(fd, s_lLastLiveLogFileOffset, SEEK_SET);
            u8 buffer[1024];
            long lRead = fread(buffer, 1, 1023, fd);
            if ( lRead > 0 )
            {
               s_lLastLiveLogFileOffset += lRead;
               send_packet_vehicle_log(buffer, (int)lRead);
            }
            else
               break;
         }
         fclose(fd);
      }
   }

   if ( g_TimeNow >= g_TimeLastHistoryTxComputation + 50 )
   {
      g_TimeLastHistoryTxComputation = g_TimeNow;
      
      // Compute the averate tx gap

      g_PHVehicleTxStats.historyTxGapAvgMiliseconds[0] = 0xFF;
      if ( g_PHVehicleTxStats.tmp_uAverageTxCount > 1 )
         g_PHVehicleTxStats.historyTxGapAvgMiliseconds[0] = (g_PHVehicleTxStats.tmp_uAverageTxSum - g_PHVehicleTxStats.historyTxGapMaxMiliseconds[0])/(g_PHVehicleTxStats.tmp_uAverageTxCount-1);
      else if ( g_PHVehicleTxStats.tmp_uAverageTxCount == 1 )
         g_PHVehicleTxStats.historyTxGapAvgMiliseconds[0] = g_PHVehicleTxStats.historyTxGapMaxMiliseconds[0];

      // Compute average video packets interval        

      g_PHVehicleTxStats.historyVideoPacketsGapAvg[0] = 0xFF;
      if ( g_PHVehicleTxStats.tmp_uVideoIntervalsCount > 1 )
         g_PHVehicleTxStats.historyVideoPacketsGapAvg[0] = (g_PHVehicleTxStats.tmp_uVideoIntervalsSum - g_PHVehicleTxStats.historyVideoPacketsGapMax[0])/(g_PHVehicleTxStats.tmp_uVideoIntervalsCount-1);
      else if ( g_PHVehicleTxStats.tmp_uVideoIntervalsCount == 1 )
         g_PHVehicleTxStats.historyVideoPacketsGapAvg[0] = g_PHVehicleTxStats.historyVideoPacketsGapMax[0];
        

      if ( ! g_bVideoPaused )
      if ( g_pCurrentModel->bDeveloperMode )
      if ( g_pCurrentModel->uDeveloperFlags & DEVELOPER_FLAGS_BIT_SEND_BACK_VEHICLE_TX_GAP )
      {
         t_packet_header PH;
         radio_packet_init(&PH, PACKET_COMPONENT_TELEMETRY, PACKET_TYPE_RUBY_TELEMETRY_VEHICLE_TX_HISTORY, STREAM_ID_DATA);
         PH.vehicle_id_src = g_pCurrentModel->uVehicleId;
         PH.vehicle_id_dest = 0;
         PH.total_length = sizeof(t_packet_header) + sizeof(t_packet_header_vehicle_tx_history);

         g_PHVehicleTxStats.iSliceInterval = 50;
         g_PHVehicleTxStats.uCountValues = MAX_HISTORY_VEHICLE_TX_STATS_SLICES;
         u8 packet[MAX_PACKET_TOTAL_SIZE];
         memcpy(packet, (u8*)&PH, sizeof(t_packet_header));
         memcpy(packet + sizeof(t_packet_header), (u8*)&g_PHVehicleTxStats, sizeof(t_packet_header_vehicle_tx_history));
         packets_queue_add_packet(&g_QueueRadioPacketsOut, packet);
      }

      for( int i=MAX_HISTORY_RADIO_STATS_RECV_SLICES-1; i>0; i-- )
      {
         g_PHVehicleTxStats.historyTxGapMaxMiliseconds[i] = g_PHVehicleTxStats.historyTxGapMaxMiliseconds[i-1];
         g_PHVehicleTxStats.historyTxGapMinMiliseconds[i] = g_PHVehicleTxStats.historyTxGapMinMiliseconds[i-1];
         g_PHVehicleTxStats.historyTxGapAvgMiliseconds[i] = g_PHVehicleTxStats.historyTxGapAvgMiliseconds[i-1];
         g_PHVehicleTxStats.historyTxPackets[i] = g_PHVehicleTxStats.historyTxPackets[i-1];
         g_PHVehicleTxStats.historyVideoPacketsGapMax[i] = g_PHVehicleTxStats.historyVideoPacketsGapMax[i-1];
         g_PHVehicleTxStats.historyVideoPacketsGapAvg[i] = g_PHVehicleTxStats.historyVideoPacketsGapAvg[i-1];
      }
      g_PHVehicleTxStats.historyTxGapMaxMiliseconds[0] = 0xFF;
      g_PHVehicleTxStats.historyTxGapMinMiliseconds[0] = 0xFF;
      g_PHVehicleTxStats.historyTxGapAvgMiliseconds[0] = 0xFF;
      g_PHVehicleTxStats.historyTxPackets[0] = 0;
      g_PHVehicleTxStats.historyVideoPacketsGapMax[0] = 0xFF;
      g_PHVehicleTxStats.historyVideoPacketsGapAvg[0] = 0xFF;

      g_PHVehicleTxStats.tmp_uAverageTxSum = 0;
      g_PHVehicleTxStats.tmp_uAverageTxCount = 0;
      g_PHVehicleTxStats.tmp_uVideoIntervalsSum = 0;
      g_PHVehicleTxStats.tmp_uVideoIntervalsCount = 0;
   }
  
   _update_videobitrate_history();

   // If relay params have changed and we have not processed the notification, do it after one second after the change
   if ( g_TimeLastNotificationRelayParamsChanged != 0 )
   if ( g_TimeNow >= g_TimeLastNotificationRelayParamsChanged+1000 )
   {
      relay_on_relay_params_changed();
      g_TimeLastNotificationRelayParamsChanged = 0;
   }

   return 0;
}