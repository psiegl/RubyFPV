
_LDFLAGS := $(LDFLAGS) -lrt -lpcap -lpthread -lwiringPi -Wl,--gc-sections
_CFLAGS := $(CFLAGS) -Wall -O2 -fdata-sections -ffunction-sections
_CPPFLAGS := -Wall -O2 -fdata-sections -ffunction-sections


INCLUDE_CENTRAL := -Imenu -Iosd -I../menu -I../osd -Icode/r_central/menu -Icode/r_central/osd -I../openvg -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -I/usr/include/freetype2

LDFLAGS_CENTRAL := -L/usr/lib/arm-linux-gnueabihf -L../openvg -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm
LDFLAGS_CENTRAL2 := -L/opt/vc/lib/ -lbrcmGLESv2 -lbrcmEGL -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm -lopenmaxil -lbcm_host -lvcos -lvchiq_arm -lmmal  -lmmal_core -lmmal_util -lmmal_vc_client  

FOLDER_BASE=code/base
FOLDER_COMMON=code/common
FOLDER_RADIO=code/radio

FOLDER_START=code/r_start
FOLDER_UTILS=code/r_utils
FOLDER_I2C=code/r_i2c
FOLDER_VEHICLE=code/r_vehicle
FOLDER_STATION=code/r_station
FOLDER_CENTRAL=code/r_central
FOLDER_CENTRAL_MENU=code/r_central/menu
FOLDER_CENTRAL_OSD=code/r_central/osd
FOLDER_CENTRAL_RENDERER=code/renderer
FOLDER_PLUGINS_OSD=code/r_plugins_osd
FOLDER_TESTS=code/r_tests

