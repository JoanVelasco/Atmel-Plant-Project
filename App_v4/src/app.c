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

static ZDO_StartNetworkReq_t networkparams;
static SimpleDescriptor_t simpleDescriptor;
static APS_RegisterEndpointReq_t endPoint;
static AppMessage_t transmitData;
APS_DataReq_t dataReq;

HAL_AppTimer_t readTimer;

uint8_t adcData;
uint8_t display[]="1234";

uint8_t val;
uint8_t toSend = 0;
uint8_t manual = 0;
uint8_t aux[] = "XXX";
uint8_t auxaux[] = "XXX";

static uint8_t ApplRxBuffer[1];

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

static void initTransmitData(void)
{
	dataReq.profileId=1;
	dataReq.dstAddrMode = APS_EXT_ADDRESS;
	if((CS_DEVICE_TYPE==DEV_TYPE_ROUTER)||(CS_DEVICE_TYPE==DEV_TYPE_ENDDEVICE))
	{
		dataReq.dstAddress.extAddress = COORDINATOR_ADDRESS;
	}
	if(CS_DEVICE_TYPE==DEV_TYPE_COORDINATOR)
	{
		dataReq.dstAddress.extAddress = ENDDEVICE_ADDRESS;
	}
	dataReq.dstEndpoint = 1;
	dataReq.asdu = transmitData.data;
	dataReq.asduLength = sizeof(transmitData.data);
	dataReq.srcEndpoint = 1;
	dataReq.APS_DataConf = APS_DataConf;
}


static void initTimer(void)
{
	readTimer.interval = 2000;
	readTimer.mode = TIMER_REPEAT_MODE;
	readTimer.callback=readSensorFired;
}


static void readSensorFired(void)
{
	appstate = APP_READ_STATE;
	SYS_PostTask(APL_TASK_ID);
}


void ZDO_StartNetworkConf(ZDO_StartNetworkConf_t *confirmInfo){
	
	if(ZDO_SUCCESS_STATUS == confirmInfo -> status){
		CS_ReadParameter(CS_DEVICE_TYPE_ID,&deviceType);
		if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR){
			//appWriteDataToUsart((uint8_t*)"Coordinator\r\n",sizeof("Coordinator\r\n")-1);
		}
		else if(CS_DEVICE_TYPE == DEV_TYPE_ROUTER){
			//appWriteDataToUsart((uint8_t*)"Router\r\n",sizeof("Router\r\n")-1);
		}
		else if(CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE){
			//appWriteDataToUsart((uint8_t*)"Enddevice\r\n",sizeof("Enddevice\r\n")-1);
		}
		SYS_PostTask(APL_TASK_ID);
	}
	
	
	else{
		//transmitData.data[10]='E';
		//transmitData.data[11]='r';
		//transmitData.data[12]='r';
		//transmitData.data[13]='o';
		//transmitData.data[14]='r';
		
	}
	
}

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

void APS_DataInd(APS_DataInd_t *indData)
{
	if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR)
	{
		appWriteDataToUsart(indData->asdu,indData->asduLength);
		appWriteDataToUsart((uint8_t*)"\r\n",2);
	}
	
	if(CS_DEVICE_TYPE == DEV_TYPE_ROUTER)
	{
		
		if(indData->asdu[0]=='1')
		{
			//appWriteDataToUsart((uint8_t*)"Port1opened\r\n",sizeof("Port1opened\r\n")-1);
			openValve();
		}
		else if(indData->asdu[0]=='2')
		{
			//appWriteDataToUsart((uint8_t*)"Port1closed\r\n",sizeof("Port1closed\r\n")-1);
			closeValve();
		}
		else if(indData->asdu[0]=='5')
		{
			manual = 1;
			closeValve();
		}
		else if(indData->asdu[0]=='6')
		{
			manual = 0;
		}
		
	}
	if (CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE)
	{
		//appWriteDataToUsart((uint8_t*)"success2\r\n",sizeof("success2\r\n")-1);
		if(indData->asdu[0]=='3')
		{
			//appWriteDataToUsart((uint8_t*)"Port2opened\r\n",sizeof("Port1opened\r\n")-1);
			openValve();
			
		}
		else if(indData->asdu[0]=='4')
		{
			//appWriteDataToUsart((uint8_t*)"Port2closed\r\n",sizeof("Port2closed\r\n")-1);
			closeValve();
			
		}
		else if(indData->asdu[0]=='5')
		{
			manual = 1;
			closeValve();
		}
		else if(indData->asdu[0]=='6')
		{
			manual = 0;
		}
	}
}


