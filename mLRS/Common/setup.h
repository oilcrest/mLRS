//*******************************************************
// Copyright (c) MLRS project
// GPL3
// https://www.gnu.org/licenses/gpl-3.0.de.html
// OlliW @ www.olliw.eu
//*******************************************************
// Setup
//*******************************************************
#ifndef SETUP_H
#define SETUP_H
#pragma once


#include "setup_types.h"


tSetup Setup;
tGlobalConfig Config;


void setup_default(void)
{
  Setup.Tx.Power = 0;
  Setup.Tx.Diversity = SETUP_TX_DIVERSITY;
  Setup.Tx.SerialDestination = SETUP_TX_SERIAL_DESTINATION;
  Setup.Tx.ChannelsSource = SETUP_TX_CHANNELS_SOURCE;
  Setup.Tx.ChannelOrder = SETUP_TX_CHANNEL_ORDER;
  Setup.Tx.InMode = SETUP_TX_IN_MODE;
  Setup.Tx.SerialBaudrate_bytespersec = (SETUP_TX_SERIAL_BAUDRATE / 10);
  Setup.Tx.SerialLinkMode = SETUP_TX_SERIAL_LINK_MODE;
  Setup.Tx.SendRadioStatus = SETUP_TX_SEND_RADIO_STATUS;

  Setup.Rx.Power = 0;
  Setup.Rx.Diversity = SETUP_RX_DIVERSITY;
  Setup.Rx.ChannelOrder = SETUP_RX_CHANNEL_ORDER;
  Setup.Rx.OutMode = SETUP_RX_OUT_MODE;
  Setup.Rx.FailsafeMode = SETUP_RX_FAILSAFE_MODE;
  Setup.Rx.SerialBaudrate_bytespersec = (SETUP_RX_SERIAL_BAUDRATE / 10);
  Setup.Rx.SerialLinkMode = SETUP_RX_SERIAL_LINK_MODE;
  Setup.Rx.SendRadioStatus = SETUP_RX_SEND_RADIO_STATUS;

  Setup.BindDblWord = BIND_DBLWORD;

  Setup.Mode = SETUP_MODE;
}


void setup_sanitize(void)
{
  // device cannot use mBridge (pin5) and CRSF (pin5) at the same time !
  if ((Setup.Tx.SerialDestination == SERIAL_DESTINATION_MBRDIGE) && (Setup.Tx.ChannelsSource == CHANNEL_SOURCE_CRSF)) {
    Setup.Tx.ChannelsSource = CHANNEL_SOURCE_NONE;
  }

#ifdef DEVICE_IS_TRANSMITTER
#ifndef DEVICE_HAS_JRPIN5
  // device doesn't support half-duplex JR pin5
  if (Setup.Tx.SerialDestination == SERIAL_DESTINATION_MBRDIGE) Setup.Tx.SerialDestination = SERIAL_DESTINATION_SERIAL_PORT;
  if (Setup.Tx.ChannelsSource == CHANNEL_SOURCE_MBRIDGE) Setup.Tx.ChannelsSource = CHANNEL_SOURCE_NONE;
  if (Setup.Tx.ChannelsSource == CHANNEL_SOURCE_CRSF) Setup.Tx.ChannelsSource = CHANNEL_SOURCE_NONE;
#else
  if (Setup.Tx.SerialDestination == SERIAL_DESTINATION_MBRDIGE) {
    // mBridge & CRSF cannot be used simultaneously
    if (Setup.Tx.ChannelsSource == CHANNEL_SOURCE_CRSF) Setup.Tx.ChannelsSource = CHANNEL_SOURCE_NONE;
  }
#endif

#ifndef DEVICE_HAS_IN
  if (Setup.Tx.ChannelsSource == CHANNEL_SOURCE_INPORT) Setup.Tx.ChannelsSource = CHANNEL_SOURCE_NONE;
#endif
#endif

#ifndef DEVICE_HAS_DIVERSITY
#ifdef DEVICE_IS_TRANSMITTER
  if (Setup.Tx.Diversity >= DIVERSITY_ANTENNA2) Setup.Tx.Diversity = DIVERSITY_DEFAULT;
#endif
#ifdef DEVICE_IS_RECEIVER
  if (Setup.Rx.Diversity >= DIVERSITY_ANTENNA2) Setup.Rx.Diversity = DIVERSITY_DEFAULT;
#endif
#endif

  if (Setup.Mode >= MODE_NUM) Setup.Mode = MODE_19HZ;
#ifdef DEVICE_HAS_SX126x
  if ((Setup.Mode != MODE_19HZ) && (Setup.Mode != MODE_31HZ)) { // only 19 Hz and 31 hz modes allowed
    Setup.Mode = MODE_19HZ;
  }
#endif
#ifdef DEVICE_HAS_SX127x
  Setup.Mode = MODE_19HZ; // only 19 Hz mode allowed
#endif
}


