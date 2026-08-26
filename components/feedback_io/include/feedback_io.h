/*
 * SPDX-FileCopyrightText: 2026 Aliro HomeKey contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "app_config.h"

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Status LEDs and RTTTL buzzer, driven off the same access_control
 * event stream MQTT and Matter reporting already subscribe to.
 *
 * The status LED mirrors the current lock state. The denied LED is pulsed for
 * a configured time when a tap is refused -- the state LED cannot say that,
 * since it is dark for a refusal and dark for no tap alike. The buzzer plays a
 * configurable RTTTL tune on a granted or denied tap. All three are optional
 * and off unless a GPIO is configured.
 *
 * @param[in] cfg Feedback configuration from the running app_config
 */
esp_err_t feedback_io_start(const feedback_config_t *cfg);

#ifdef __cplusplus
}
#endif
