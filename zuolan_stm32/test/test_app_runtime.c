/* 应用层周期任务板级示例。 */
#include "app_pid.h"
#include "key_app.h"
#include "scheduler.h"
#include "g_signal_app.h"

void example_app_init(void)
{
    PID_Init();
    GSignalApp_Init();
    scheduler_init();
}

void example_app_poll(void)
{
    /* 放在主循环中；调度器会按任务表运行按键和AD9959状态任务。 */
    scheduler_run();
}

void example_app_manual_tasks(void)
{
    /* 仅在不使用scheduler_run()时手动周期调用。 */
    key_proc();
    Pid_Proc();
    GSignalApp_Task();
    GSignalApp_HandleControl(G_SIGNAL_CONTROL_MEASURE);
}
