/**************************************************************************//**
  \file app.h

  \brief Usart-Anwendung Headerdatei.

  \author
    Markus Krauﬂe

******************************************************************************/

#ifndef _APP_H
#define _APP_H

#define APP_SENDE_INTERVAL    1000
#define MAX_HUMIDUTY_SENSOR	  200
#define ASCII_DLE			  16
#define ASCII_STX			  2
#define ASCII_ETX			  3

static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo);
static void initEndpoint(void);
void APS_DataInd(APS_DataInd_t *indData);

static void readSensorDoneCb(void);
//static void displayLCDDoneCb(void);

static void readSensorFired(void);
static void initTimer(void);

static void APS_DataConf(APS_DataConf_t *confInfo);
static void initTransmitData(void);
static void setupTransmitData(int64_t addr);

void openValve(void);
void closeValve(void);
void sendOtherEndDevice(void);
void prepareSendUartArray(void);
void manageMessage(void);
void readUsart(uint16_t length);

//LCD function delcaration
void SPI_MasterInit(void);
void SPI_MasterTransmit(uint8_t byte);
uint8_t SeparationOfDigits(uint8_t humid);
void PrintingDigits(uint8_t digit);
void StandardDisplayInfo();
void UpdateHumidityData(uint8_t device, uint8_t data);


typedef enum{
	APP_INIT_STATE,
	APP_START_JOIN_NETWORK_STATE,
	APP_INIT_ENDPOINT_TRANSMITDATA_STATE,
	APP_START_READ_STATE,
	APP_READ_STATE,
	APP_TRANSMIT_STATE,
	APP_NOTHING_STATE
} AppState_t;

typedef enum{
	MSG_START,
	MSG_SUCCESS,
	MSG_FAIL
} MsgState_t;

typedef struct Message {
	uint8_t addr;
	uint8_t info;
	uint8_t data[4];
}Message_t;

//Structure of message for the ZigBee communication
BEGIN_PACK
typedef struct _AppMessage_t {
	uint8_t header[APS_ASDU_OFFSET]; //APS header
	struct Message data;
	uint8_t footer[APS_AFFIX_LENGTH - APS_ASDU_OFFSET]; //footer
}PACK AppMessage_t;
END_PACK

#endif
// eof app.h