$(FOLDER_BASE)/%.o: $(FOLDER_BASE)/%.c $(FOLDER_BASE)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_BASE)/%.o: $(FOLDER_BASE)/%.cpp $(FOLDER_BASE)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_COMMON)/%.o: $(FOLDER_COMMON)/%.c $(FOLDER_COMMON)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_COMMON)/%.o: $(FOLDER_COMMON)/%.cpp $(FOLDER_COMMON)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_RADIO)/%.o: $(FOLDER_RADIO)/%.c $(FOLDER_RADIO)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_RADIO)/%.o: $(FOLDER_RADIO)/%.cpp $(FOLDER_RADIO)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_START)/%.o: $(FOLDER_START)/%.c $(FOLDER_START)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_START)/%.o: $(FOLDER_START)/%.cpp $(FOLDER_START)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_UTILS)/%.o: $(FOLDER_UTILS)/%.c $(FOLDER_UTILS)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_UTILS)/%.o: $(FOLDER_UTILS)/%.cpp $(FOLDER_UTILS)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_I2C)/%.o: $(FOLDER_I2C)/%.c $(FOLDER_I2C)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_I2C)/%.o: $(FOLDER_I2C)/%.cpp $(FOLDER_I2C)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_VEHICLE)/%.o: $(FOLDER_VEHICLE)/%.c $(FOLDER_VEHICLE)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_VEHICLE)/%.o: $(FOLDER_VEHICLE)/%.cpp $(FOLDER_VEHICLE)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_STATION)/%.o: $(FOLDER_STATION)/%.c $(FOLDER_STATION)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_STATION)/%.o: $(FOLDER_STATION)/%.cpp $(FOLDER_STATION)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_CENTRAL)/%.o: $(FOLDER_CENTRAL)/%.c $(FOLDER_CENTRAL)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_CENTRAL)/%.o: $(FOLDER_CENTRAL)/%.cpp $(FOLDER_CENTRAL)/*.h
	$(CXX) $(_CFLAGS) $(INCLUDE_CENTRAL) -export-dynamic -c -o $@ $<

$(FOLDER_CENTRAL_MENU)/%.o: $(FOLDER_CENTRAL_MENU)/%.c $(FOLDER_CENTRAL_MENU)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_CENTRAL_MENU)/%.o: $(FOLDER_CENTRAL_MENU)/%.cpp $(FOLDER_CENTRAL_MENU)/*.h
	$(CXX) $(_CFLAGS) $(INCLUDE_CENTRAL) -export-dynamic -c -o $@ $<

$(FOLDER_CENTRAL_OSD)/%.o: $(FOLDER_CENTRAL_OSD)/%.c $(FOLDER_CENTRAL_OSD)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_CENTRAL_OSD)/%.o: $(FOLDER_CENTRAL_OSD)/%.cpp $(FOLDER_CENTRAL_OSD)/*.h
	$(CXX) $(_CFLAGS) $(INCLUDE_CENTRAL) -export-dynamic -c -o $@ $<

$(FOLDER_CENTRAL_RENDERER)/%.o: $(FOLDER_CENTRAL_RENDERER)/%.c $(FOLDER_CENTRAL_RENDERER)/*.h
	$(CC) $(_CFLAGS) $(INCLUDE_CENTRAL) -c -o $@ $<

$(FOLDER_CENTRAL_RENDERER)/%.o: $(FOLDER_CENTRAL_RENDERER)/%.cpp $(FOLDER_CENTRAL_RENDERER)/*.h
	$(CXX) $(_CFLAGS) $(INCLUDE_CENTRAL) -export-dynamic -c -o $@ $<

$(FOLDER_PLUGINS_OSD)/%.o: $(FOLDER_PLUGINS_OSD)/%.c $(FOLDER_PLUGINS_OSD)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_PLUGINS_OSD)/%.o: $(FOLDER_PLUGINS_OSD)/%.cpp $(FOLDER_PLUGINS_OSD)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

$(FOLDER_TESTS)/%.o: $(FOLDER_TESTS)/%.c $(FOLDER_TESTS)/*.h
	$(CC) $(_CFLAGS) -c -o $@ $<

$(FOLDER_TESTS)/%.o: $(FOLDER_TESTS)/%.cpp $(FOLDER_TESTS)/*.h
	$(CXX) $(_CFLAGS) -c -o $@ $<

core_plugins_utils.o: code/public/utils/core_plugins_utils.c
	$(CC) $(_CFLAGS) -c -o $@ $<

osd_plugins_utils.o: code/public/utils/osd_plugins_utils.c
	$(CC) $(_CFLAGS) -c -o $@ $<

MODULE_BASE := $(FOLDER_BASE)/base.o $(FOLDER_BASE)/shared_mem.o $(FOLDER_BASE)/config.o $(FOLDER_BASE)/hardware.o $(FOLDER_BASE)/hw_procs.o $(FOLDER_BASE)/utils.o $(FOLDER_BASE)/encr.o $(FOLDER_BASE)/hardware_i2c.o $(FOLDER_BASE)/alarms.o $(FOLDER_BASE)/hardware_radio.o $(FOLDER_BASE)/hardware_serial.o $(FOLDER_BASE)/hardware_radio_sik.o $(FOLDER_BASE)/ruby_ipc.o $(FOLDER_BASE)/enc.o $(FOLDER_BASE)/commands.o
MODULE_BASE2 := $(FOLDER_BASE)/gpio.o $(FOLDER_BASE)/ctrl_settings.o $(FOLDER_BASE)/controller_utils.o $(FOLDER_BASE)/ctrl_preferences.o $(FOLDER_BASE)/ctrl_interfaces.o
MODULE_COMMON := $(FOLDER_COMMON)/string_utils.o $(FOLDER_COMMON)/relay_utils.o
MODULE_MODELS := $(FOLDER_BASE)/models.o $(FOLDER_BASE)/models_list.o
MODULE_RADIO := $(FOLDER_COMMON)/radio_stats.o $(FOLDER_RADIO)/fec.o $(FOLDER_RADIO)/zfec.o $(FOLDER_RADIO)/radio_duplicate_det.o $(FOLDER_RADIO)/radio_rx.o $(FOLDER_RADIO)/radio_tx.o $(FOLDER_RADIO)/radiolink.o $(FOLDER_RADIO)/radiopackets_rc.o $(FOLDER_RADIO)/radiopackets_short.o $(FOLDER_RADIO)/radiopackets_wfbohd.o $(FOLDER_RADIO)/radiopackets2.o $(FOLDER_RADIO)/radiopacketsqueue.o $(FOLDER_RADIO)/radiotap.o
MODULE_VEHICLE := $(FOLDER_VEHICLE)/shared_vars.o $(FOLDER_VEHICLE)/timers.o $(FOLDER_VEHICLE)/utils_vehicle.o $(FOLDER_VEHICLE)/launchers_vehicle.o
MODULE_STATION := $(FOLDER_STATION)/shared_vars.o $(FOLDER_STATION)/timers.o


CENTRAL_MENU_ITEMS_ALL := $(FOLDER_CENTRAL_MENU)/menu_items.o $(FOLDER_CENTRAL_MENU)/menu_item_select_base.o $(FOLDER_CENTRAL_MENU)/menu_item_select.o $(FOLDER_CENTRAL_MENU)/menu_item_slider.o $(FOLDER_CENTRAL_MENU)/menu_item_range.o $(FOLDER_CENTRAL_MENU)/menu_item_edit.o $(FOLDER_CENTRAL_MENU)/menu_item_section.o $(FOLDER_CENTRAL_MENU)/menu_item_text.o $(FOLDER_CENTRAL_MENU)/menu_item_legend.o $(FOLDER_CENTRAL_MENU)/menu_item_checkbox.o $(FOLDER_CENTRAL_MENU)/menu_item_radio.o
CENTRAL_MENU_ALL1 := $(FOLDER_CENTRAL_MENU)/menu.o $(FOLDER_CENTRAL_MENU)/menu_objects.o $(FOLDER_CENTRAL_MENU)/menu_preferences_buttons.o $(FOLDER_CENTRAL_MENU)/menu_root.o $(FOLDER_CENTRAL_MENU)/menu_search.o $(FOLDER_CENTRAL_MENU)/menu_spectator.o $(FOLDER_CENTRAL_MENU)/menu_vehicles.o $(FOLDER_CENTRAL_MENU)/menu_vehicle.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_general.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_expert.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_video.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_camera.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_osd.o $(FOLDER_CENTRAL_MENU)/menu_confirmation.o $(FOLDER_CENTRAL_MENU)/menu_storage.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_telemetry.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_video_encodings.o $(FOLDER_CENTRAL_MENU)/menu_text.o $(FOLDER_CENTRAL_MENU)/menu_txinfo.o $(FOLDER_CENTRAL_MENU)/menu_calibrate_hdmi.o $(FOLDER_CENTRAL_MENU)/menu_confirmation_import_key.o
CENTRAL_MENU_ALL2 := $(FOLDER_CENTRAL_MENU)/menu_vehicle_relay.o $(FOLDER_CENTRAL_MENU)/menu_controller.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_alarms.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_camera_gains.o $(FOLDER_CENTRAL_MENU)/menu_controller_peripherals.o $(FOLDER_CENTRAL_MENU)/menu_controller_expert.o $(FOLDER_CENTRAL_MENU)/menu_controller_radio_interfaces.o $(FOLDER_CENTRAL_MENU)/menu_system.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_osd_instruments.o $(FOLDER_CENTRAL_MENU)/menu_preferences_ui.o $(FOLDER_CENTRAL_MENU)/menu_system_expert.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_video_profile.o $(FOLDER_CENTRAL_MENU)/menu_confirmation_import.o $(FOLDER_CENTRAL_MENU)/menu_system_alarms.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_selector.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_osd_widgets.o
CENTRAL_MENU_ALL3 := $(FOLDER_CENTRAL_MENU)/menu_vehicle_management.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_import.o $(FOLDER_CENTRAL_MENU)/menu_switch_vehicle.o $(FOLDER_CENTRAL_MENU)/menu_controller_joystick.o $(FOLDER_CENTRAL_MENU)/menu_system_all_params.o $(FOLDER_CENTRAL_MENU)/menu_color_picker.o $(FOLDER_CENTRAL_MENU)/menu_controller_video.o $(FOLDER_CENTRAL_MENU)/menu_controller_telemetry.o $(FOLDER_CENTRAL_MENU)/menu_update_vehicle.o $(FOLDER_CENTRAL_MENU)/menu_device_i2c.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_osd_stats.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_audio.o $(FOLDER_CENTRAL_MENU)/menu_channels_select.o $(FOLDER_CENTRAL_MENU)/menu_tx_power_max.o $(FOLDER_CENTRAL_MENU)/menu_system_dev_logs.o $(FOLDER_CENTRAL_MENU)/menu_item_vehicle.o $(FOLDER_CENTRAL_MENU)/menu_radio_config.o $(FOLDER_CENTRAL_MENU)/menu_preferences.o
CENTRAL_MENU_ALL4 := $(FOLDER_CENTRAL_MENU)/menu_vehicle_data_link.o $(FOLDER_CENTRAL_MENU)/menu_controller_network.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_osd_plugins.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_instruments_general.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_osd_elements.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_osd_plugin.o $(FOLDER_CENTRAL_MENU)/menu_controller_plugins.o $(FOLDER_CENTRAL_MENU)/menu_controller_encryption.o $(FOLDER_CENTRAL_MENU)/menu_search_connect.o $(FOLDER_CENTRAL_MENU)/menu_system_hardware.o $(FOLDER_CENTRAL_MENU)/menu_confirmation_hdmi.o $(FOLDER_CENTRAL_MENU)/menu_controller_recording.o $(FOLDER_CENTRAL_MENU)/menu_system_video_profiles.o $(FOLDER_CENTRAL_MENU)/menu_info_booster.o $(FOLDER_CENTRAL_MENU)/menu_controller_radio_interface.o
CENTRAL_MENU_ALL5 := $(FOLDER_CENTRAL_MENU)/menu_system_dev_stats.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_radio_link.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_radio_interface.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_management_plugins.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_peripherals.o $(FOLDER_CENTRAL_MENU)/menu_confirmation_delete_logs.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_radio.o
CENTRAL_MENU_RC := $(FOLDER_CENTRAL_MENU)/menu_vehicle_rc.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_rc_failsafe.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_rc_channels.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_rc_expo.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_rc_camera.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_rc_input.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_functions.o
CENTRAL_MENU_RADIO := $(FOLDER_CENTRAL_MENU)/menu_controller_radio_interface_sik.o $(FOLDER_CENTRAL_MENU)/menu_vehicle_radio_link_sik.o $(FOLDER_CENTRAL_MENU)/menu_diagnose_radio_link.o
CENTRAL_POPUP_ALL := $(FOLDER_CENTRAL)/popup.o $(FOLDER_CENTRAL)/popup_log.o $(FOLDER_CENTRAL)/popup_commands.o $(FOLDER_CENTRAL)/popup_camera_params.o
CENTRAL_RENDER_ALL := $(FOLDER_CENTRAL)/colors.o $(FOLDER_CENTRAL)/render_commands.o $(FOLDER_CENTRAL)/render_joysticks.o $(FOLDER_CENTRAL)/process_router_messages.o
CENTRAL_RENDER_RAW := $(FOLDER_CENTRAL_RENDERER)/lodepng.o $(FOLDER_CENTRAL_RENDERER)/nanojpeg.o $(FOLDER_CENTRAL_RENDERER)/fbgraphics.o $(FOLDER_CENTRAL_RENDERER)/render_engine.o $(FOLDER_CENTRAL_RENDERER)/render_engine_raw.o $(FOLDER_CENTRAL_RENDERER)/render_engine_ui.o $(FOLDER_CENTRAL_RENDERER)/fbg_dispmanx.o
CENTRAL_OSD_ALL := $(FOLDER_CENTRAL_OSD)/osd_common.o $(FOLDER_CENTRAL_OSD)/osd.o $(FOLDER_CENTRAL_OSD)/osd_stats.o $(FOLDER_CENTRAL_OSD)/osd_ahi.o $(FOLDER_CENTRAL_OSD)/osd_lean.o $(FOLDER_CENTRAL_OSD)/osd_warnings.o $(FOLDER_CENTRAL_OSD)/osd_gauges.o $(FOLDER_CENTRAL_OSD)/osd_plugins.o $(FOLDER_CENTRAL_OSD)/osd_stats_dev.o $(FOLDER_CENTRAL_OSD)/osd_links.o $(FOLDER_CENTRAL_OSD)/osd_stats_radio.o $(FOLDER_CENTRAL_OSD)/osd_widgets.o
CENTRAL_ALL := $(FOLDER_CENTRAL)/notifications.o $(FOLDER_CENTRAL)/launchers_controller.o $(FOLDER_CENTRAL)/local_stats.o $(FOLDER_CENTRAL)/rx_scope.o $(FOLDER_CENTRAL)/forward_watch.o $(FOLDER_CENTRAL)/timers.o $(FOLDER_CENTRAL)/ui_alarms.o $(FOLDER_CENTRAL)/media.o $(FOLDER_CENTRAL)/pairing.o $(FOLDER_CENTRAL)/link_watch.o $(FOLDER_CENTRAL)/warnings.o $(FOLDER_CENTRAL)/handle_commands.o $(FOLDER_CENTRAL)/events.o $(FOLDER_CENTRAL)/shared_vars_ipc.o $(FOLDER_CENTRAL)/shared_vars_state.o $(FOLDER_CENTRAL)/shared_vars_osd.o $(FOLDER_CENTRAL)/fonts.o $(FOLDER_CENTRAL)/keyboard.o $(FOLDER_CENTRAL)/quickactions.o $(FOLDER_CENTRAL)/shared_vars.o
CENTRAL_RADIO := $(FOLDER_RADIO)/radiopackets2.o $(FOLDER_RADIO)/radiopackets_short.o $(FOLDER_RADIO)/radiolink.o $(FOLDER_RADIO)/radiotap.o 

all: vehicle controller ruby_i2c ruby_central ruby_plugins

vehicle: ruby_start ruby_utils ruby_vehicle ruby_rx_rc ruby_rx_commands ruby_tx_telemetry ruby_rt_vehicle

controller: ruby_start ruby_utils ruby_controller ruby_rt_station ruby_tx_rc ruby_rx_telemetry


ruby_central: $(FOLDER_CENTRAL)/ruby_central.o $(MODULE_BASE) $(MODULE_MODELS) $(MODULE_COMMON) $(MODULE_BASE2) $(CENTRAL_MENU_ITEMS_ALL) $(CENTRAL_MENU_ALL1) $(CENTRAL_RENDER_RAW) $(CENTRAL_MENU_ALL2) $(CENTRAL_MENU_ALL3) $(CENTRAL_MENU_ALL4) $(CENTRAL_MENU_ALL5)  $(CENTRAL_MENU_RC)  $(CENTRAL_MENU_RADIO) $(CENTRAL_POPUP_ALL) $(CENTRAL_RENDER_ALL) $(CENTRAL_OSD_ALL) $(CENTRAL_ALL) $(CENTRAL_RADIO) $(FOLDER_BASE)/shared_mem_controller_only.o $(FOLDER_BASE)/hdmi.o $(FOLDER_COMMON)/favorites.o $(FOLDER_BASE)/plugins_settings.o \
	$(FOLDER_BASE)/core_plugins_settings.o $(FOLDER_COMMON)/models_connect_frequencies.o $(FOLDER_BASE)/shared_mem_i2c.o $(FOLDER_BASE)/hdmi_video.o
	$(CXX) -export-dynamic -o $@ $^ $(_LDFLAGS) -ldl -lfreetype -lpng -ljpeg $(LDFLAGS_CENTRAL) $(LDFLAGS_CENTRAL2)


ruby_utils: ruby_timeinit ruby_logger ruby_initdhcp ruby_initradio ruby_sik_config ruby_alive ruby_video_proc ruby_gpio_detect ruby_update ruby_update_worker

ruby_start: $(FOLDER_START)/ruby_start.o $(MODULE_BASE) $(MODULE_MODELS) $(MODULE_COMMON) $(MODULE_BASE2)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_i2c: $(FOLDER_I2C)/ruby_i2c.o $(MODULE_BASE) $(MODULE_MODELS) $(MODULE_COMMON) $(MODULE_BASE2) $(FOLDER_BASE)/shared_mem_i2c.o
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_timeinit: $(FOLDER_UTILS)/ruby_timeinit.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_logger: $(FOLDER_UTILS)/ruby_logger.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_initdhcp: $(FOLDER_UTILS)/ruby_initdhcp.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_initradio: $(FOLDER_UTILS)/ruby_initradio.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_sik_config: $(FOLDER_UTILS)/ruby_sik_config.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_alive: $(FOLDER_UTILS)/ruby_alive.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_video_proc: $(FOLDER_UTILS)/ruby_video_proc.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_gpio_detect: $(FOLDER_UTILS)/ruby_gpio_detect.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_update: $(FOLDER_UTILS)/ruby_update.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON) $(FOLDER_BASE)/vehicle_settings.o
	$(CXX) -o $@ $^ $(_LDFLAGS)

ruby_update_worker: $(FOLDER_UTILS)/ruby_update_worker.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_MODELS) $(MODULE_COMMON)
	$(CXX) -o $@ $^ $(_LDFLAGS)
 
ruby_vehicle: $(FOLDER_VEHICLE)/ruby_vehicle.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_VEHICLE) $(FOLDER_VEHICLE)/hw_config_check.o
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium

ruby_rx_rc: $(FOLDER_VEHICLE)/ruby_rx_rc.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_VEHICLE)
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium

ruby_rx_commands: $(FOLDER_VEHICLE)/ruby_rx_commands.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_VEHICLE) $(FOLDER_VEHICLE)/process_upload.o $(FOLDER_BASE)/core_plugins_settings.o $(FOLDER_BASE)/vehicle_settings.o
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium -ldl

ruby_tx_telemetry: $(FOLDER_VEHICLE)/ruby_tx_telemetry.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_VEHICLE) $(FOLDER_BASE)/parse_fc_telemetry.o $(FOLDER_BASE)/parse_fc_telemetry_ltm.o $(FOLDER_BASE)/vehicle_settings.o
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium

ruby_rt_vehicle: $(FOLDER_VEHICLE)/ruby_rt_vehicle.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_VEHICLE) $(FOLDER_BASE)/vehicle_settings.o $(FOLDER_VEHICLE)/processor_relay.o $(FOLDER_VEHICLE)/processor_tx_video.o $(FOLDER_VEHICLE)/processor_tx_audio.o $(FOLDER_VEHICLE)/events.o $(FOLDER_VEHICLE)/packets_utils.o $(FOLDER_VEHICLE)/process_local_packets.o $(FOLDER_VEHICLE)/process_radio_in_packets.o $(FOLDER_VEHICLE)/process_received_ruby_messages.o $(FOLDER_VEHICLE)/radio_links.o $(FOLDER_VEHICLE)/video_link_auto_keyframe.o $(FOLDER_VEHICLE)/video_link_check_bitrate.o $(FOLDER_VEHICLE)/video_link_stats_overwrites.o $(FOLDER_BASE)/camera_utils.o $(FOLDER_VEHICLE)/test_link_params.o $(FOLDER_BASE)/radio_utils.o
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium

ruby_controller: $(FOLDER_STATION)/ruby_controller.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_STATION)
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium

ruby_rx_telemetry: $(FOLDER_STATION)/ruby_rx_telemetry.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_STATION)
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium

ruby_tx_rc: $(FOLDER_STATION)/ruby_tx_rc.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_STATION) $(FOLDER_BASE)/shared_mem_i2c.o
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium

ruby_rt_station: $(FOLDER_STATION)/ruby_rt_station.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS) $(MODULE_STATION) $(FOLDER_STATION)/links_utils.o $(FOLDER_STATION)/packets_utils.o $(FOLDER_STATION)/process_local_packets.o $(FOLDER_STATION)/process_radio_in_packets.o $(FOLDER_STATION)/processor_rx_audio.o $(FOLDER_STATION)/processor_rx_video.o $(FOLDER_STATION)/processor_rx_video_wfbohd.o $(FOLDER_STATION)/radio_links.o $(FOLDER_STATION)/relay_rx.o $(FOLDER_STATION)/test_link_params.o $(FOLDER_STATION)/rx_video_output.o $(FOLDER_STATION)/video_link_adaptive.o $(FOLDER_STATION)/video_link_keyframe.o $(FOLDER_STATION)/wfbohd.o $(FOLDER_BASE)/shared_mem_controller_only.o $(FOLDER_COMMON)/models_connect_frequencies.o $(FOLDER_BASE)/parse_fc_telemetry.o $(FOLDER_BASE)/parse_fc_telemetry_ltm.o $(FOLDER_STATION)/radio_links_sik.o $(FOLDER_BASE)/radio_utils.o $(FOLDER_BASE)/core_plugins_settings.o $(FOLDER_BASE)/camera_utils.o
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium -ldl

ruby_plugins: ruby_plugin_osd_ahi ruby_plugin_gauge_speed ruby_plugin_gauge_altitude ruby_plugin_gauge_ahi ruby_plugin_gauge_heading

ruby_plugin_osd_ahi: $(FOLDER_PLUGINS_OSD)/ruby_plugin_osd_ahi.o osd_plugins_utils.o core_plugins_utils.o
	gcc $(FOLDER_PLUGINS_OSD)/ruby_plugin_osd_ahi.o osd_plugins_utils.o core_plugins_utils.o -shared -Wl,-soname,ruby_plugin_osd_ahi.so.1 -o ruby_plugin_osd_ahi.so.1.0.1 -lc

ruby_plugin_gauge_speed: $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_speed.o osd_plugins_utils.o core_plugins_utils.o
	gcc $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_speed.o osd_plugins_utils.o core_plugins_utils.o -shared -Wl,-soname,ruby_plugin_gauge_speed.so.1 -o ruby_plugin_gauge_speed.so.1.0.1 -lc

ruby_plugin_gauge_altitude: $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_altitude.o osd_plugins_utils.o core_plugins_utils.o
	gcc $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_altitude.o osd_plugins_utils.o core_plugins_utils.o -shared -Wl,-soname,ruby_plugin_gauge_altitude.so.1 -o ruby_plugin_gauge_altitude.so.1.0.1 -lc

ruby_plugin_gauge_ahi: $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_ahi.o osd_plugins_utils.o core_plugins_utils.o
	gcc $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_ahi.o osd_plugins_utils.o core_plugins_utils.o -shared -Wl,-soname,ruby_plugin_gauge_ahi.so.1 -o ruby_plugin_gauge_ahi.so.1.0.1 -lc

ruby_plugin_gauge_heading: $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_heading.o osd_plugins_utils.o core_plugins_utils.o
	gcc $(FOLDER_PLUGINS_OSD)/ruby_plugin_gauge_heading.o osd_plugins_utils.o core_plugins_utils.o -shared -Wl,-soname,ruby_plugin_gauge_heading.so.1 -o ruby_plugin_gauge_heading.so.1.0.1 -lc

tests: test_log

test_log:$(FOLDER_TESTS)/test_log.o core_plugins_utils.o $(MODULE_BASE) $(MODULE_BASE2) $(MODULE_COMMON) $(MODULE_RADIO) $(MODULE_MODELS)
	$(CXX) -o $@ $^ $(_LDFLAGS) -lsodium -ldl -lc

clean:
	rm -rf ruby_start ruby_i2c ruby_timeinit ruby_logger ruby_initdhcp ruby_initradio ruby_sik_config ruby_alive ruby_video_proc ruby_gpio_detect ruby_update ruby_update_worker \
          ruby_vehicle ruby_rx_rc ruby_rx_commands ruby_tx_telemetry ruby_rt_vehicle \
          ruby_controller ruby_rt_station ruby_tx_rc ruby_rx_telemetry \
          ruby_central $(FOLDER_CENTRAL)/ruby_central test_log $(FOLDER_TESTS)/test_log ruby_plugin* \
          $(FOLDER_VEHICLE)/ruby_vehicle $(FOLDER_VEHICLE)/ruby_rx_rc $(FOLDER_VEHICLE)/ruby_rx_commands $(FOLDER_VEHICLE)/ruby_tx_telemetry $(FOLDER_VEHICLE)/ruby_rt_vehicle \
          $(FOLDER_STATION)/ruby_controller $(FOLDER_STATION)/ruby_rt_station $(FOLDER_STATION)/ruby_tx_rc $(FOLDER_STATION)/ruby_rx_telemetry \
          $(FOLDER_START)/ruby_start $(FOLDER_I2C)/ruby_i2c $(FOLDER_UTILS)/ruby_timeinit $(FOLDER_UTILS)/ruby_logger $(FOLDER_UTILS)/ruby_initdhcp $(FOLDER_UTILS)/ruby_initradio $(FOLDER_UTILS)/ruby_sik_config $(FOLDER_UTILS)/ruby_alive $(FOLDER_UTILS)/ruby_video_proc $(FOLDER_UTILS)/ruby_gpio_detect $(FOLDER_UTILS)/ruby_update $(FOLDER_UTILS)/ruby_update_worker \
          $(FOLDER_BASE)/*.o $(FOLDER_COMMON)/*.o $(FOLDER_RADIO)/*.o $(FOLDER_START)/*.o $(FOLDER_UTILS)/*.o $(FOLDER_VEHICLE)/*.o $(FOLDER_STATION)/*.o \
          $(FOLDER_CENTRAL)/*.o $(FOLDER_CENTRAL_MENU)/*.o $(FOLDER_CENTRAL_OSD)/*.o $(FOLDER_CENTRAL_RENDERER)/*.o \
          $(FOLDER_PLUGINS_OSD)/*.o code/public/utils/*.o $(FOLDER_TESTS)/*.o