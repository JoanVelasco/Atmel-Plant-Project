/**************************************************************************//**
  \file app.c
  

  \brief Main code for the atmel boards controlling the irrigation plant system

  \author Joan Velasco i Estanyol, Kunil..., Nandhini...

******************************************************************************/

#include <appTimer.h>
#include <zdo.h>
#include <app.h>
#include <aps.h>
#include <sysTaskManager.h>
#include <usartManager.h>
#include <bspLeds.h>
#include <adc.h>
#include <spi.h>
#include <lcd.h>

static AppState_t appstate = APP_INIT_STATE;
static MsgState_t msgstate = MSG_START;
static uint8_t deviceType;

static ZDO_StartNetworkReq_t networkparams;
static SimpleDescriptor_t simpleDescriptor;
static APS_RegisterEndpointReq_t endPoint;
static AppMessage_t transmitData;
static struct Message messageSend;
static struct Message messageRcv;
APS_DataReq_t dataReq;

HAL_AppTimer_t readTimer;

uint8_t adcData;
uint8_t display[]="1234";
uint8_t plantType = 55;

uint8_t val;
uint8_t toSend = 0;
uint8_t manual = 0;
uint8_t sendUartArray[8];

uint8_t aux[] = "XXX";
uint8_t auxaux[] = "XXXXX";

static uint8_t ApplRxBuffer[50];
static uint8_t applRxBufferPos = 0;

/*
static HAL_SpiDescriptor_t spidescriptor= {
	.tty=SPI_CHANNEL_0,
	.clockMode=SPI_CLOCK_MODE0,
	.dataOrder=SPI_DATA_MSB_FIRST,
	.baudRate=SPI_CLOCK_RATE_62,
	.callback =displayLCDDoneCb
}; */


//ADC struct
static HAL_AdcDescriptor_t adcdescriptor = {
	.resolution = RESOLUTION_8_BIT,
	.sampleRate = ADC_4800SPS,
	.voltageReference = AVCC,
	.bufferPointer = &adcData,
	.selectionsAmount = 1,
	.callback = readSensorDoneCb
};


//Board initializer for receiving data from the other boards.
static void initEndpoint(void)
{
	simpleDescriptor.AppDeviceId = 1;
	simpleDescriptor.AppProfileId = 1;
	simpleDescriptor.endpoint = 1;
	simpleDescriptor.AppDeviceVersion = 1;
	endPoint.simpleDescriptor = &simpleDescriptor;
	endPoint.APS_DataInd = APS_DataInd;
	APS_RegisterEndpointReq(&endPoint);
}


//Transmission data initializer
static void initTransmitData(void)
{
	dataReq.profileId=1;
	dataReq.dstAddrMode = APS_EXT_ADDRESS;
	dataReq.dstAddress.extAddress = COORDINATOR_ADDRESS;
	dataReq.dstEndpoint = 1;
	dataReq.asdu = (uint8_t *) &transmitData.data;
	dataReq.asduLength = sizeof(transmitData.data);
	dataReq.srcEndpoint = 1;
	dataReq.APS_DataConf = APS_DataConf;
}

static void setupTransmitData(int64_t addr) {
	dataReq.profileId=1;
	dataReq.dstAddrMode = APS_EXT_ADDRESS;
	dataReq.dstAddress.extAddress = addr;
	dataReq.dstEndpoint = 1;
	dataReq.asdu = (uint8_t *) &transmitData.data;
	dataReq.asduLength = sizeof(transmitData.data);
	dataReq.srcEndpoint = 1;
	dataReq.APS_DataConf = APS_DataConf;
}

//Timer initializer
static void initTimer(void)
{
	readTimer.interval = 5000;
	readTimer.mode = TIMER_REPEAT_MODE;
	readTimer.callback=readSensorFired;
}


//Function called when the timer makes an interruption. This will lead the end devices to read the sensor.
static void readSensorFired(void)
{
	appstate = APP_READ_STATE;
	SYS_PostTask(APL_TASK_ID);
}


void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo){
	
	if(ZDO_SUCCESS_STATUS == confirmInfo -> status){
		CS_ReadParameter(CS_DEVICE_TYPE_ID, &deviceType);
		SYS_PostTask(APL_TASK_ID);
	}

}


//Function that is called when the transmission of data between boards through ZigBee is finished.
static void APS_DataConf(APS_DataConf_t * confInfo){
	if(confInfo ->status == APS_SUCCESS_STATUS){
		if(toSend == 1){
			sendOtherEndDevice();
		} else {
			appstate = APP_NOTHING_STATE;
			SYS_PostTask(APL_TASK_ID);
		}
	}
}


