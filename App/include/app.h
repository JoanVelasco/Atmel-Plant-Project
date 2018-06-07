/**************************************************************************//**
  \file app.h

  \brief Usart-Anwendung Headerdatei.

  \author
    Markus Krauﬂe

******************************************************************************/

#ifndef _APP_H
#define _APP_H

#define APP_SENDE_INTERVAL    1000

typedef enum{
	APP_INIT_STATE,
	APP_AUSGABE_STATE,
	APP_NOTHING_STATE
} AppState_t;
#endif
// eof app.h