void rxCallbackAppl(uint16_t length)
{
	
	//BSP_ToggleLed(LED_FIRST);
	if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR)
	{
		//appWriteDataToUsart((uint8_t*)"success\r\n",sizeof("success\r\n")-1);
		HAL_ReadUsart(&usartDescriptor,ApplRxBuffer,length);
		//appWriteDataToUsart(ApplRxBuffer,sizeof(ApplRxBuffer));
		
		transmitData.data[0]=ApplRxBuffer[0];
		val = transmitData.data[0];
		if(val=='5') {
			manual = 1;
		} else if (val =='6') {
			manual = 0;
		}
		
		if((val=='5')||(val=='6')) {
			dataReq.profileId=1;
			dataReq.dstAddrMode = APS_EXT_ADDRESS;
			dataReq.dstAddress.extAddress = ROUTER_ADDRESS;
			dataReq.dstEndpoint = 1;
			dataReq.asdu = transmitData.data;
			dataReq.asduLength = sizeof(transmitData.data);
			dataReq.srcEndpoint = 1;
			dataReq.APS_DataConf = APS_DataConf;
			toSend = 1;
			appstate = APP_TRANSMIT_STATE;
			SYS_PostTask(APL_TASK_ID);
		}
		
		if(manual == 1) {
			//review why the whole initialization is needed
			if((val=='1')||(val=='2'))
			{
				dataReq.profileId=1;
				dataReq.dstAddrMode = APS_EXT_ADDRESS;
				dataReq.dstAddress.extAddress = ROUTER_ADDRESS;
				dataReq.dstEndpoint = 1;
				dataReq.asdu = transmitData.data;
				dataReq.asduLength = sizeof(transmitData.data);
				dataReq.srcEndpoint = 1;
				dataReq.APS_DataConf = APS_DataConf;
			}
		
			else if((val=='3')||(val=='4'))
			{
				dataReq.profileId=1;
				dataReq.dstAddrMode = APS_EXT_ADDRESS;
				dataReq.dstAddress.extAddress = ENDDEVICE_ADDRESS;
				dataReq.dstEndpoint = 1;
				dataReq.asdu = transmitData.data;
				dataReq.asduLength = sizeof(transmitData.data);
				dataReq.srcEndpoint = 1;
				dataReq.APS_DataConf = APS_DataConf;	
			}
			appstate = APP_TRANSMIT_STATE;
			SYS_PostTask(APL_TASK_ID);
		}	
		
	}
	
}


static void readSensorDoneCb(void) {
	
	uint8_t max = 146;
	uint32_t humidity = 0;

	humidity = (adcData*100/max);
	
	//TODO: LCD print humidity value

	if (humidity <= 55)
	{
		uint32_to_str(auxaux, sizeof(auxaux), manual, 0, 1);
		appWriteDataToUsart(auxaux, sizeof(auxaux));
		if(manual == 0){
			openValve();
			transmitData.data[5]='1';
		}
		
	}
	else
	{
		uint32_to_str(auxaux, sizeof(auxaux), manual, 0, 1);
		appWriteDataToUsart(auxaux, sizeof(auxaux));
		if(manual == 0){
			closeValve();
			transmitData.data[5]='0';
		}
	}
		
	uint32_to_str(aux, sizeof(aux), humidity, 0, 3);
	
	transmitData.data[1]=aux[0];
	transmitData.data[2]=aux[1];
	transmitData.data[3]=aux[2];
	
	if (CS_DEVICE_TYPE == DEV_TYPE_ROUTER) {
		transmitData.data[4]='B';
	} else if (CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE) {
		transmitData.data[4]='D';
	}
	appstate = APP_TRANSMIT_STATE;
	SYS_PostTask(APL_TASK_ID);
}

void openValve(void) {
	PORTE |= (1<<PE1);
	BSP_OnLed(LED_FIRST);
}

void closeValve(void) {
	PORTE &= ~(1<<PE1);
	BSP_OffLed(LED_FIRST);
}

void sendOtherEndDevice(void) {
	toSend = 0;
	transmitData.data[0] = val;
	dataReq.profileId=1;
	dataReq.dstAddrMode = APS_EXT_ADDRESS;
	dataReq.dstAddress.extAddress = ENDDEVICE_ADDRESS;
	dataReq.dstEndpoint = 1;
	dataReq.asdu = transmitData.data;
	dataReq.asduLength = sizeof(transmitData.data);
	dataReq.srcEndpoint = 1;
	dataReq.APS_DataConf = APS_DataConf;

	appstate = APP_TRANSMIT_STATE;
	SYS_PostTask(APL_TASK_ID);
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
			
			DDRB |= (1<<PB2) | (1<<PB3) | (1<<PB1);
			HAL_OpenAdc(&adcdescriptor);
			//HAL_OpenSpi(&spidescriptor);
			DDRE |= (1<<PE1);
			
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
#if CS_DEVICE_TYPE == DEV_TYPE_ROUTER
			transmitData.data[0]='A';
#elif CS_DEVICE_TYPE == DEV_TYPE_ENDDEVICE
			transmitData.data[0]='C';
#endif
			//HAL_WriteSpi(&spidescriptor,display,3);
			APS_DataReq(&dataReq);
/*#if CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR
			if(toSend == 1){
				sendOtherEndDevice();
			}
#endif*/
			//appstate = APP_NOTHING_STATE;
			//SYS_PostTask(APL_TASK_ID);
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