//Function that is called when the board receives a message from another board, being from end device to coordinator or vice versa.
void APS_DataInd(APS_DataInd_t *indData)
{
	//AppMessage_t *temporal = (AppMessage_t *) indData->asdu;
	messageRcv = * ((struct Message *) indData->asdu);
	//memcpy(&messageRcv, &temporal->data, sizeof(temporal->data));
	
	if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR)
	{
		prepareSendUartArray();
		appWriteDataToUsart(sendUartArray, sizeof(sendUartArray));
	}
	
	if(CS_DEVICE_TYPE == DEV_TYPE_ROUTER || CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE)
	{
		switch(messageRcv.info){
			case 'T':
				plantType = messageRcv.data[0];
				break;
			
			case 'M':
				manual = messageRcv.data[0];
				closeValve();
				break;
			
			case 'V':
				if(manual == 1) {
					if(messageRcv.data[0] == 1) {
						openValve();
					} else if(messageRcv.data[0] == 0) {
						closeValve();
					}
				}
		}
	}
}


//Function that is called when the coordinator receives data from the USART.
void rxCallbackAppl(uint16_t length)
{
	if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR)
	{
		msgstate = MSG_START;
		readUsart(length);
		if(applRxBufferPos > 6) {
			applRxBufferPos = 0;
			manageMessage();
			if(msgstate == MSG_SUCCESS){
			
				transmitData.data = messageSend;
			
				switch(messageSend.addr){
					case 'R':
						setupTransmitData(ROUTER_ADDRESS);
						break;
					
					case 'E':
						setupTransmitData(ENDDEVICE_ADDRESS);
						break;
				
					case 'B':
						if(messageSend.info == 'M'){
							manual = messageSend.data[0];
						}
						setupTransmitData(ROUTER_ADDRESS);
						toSend = 1;
						break;
				}	
				appstate = APP_TRANSMIT_STATE;
				SYS_PostTask(APL_TASK_ID);
			}
		}
	}
}

//Function that is called when the sensor is finished reading. It will check the humidity, decide if the valves have to open or close, and send the value to the coordinator
static void readSensorDoneCb(void) {
	uint32_t humidity = 0;
	humidity = (adcData*100/MAX_HUMIDUTY_SENSOR);
	
	//TODO: LCD print humidity value

	if(manual == 0){
		if (humidity <= plantType) {
			openValve();
			messageSend.data[1] = 1;
		} else {
			closeValve();
			messageSend.data[1] = 0;
		}
	}

	if(CS_DEVICE_TYPE == DEV_TYPE_ROUTER) {
		messageSend.addr = 'R';
	} else {
		messageSend.addr = 'E';
	}
	messageSend.info = 'H';
	messageSend.data[0] = humidity;
	
	memcpy(&transmitData.data, &messageSend, sizeof(messageSend));

	appstate = APP_TRANSMIT_STATE;
	SYS_PostTask(APL_TASK_ID);
}


//Function that open the valve
void openValve(void) {
	PORTE |= (1<<PE2);
	BSP_OnLed(LED_FIRST);
}


//Function that closes the valve
void closeValve(void) {
	PORTE &= ~(1<<PE2);
	BSP_OffLed(LED_FIRST);
}


//Function required when the coordinator has to send a message to both end devices. It is called when the first communication is finished.
void sendOtherEndDevice(void) {
	toSend = 0;
	transmitData.data = messageSend;
	setupTransmitData(ENDDEVICE_ADDRESS);
	
	appstate = APP_TRANSMIT_STATE;
	SYS_PostTask(APL_TASK_ID);
}

void readUsart(uint16_t length) {
	uint8_t data;
	while(length--) {
		HAL_ReadUsart(&usartDescriptor, &data, 1);
		ApplRxBuffer[applRxBufferPos] = data;
		applRxBufferPos++;
	}
}

void manageMessage(void) {
	if(ApplRxBuffer[0] == ASCII_DLE){
		if(ApplRxBuffer[1] == ASCII_STX){
			messageSend.addr = ApplRxBuffer[2];
			messageSend.info = ApplRxBuffer[3];
			int i = 4, j = 0;
			while(!(ApplRxBuffer[i] == ASCII_DLE && ApplRxBuffer[i+1] == ASCII_ETX)){
				messageSend.data[j] = ApplRxBuffer[i];
				i++;
				j++;
			}
			msgstate = MSG_SUCCESS;
		} else {
			msgstate = MSG_FAIL;
		}
	} else {
		msgstate = MSG_FAIL;
	}
}

