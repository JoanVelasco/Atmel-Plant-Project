/**************************************************************************//**
  \file app.c
  

  \brief Main code for the atmel boards controlling the irrigation plant system

  \author Joan Velasco i Estanyol, Kunil..., Nandhini...
  
  \In the LCD:	1:Router
				2:End Device

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

//command seq of internally powered
uint8_t Command[50]={0xAE,0xD5,0x80,0xA8,0x1F,0xD3,0x00,0x40,0x8D,0x14,0xA1,0xC8,0xDA,0x02,0x81,0x82,0xD9,0xF1,0xDB,0x30,0xA4,0xA6,0xAF};
//command sequence for standard display info
uint8_t ConstantDisplay[32]={0x20,0x00,0x21,0x00,0xF1,0x22,0x00,0x00,0x21,0x38,0xF1,0x21,0x00,0xF1,0x22,0x03,0x03,0x21,0x38,0xF1,0x22,0x00,0x00,0x21,0x18,0x38,0x22,0x03,0x03,0x21,0x18,0x38};
//pixel values for 0 to 9 and %
uint8_t A0[8]={0x00,0xFF,0x81,0x81,0x81,0x81,0xFF,0x00};
uint8_t A1[8]={0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00};
uint8_t A2[8]={0x00,0xF1,0x91,0x91,0x91,0x91,0x9F,0x00};
uint8_t A3[8]={0x00,0x91,0x91,0x91,0x91,0x91,0xFF,0x00};
uint8_t A4[8]={0x00,0x1F,0x10,0x10,0xFC,0x10,0x10,0x00};
uint8_t A5[8]={0x00,0x9F,0x91,0x91,0x91,0x91,0xF1,0x00};
uint8_t A6[8]={0x00,0xFF,0x91,0x91,0x91,0x91,0xF1,0x00};
uint8_t A7[8]={0x00,0x01,0x01,0x01,0x01,0x01,0xFF,0x00};
uint8_t A8[8]={0x00,0xFF,0x91,0x91,0x91,0x91,0xFF,0x00};
uint8_t A9[8]={0x00,0x8F,0x91,0x91,0x91,0x91,0xFF,0x00};
uint8_t APer[8]={0x00,0x40,0x26,0x16,0x08,0x64,0x62,0x00};
uint8_t Ab[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

uint8_t digits[4];
uint8_t h=0;
uint8_t s=0;
uint8_t a=32;
uint8_t pos=0;
uint8_t b;


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

void SPI_MasterInit(void){
	/* Enable SPI, Master, set clock rate fck/8 */
	appWriteDataToUsart((uint8_t*)"SPIINIT\r\n",sizeof("SPIINIT\r\n")-1);
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	SPSR &= ~(1<<SPI2X);
}


void SPI_MasterTransmit(uint8_t byte){
	appWriteDataToUsart((uint8_t*)&byte,1);
	/* Start transmission */
	SPDR = byte;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));
}
uint8_t SeparationOfDigits(uint8_t humid)
{
	uint8_t number;
	
	number =humid%10;
	
	return number;
}
void PrintingDigits(uint8_t digit)
{
	switch(digit)
	{
		case 0:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A0[r++]);
		}
		break;
		case 1:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A1[r++]);
		}
		break;
		case 2:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A2[r++]);
		}
		break;
		
		case 3:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A3[r++]);
		}
		break;
		case 4:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A4[r++]);
		}
		break;
		case 5:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A5[r++]);
		}
		break;
		case 6:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A6[r++]);
		}
		break;
		case 7:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A7[r++]);
		}
		break;
		case 8:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A8[r++]);
		}
		break;
		case 9:
		for(int r=0;r<8;)
		{
			SPI_MasterTransmit(A9[r++]);
		}
		break;
		
	}
}

