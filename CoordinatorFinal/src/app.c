/**************************************************************************//**
  \file app.c
  

  \brief Basis-Anwendung.

  \author Markus Krauße

******************************************************************************/

#include <appTimer.h>
#include <zdo.h>
#include <app.h>
#include <sysTaskManager.h>
#include <usartManager.h>
#include <bspLeds.h>
#include <adc.h>
#include <spi.h>
#include <lcd.h>

static AppState_t appstate = APP_INIT_STATE;
static uint8_t deviceType;
static void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo);
static ZDO_StartNetworkReq_t networkparams;
static SimpleDescriptor_t simpleDescriptor;
static APS_RegisterEndpointReq_t endPoint;
static HAL_UsartDescriptor_t usartDesc;
static void initEndpoint(void);
void APS_DataInd(APS_DataInd_t *indData);
HAL_AppTimer_t recieveTimerLed;
HAL_AppTimer_t transmitTimerLed;
HAL_AppTimer_t transmitTimer;
uint8_t adcData;
uint8_t display[]="1234";

static void readSensorDoneCb(void);
static void displayLCDDoneCb(void);

static void recieveTimerLedFired(void);
static void transmitTimerFired(void);
static void initTimer(void);
uint8_t temp[] = "The Resistance:XXXXXXXXXXX";
static uint8_t ApplRxBuffer[1];

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


/*static void initUsart(void)
{
	usartDesc.tty = USART_CHANNEL_1;
	usartDesc.mode = USART_MODE_ASYNC;
	usartDesc.baudrate =USART_BAUDRATE_38400;
	usartDesc.parity = USART_PARITY_NONE;
	usartDesc.dataLength = USART_DATA8;
	usartDesc.stopbits = USART_STOPBIT_1;
	usartDesc.flowControl = USART_FLOW_CONTROL_NONE;
	usartDesc.rxBuffer = usartRxBuffer;
	usartDesc.rxBufferLength = sizeof(usartRxBuffer);
	usartDesc.rxCallback = rxCallbackAppl;
	usartDesc.txBuffer = NULL;
	usartDesc.txBufferLength = 0;
	usartDesc.txCallback = NULL;
	
}*/

void APS_DataInd(APS_DataInd_t *indData)
{
	HAL_StartAppTimer(&recieveTimerLed);
	
	appWriteDataToUsart(indData->asdu,indData->asduLength);
	appWriteDataToUsart((uint8_t*)"\r\n",2);
}





static void initTimer(void)
{
	/*transmitTimerLed.interval=5000;
	transmitTimerLed.mode = TIMER_REPEAT_MODE;
	transmitTimerLed.callback = Waterlevelstatusindicator;*/


	recieveTimerLed.interval = 500;
	recieveTimerLed.mode = TIMER_ONE_SHOT_MODE;
	recieveTimerLed.callback=recieveTimerLedFired;

	transmitTimer.interval = 2000;
	transmitTimer.mode = TIMER_REPEAT_MODE;
	transmitTimer.callback=transmitTimerFired;
}

static void recieveTimerLedFired(void)
{

}

 void rxCallbackAppl(uint16_t length)
{
	if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR)
	{
	appWriteDataToUsart((uint8_t*)"success\r\n",sizeof("success\r\n")-1);
		HAL_ReadUsart(&usartDescriptor,ApplRxBuffer,length);
		appWriteDataToUsart(ApplRxBuffer,sizeof(ApplRxBuffer));
		}
		else
		{
		}
		
	
}
static void transmitTimerFired(void)
{
	appstate = APP_TRANSMIT_STATE;
	SYS_PostTask(APL_TASK_ID);
}

BEGIN_PACK
typedef struct _AppMessage_t {
	uint8_t header[APS_ASDU_OFFSET];
	uint8_t data[15];
	uint8_t footer[APS_AFFIX_LENGTH - APS_ASDU_OFFSET];
}PACK AppMessage_t;
END_PACK

static AppMessage_t transmitData;
APS_DataReq_t dataReq;
static void APS_DataConf(APS_DataConf_t *confInfo);
static void initTransmitData(void);


static void initTransmitData(void)
{
	dataReq.profileId=1;
	dataReq.dstAddrMode = APS_SHORT_ADDRESS;
	dataReq.dstAddress.shortAddress = CPU_TO_LE16(0x0000);
	dataReq.dstEndpoint = 1;
	dataReq.asdu = transmitData.data;
	dataReq.asduLength = sizeof(transmitData.data);
	dataReq.srcEndpoint = 1;
	dataReq.APS_DataConf = APS_DataConf;
}

static void APS_DataConf(APS_DataConf_t * confInfo){
	if(confInfo ->status == APS_SUCCESS_STATUS){
		HAL_StartAppTimer(&transmitTimerLed);
		appstate = APP_NOTHING_STATE;
		SYS_PostTask(APL_TASK_ID);
	}
}

