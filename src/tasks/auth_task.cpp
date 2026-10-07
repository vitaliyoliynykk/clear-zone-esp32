#include <Arduino.h>
#include <string>
#include "tasks.h"
#include "task_context.h"
#include "http_manager.h"
#include "ble_manager.h"

namespace
{
    volatile bool authInProgress = false;
}

bool isAuthInProgress()
{
    return authInProgress;
}

bool requestAuth()
{
    if (authInProgress)
    {
        return false;
    }

    uint8_t msg = 1;
    if (xQueueSend(authQueue, &msg, 0) != pdTRUE)
    {
        return false;
    }

    authInProgress = true;
    return true;
}

void AuthTask(void *param)
{
    uint8_t msg;
    for (;;)
    {
        if (xQueueReceive(authQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            bool isRegistered = handleAuthInitialization();
            sendAuthResult(isRegistered);
            authInProgress = false;

            if (isRegistered)
            {
                resetRetriesAfterAuthSuccess();
            }
        }
    }
}
