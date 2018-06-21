/**************************************************************************//**
  \file app.h

  \brief Usart-Anwendung Headerdatei.

  \author
    Markus Krauﬂe

******************************************************************************/

#ifndef _APP_H
#define _APP_H

#define APP_SENDE_INTERVAL    1000

static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo);
static void initEndpoint(void);
void APS_DataInd(APS_DataInd_t *indData);

static void readSensorDoneCb(void);
//static void displayLCDDoneCb(void);

static void readSensorFired(void);
static void initTimer(void);

static void APS_DataConf(APS_DataConf_t *confInfo);
static void initTransmitData(void);

void openValve(void);
void closeValve(void);
void sendOtherEndDevice(void);

typedef enum{
	APP_INIT_STATE,
	APP_START_JOIN_NETWORK_STATE,
	APP_INIT_ENDPOINT_TRANSMITDATA_STATE,
	APP_START_READ_STATE,
	APP_READ_STATE,
	APP_TRANSMIT_STATE,
	APP_NOTHING_STATE
} AppState_t;

//Structure of message for the ZigBee communication
BEGIN_PACK
typedef struct _AppMessage_t {
	uint8_t header[APS_ASDU_OFFSET]; //APS header
	uint8_t data[6];
	uint8_t footer[APS_AFFIX_LENGTH - APS_ASDU_OFFSET]; //footer
}PACK AppMessage_t;
END_PACK

#endif
// eof app.h