void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo){
	
	if(ZDO_SUCCESS_STATUS == confirmInfo -> status){
		CS_ReadParameter(CS_DEVICE_TYPE_ID,&deviceType);
		if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR){
			appWriteDataToUsart((uint8_t*)"Coordinator\r\n",sizeof("Coordinator\r\n")-1);
		}
		else if(CS_DEVICE_TYPE == DEV_TYPE_ROUTER){
			appWriteDataToUsart((uint8_t*)"Router\r\n",sizeof("Router\r\n")-1);
		}
		else if(CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE){
			appWriteDataToUsart((uint8_t*)"Enddevice\r\n",sizeof("Enddevice\r\n")-1);
		}
		SYS_PostTask(APL_TASK_ID);
	}
	
	
		else{	
				transmitData.data[10]='E';
				transmitData.data[11]='r';
				transmitData.data[12]='r';
				transmitData.data[13]='o';
				transmitData.data[14]='r';
				
		}
	
}
/*
static HAL_SpiDescriptor_t spidescriptor= {
	.tty=SPI_CHANNEL_0,
	.clockMode=SPI_CLOCK_MODE0,
	.dataOrder=SPI_DATA_MSB_FIRST,
	.baudRate=SPI_CLOCK_RATE_62,
	.callback =displayLCDDoneCb
}; */

static HAL_AdcDescriptor_t adcdescriptor = {
	.resolution = RESOLUTION_8_BIT,
	.sampleRate = ADC_4800SPS,
	.voltageReference = AVCC,
	.bufferPointer = &adcData,
	.selectionsAmount = 1,
	.callback = readSensorDoneCb
};

static void readSensorDoneCb(void) {
	
	uint8_t max = 146;
	uint32_t humidity = 0;
	humidity = (adcData*100/max);
if (humidity <= 55)
{
	PORTE |= (1<<PE3);
	if (CS_DEVICE_TYPE == DEV_TYPE_ROUTER)
	{
	transmitData.data[4]='B';
	transmitData.data[5]='1';
	}
	
	if (CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE)
		{
			transmitData.data[4]='D';
			transmitData.data[5]='1';
		}
		
}
else
{
	PORTE &= ~(1<<PE3);
		if (CS_DEVICE_TYPE == DEV_TYPE_ROUTER)
		{
			transmitData.data[5]='0';
		}
		
		if (CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE)
		{

			transmitData.data[5]='0';
		}
}
	uint32_to_str(temp, sizeof(temp), humidity, 15, 8);
	
	
	transmitData.data[1]=temp[20];
	transmitData.data[2]=temp[21];
	transmitData.data[3]=temp[22];

	
}

/*static void displayLCDDoneCb(void)
{
	appWriteDataToUsart((uint8_t*)"success\r\n",sizeof("success\r\n")-1);
}*/

void APL_TaskHandler(void){
	switch (appstate){
		case APP_INIT_STATE:
			appInitUsartManager();
			initTimer();
			BSP_OpenLeds();
			//initUsart();
			//HAL_OpenUsart(&usartDesc);
			DDRB |= (1<<PB2) | (1<<PB3) | (1<<PB1);
			HAL_OpenAdc(&adcdescriptor);
			//HAL_OpenSpi(&spidescriptor);
			DDRE |= (1<<PE3);
			appstate = APP_START_JOIN_NETWORK_STATE;
			SYS_PostTask(APL_TASK_ID);
			break;
		case APP_START_JOIN_NETWORK_STATE:
			networkparams.ZDO_StartNetworkConf = ZDO_StartNetworkConf;
			ZDO_StartNetworkReq(&networkparams);
			appstate = APP_INIT_ENDPOINT_STATE;
			break;
		case APP_INIT_ENDPOINT_STATE:
			initEndpoint();
			#if CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR
			appstate = APP_NOTHING_STATE;
			#else
			appstate = APP_INIT_TRANSMITDATA_STATE;
			#endif
			SYS_PostTask(APL_TASK_ID);
			break;
		case APP_INIT_TRANSMITDATA_STATE:
			initTransmitData();
			appstate = APP_NOTHING_STATE;
			HAL_StartAppTimer(&transmitTimer);
			SYS_PostTask(APL_TASK_ID);
			break;
		case APP_TRANSMIT_STATE:
#if (CS_DEVICE_TYPE == DEV_TYPE_ROUTER)
HAL_ReadAdc(&adcdescriptor, HAL_ADC_CHANNEL0);
transmitData.data[0]='A';
			
#else if CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
HAL_ReadAdc(&adcdescriptor, HAL_ADC_CHANNEL1);
transmitData.data[0]='C';
		
#endif
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