void StandardDisplayInfo()
{
	pos=0;
	PORTE &= ~(1<<PE2);
	for(pos=0;pos<8;)
	{
		SPI_MasterTransmit(ConstantDisplay[pos++]);
	}
	
	if(pos==8)
	{
		//data
		PORTE |= (1<<PE2);
	}
	
	//plant 1
	for(int j=0;j<8;)
	{
		SPI_MasterTransmit(A1[j++]);
	}
	PORTE &= ~(1<<PE2);
	
	for(pos=8;pos<11;)
	{
		SPI_MasterTransmit(ConstantDisplay[pos++]);
	}
	
	PORTE |= (1<<PE2);
	for(int n=0;n<8;)
	{
		SPI_MasterTransmit(APer[n++]);
	}
	
	
	PORTE &= ~(1<<PE2);
	for(pos=11;pos<17;)
	{
		SPI_MasterTransmit(ConstantDisplay[pos++]);
	}
	
	//data
	PORTE |= (1<<PE2);
	for(int l=0;l<8;)
	{
		SPI_MasterTransmit(A2[l++]);
	}
	
	PORTE &= ~(1<<PE2);
	for(pos=17;pos<20;)
	{
		SPI_MasterTransmit(ConstantDisplay[pos++]);
	}

	PORTE |= (1<<PE2);
	for(int r=0;r<8;)
	{
		SPI_MasterTransmit(APer[r++]);
	}
	
}


void UpdateHumidityData(uint8_t device, uint8_t data)
{
	uint8_t Sensor;
	Sensor=device;
	PORTE &= ~(1<<PE2);
	if(Sensor=='R')
	{
		for(pos=20;pos<26;)
		{
			SPI_MasterTransmit(ConstantDisplay[pos++]);
		}
	}
	else if (Sensor=='E')
	{
		for(pos=20;pos<26;)
		{
			if(pos==21)
			{
				SPI_MasterTransmit(0x03);
				pos++;
			}
			SPI_MasterTransmit(ConstantDisplay[pos++]);
		}
	}
	
	for(h=3;h>0;)
	{
		digits[h--]=SeparationOfDigits(data);
		data=data/10;
	}
	PORTE |= (1<<PE2);
	for(s=1;s<=3;)
	{
		PrintingDigits(digits[s++]);
	}
	//the array should point 20 location again when it is coming back.
	pos=20;
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
	messageRcv = * ((struct Message *) indData->asdu);
	
	if(CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR)
	{
		UpdateHumidityData(messageRcv.addr, messageRcv.data[0]);
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
	PORTE |= (1<<PE1);
	BSP_OnLed(LED_FIRST);
}


//Function that closes the valve
void closeValve(void) {
	PORTE &= ~(1<<PE1);
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


//TaskHandler is the function called over an over by the main's endless loop that allows the system to work with BitCloud
void APL_TaskHandler(void){
	switch (appstate){
		case APP_INIT_STATE:
			appInitUsartManager();
			initTimer();
			BSP_OpenLeds();
			
			DDRB |= (1<<PB2) | (1<<PB3) | (1<<PB1);
			HAL_OpenAdc(&adcdescriptor);
			DDRE |= (1<<PE1);
#if CS_DEVICE_TYPE == DEV_TYPE_COORDINATOR
			//MOSI and SCK as Output
			DDRB |= (1<<PB2);
			DDRB |= (1<<PB1);
			//MISO as Input
			DDRB &= ~(1<<PB3);
			
			//to set clock mode and freq
			SPI_MasterInit();
			//Chip select as output
			DDRG |=(1<<PG0);
			//chip select low
			PORTG &= ~(1<<PG0);

			//Data/Command pin as output
			DDRE |=(1<<PE2);
			//command indication
			PORTE &= ~(1<<PE2);
			//RES
			DDRD |= (1<<PD5);
			
			
			//RES as low for power stablisation sequence
			PORTD &= ~(1<<PD5);
			//RES as high for operation
			PORTD |= (1<<PD5);
			//command
			PORTE &= ~(1<<PE2);
			//initialization sequence
			for(pos=0;pos<25;)
			{
				SPI_MasterTransmit(Command[pos++]);
			}
			PORTE |= (1<<PE2);
			StandardDisplayInfo();
#endif
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