void setup_configure(void)
{
  // TODO: we momentarily use the POWER values, but eventually we need to use the power_list[] array and setup Rx/Tx power
#ifdef DEVICE_IS_TRANSMITTER
  Config.Power = SETUP_TX_POWER;
#endif
#ifdef DEVICE_IS_RECEIVER
  Config.Power = SETUP_RX_POWER;
#endif

  switch (Setup.Mode) {
  case MODE_50HZ:
    Config.frame_rate_ms = 20; // 20 ms = 50 Hz
    Config.frame_rate_hz = 50;
    Config.LoraConfigIndex = SX128x_LORA_CONFIG_BW800_SF5_CRLI4_5;
    Config.lora_send_frame_tmo = MODE_50HZ_SEND_FRAME_TMO; // 10;
    break;
  case MODE_31HZ:
    Config.frame_rate_ms = 32; // 32 ms = 31.25 Hz
    Config.frame_rate_hz = 31;
#ifdef DEVICE_HAS_SX128x
    Config.LoraConfigIndex = SX128x_LORA_CONFIG_BW800_SF6_CRLI4_5;
#else
    Config.LoraConfigIndex = SX126x_LORA_CONFIG_BW500_SF5_CR4_5;
#endif
    Config.lora_send_frame_tmo = MODE_31HZ_SEND_FRAME_TMO; // 15
    break;
  case MODE_19HZ:
    Config.frame_rate_ms = 53; // 53 ms = 18.9 Hz
    Config.frame_rate_hz = 19;
#ifdef DEVICE_HAS_SX128x
    Config.LoraConfigIndex = SX128x_LORA_CONFIG_BW800_SF7_CRLI4_5;
#elif defined DEVICE_HAS_SX126x
    Config.LoraConfigIndex = SX126x_LORA_CONFIG_BW500_SF6_CR4_5;
#else
    Config.LoraConfigIndex = SX127x_LORA_CONFIG_BW500_SF6_CR4_5;
#endif
    Config.lora_send_frame_tmo = MODE_19HZ_SEND_FRAME_TMO; // 25;
    break;
  default:
    while (1) {} // must not happen
  }

  Config.FrameSyncWord = (uint16_t)(Setup.BindDblWord & 0x0000FFFF);

  Config.FhssSeed = Setup.BindDblWord;
#if defined FREQUENCY_BAND_868_MHZ
  Config.FhssNum = FHSS_NUM_BAND_868_MHZ;
#elif defined FREQUENCY_BAND_915_MHZ_FCC
  Config.FhssNum = FHSS_NUM_BAND_915_MHZ_FCC;
#else
  switch (Setup.Mode) {
  case MODE_50HZ:
    Config.FhssNum = FHSS_NUM_BAND_2P4_GHZ;
    break;
  case MODE_31HZ:
    Config.FhssNum = FHSS_NUM_BAND_2P4_GHZ_31HZ_MODE;
    break;
  case MODE_19HZ:
    Config.FhssNum = FHSS_NUM_BAND_2P4_GHZ_19HZ_MODE;
    break;
  }
#endif

  Config.connect_tmo_systicks = SYSTICK_DELAY_MS((uint16_t)( (float)CONNECT_TMO_MS + 0.75f * Config.frame_rate_ms));
  Config.connect_listen_hop_cnt = (uint8_t)(1.5f * Config.FhssNum);

  Config.LQAveragingPeriod = (LQ_AVERAGING_MS/Config.frame_rate_ms);

#ifdef DEVICE_HAS_DIVERSITY
#ifdef DEVICE_IS_TRANSMITTER
  switch (Setup.Tx.Diversity) {
#endif
#ifdef DEVICE_IS_RECEIVER
  switch (Setup.Rx.Diversity) {
#endif
  case DIVERSITY_DEFAULT:
    Config.UseAntenna1 = true;
    Config.UseAntenna2 = true;
    break;
  case DIVERSITY_ANTENNA1:
    Config.UseAntenna1 = true;
    Config.UseAntenna2 = false;
    break;
  case DIVERSITY_ANTENNA2:
    Config.UseAntenna1 = false;
    Config.UseAntenna2 = true;
    break;
  }
#else
  Config.UseAntenna1 = true;
  Config.UseAntenna2 = false;
#endif
}


void setup_init(void)
{
  setup_default();

  setup_sanitize();

  setup_configure();
}



#endif // SETUP_H
