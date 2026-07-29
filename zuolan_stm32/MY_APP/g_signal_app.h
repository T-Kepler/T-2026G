#ifndef G_SIGNAL_APP_H
#define G_SIGNAL_APP_H

#include "g_signal_analysis.h"

typedef enum
{
    G_SIGNAL_CONTROL_MODE_NEXT = 0,
    G_SIGNAL_CONTROL_PERIOD_TOGGLE,
    G_SIGNAL_CONTROL_VIEW_TOGGLE,
    G_SIGNAL_CONTROL_MEASURE,
    G_SIGNAL_CONTROL_RESULT_PAGE_READY,
    G_SIGNAL_CONTROL_WAVE_PAGE_READY,
    G_SIGNAL_CONTROL_CLEAR
} GSignalControl;

void GSignalApp_Init(void);
void GSignalApp_Task(void);
void GSignalApp_HandleControl(GSignalControl control);

#endif
