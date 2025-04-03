/*******************************************************************************
 *   @file   freeRTOS/freertos_semaphore.c
 *   @brief  Implementation of no-OS semaphore funtionality.
 *   @author Robert Budai (robert.budai@analog.com)
********************************************************************************
 * Copyright 2023(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <FreeRTOS.h>
#include "no_os_semaphore.h"
#include "semphr.h"

/**
 * @brief Initialize semaphore.
 * semaphore - Pointer toward the semaphore.
 */
__attribute__((weak)) inline void no_os_semaphore_init(void **semaphore)
{
	if (*semaphore == NULL) {
		SemaphoreHandle_t* semaphoreTmp = (SemaphoreHandle_t *)no_os_calloc(1,
						  sizeof(SemaphoreHandle_t));

		*semaphore = xSemaphoreCreate();
		xSemaphoreGive(*semaphore);
	}
}

/**
 * @brief Take token from semaphore.
 * semaphore - Pointer toward the semaphore.
 */
__attribute__((weak)) inline void no_os_semaphore_take(void *semaphore)
{
	if (semaphore != NULL)
		xSemaphoreTake((SemaphoreHandle_t)semaphore, portMAX_DELAY);
}

/**
 * @brief Give token to semaphore
 * semaphore - Pointer toward the semaphore.
 */
__attribute((weak)) inline void no_os_semaphore_give(void *semaphore)
{
	if (semaphore != NULL)
		xSemaphoreGive((SemaphoreHandle_t)semaphore);
}

/**
 * @brief Remove semaphore.
 * semaphore - Pointer toward the semaphore.
 */
__attribute__((weak)) inline void no_os_semaphore_remove(void *semaphore)
{
	if (semaphore != NULL) {
		vSemaphoreDelete((SemaphoreHandle_t)semaphore);
	}
}