void prepareSendUartArray(void){
	sendUartArray[0] = ASCII_DLE;
	sendUartArray[1] = ASCII_STX;
	sendUartArray[2] = messageRcv.addr;
	sendUartArray[3] = messageRcv.info;
	sendUartArray[4] = messageRcv.data[0];
	sendUartArray[5] = messageRcv.data[1];
	sendUartArray[6] = ASCII_DLE;
	sendUartArray[7] = ASCII_ETX;
}

/*static void displayLCDDoneCb(void)
{
	appWriteDataToUsart((uint8_t*)"success\r\n",sizeof("success\r\n")-1);
}*/


//TaskHandler is the function called over an over by the main's endless loop that allows the system to work with BitCloud
void APL_TaskHandler(void){
	switch (appstate){
		case APP_INIT_STATE:
			appInitUsartManager();
			initTimer();
			BSP_OpenLeds();
			
			DDRB |= (1<<PB2) | (1<<PB3) | (1<<PB1);
			HAL_OpenAdc(&adcdescriptor);
			//HAL_OpenSpi(&spidescriptor);
			DDRE |= (1<<PE2);
			
			appstate = APP_START_JOIN_NETWORK_STATE;
			SYS_PostTask(APL_TASK_ID);
			break;
			
		case APP_START_JOIN_NETWORK_STATE:
			networkparams.ZDO_StartNetworkConf = ZDO_StartNetworkConf;
			ZDO_StartNetworkReq(&networkparams);
			appstate = APP_INIT_ENDPOINT_TRANSMITDATA_STATE;
			break;
			
		case APP_INIT_ENDPOINT_TRANSMITDATA_STATE:
			initEndpoint();
			initTransmitData();
#if CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR
			appstate = APP_NOTHING_STATE;
#else
			appstate = APP_START_READ_STATE;
#endif
			SYS_PostTask(APL_TASK_ID);
			break;
			
		case APP_START_READ_STATE:
			HAL_StartAppTimer(&readTimer);
			appstate = APP_NOTHING_STATE;
			SYS_PostTask(APL_TASK_ID);
			break;
			
		case APP_READ_STATE:
			HAL_ReadAdc(&adcdescriptor, HAL_ADC_CHANNEL0);
			break;
			
		case APP_TRANSMIT_STATE:
			//HAL_WriteSpi(&spidescriptor,display,3);
			APS_DataReq(&dataReq);
			break;
			
		case APP_NOTHING_STATE:
			break;
	}
}




/*******************************************************************************
  \brief The function is called by the stack to notify the application about 
  various network-related events. See detailed description in API Reference.
  
  Mandatory function: must be present in any application.

  \param[in] nwkParams - contains notification type and additional data varying
             an event
  \return none
*******************************************************************************/
void ZDO_MgmtNwkUpdateNotf(ZDO_MgmtNwkUpdateNotf_t *nwkParams)
{
  nwkParams = nwkParams;  // Unused parameter warning prevention
}

/*******************************************************************************
  \brief The function is called by the stack when the node wakes up by timer.
  
  When the device starts after hardware reset the stack posts an application
  task (via SYS_PostTask()) once, giving control to the application, while
  upon wake up the stack only calls this indication function. So, to provide 
  control to the application on wake up, change the application state and post
  an application task via SYS_PostTask(APL_TASK_ID) from this function.

  Mandatory function: must be present in any application.
  
  \return none
*******************************************************************************/
void ZDO_WakeUpInd(void)
{
}

#ifdef _BINDING_
/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.
  
  Mandatory function: must be present in any application.

  \param[in] bindInd - information about the bound device
  \return none
 ***********************************************************************************/
void ZDO_BindIndication(ZDO_BindInd_t *bindInd)
{
  (void)bindInd;
}

/***********************************************************************************
  \brief The function is called by the stack to notify the application that a 
  binding request has been received from a remote node.

  Mandatory function: must be present in any application.
  
  \param[in] unbindInd - information about the unbound device
  \return none
 ***********************************************************************************/
void ZDO_UnbindIndication(ZDO_UnbindInd_t *unbindInd)
{
  (void)unbindInd;
}
#endif //_BINDING_

/**********************************************************************//**
  \brief The entry point of the program. This function should not be
  changed by the user without necessity and must always include an
  invocation of the SYS_SysInit() function and an infinite loop with
  SYS_RunTask() function called on each step.

  \return none
**************************************************************************/
int main(void)
{
  //Initialization of the System Environment
  SYS_SysInit();

  //The infinite loop maintaing task management
  for(;;)
  {
    //Each time this function is called, the task
    //scheduler processes the next task posted by one
    //of the BitCloud components or the application
    SYS_RunTask();
  }
}

//eof app.c