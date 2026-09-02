/*
 * SPDX-FileCopyrightText: 2026 Aliro HomeKey contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <esp_err.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration UI and REST API.
 *
 * Routes, trimmed from HomeKey-ESP32's set to what this project actually has:
 *
 *   GET  /                 the UI
 *   GET  /api/status       chip, uptime, heap, network, reader state
 *   GET  /api/hardware     which pins this chip allows, for the pin pickers
 *   GET  /api/config       running configuration, passwords masked
 *   POST /api/config       validate and persist a configuration
 *   POST /api/config/reset erase the stored configuration
 *   POST /api/reboot       restart, so a new configuration takes effect
 *   POST /api/unlock       drive the lock output now
 *
 * Deliberately absent, unlike the project this borrows from: HomeKit pairing,
 * Ethernet, NeoPixel, OTA upload, certificate management and HTTPS.
 */

/** @brief Reported through /api/status so the UI can show the reader state. */
typedef struct {
    size_t (*credential_count)(void);
    const char *(*transport_name)(void);
    bool (*lock_is_locked)(void);
    bool (*mqtt_enabled)(void);
    bool (*mqtt_connected)(void);
    esp_err_t (*unlock)(void);
} web_server_hooks_t;

/** @brief Event observer callback for web server events (config changes, lock events, etc.) */
typedef void (*web_server_event_observer_t)(const char *event_type, const void *data);

esp_err_t web_server_start(const web_server_hooks_t *hooks);

esp_err_t web_server_stop(void);

/**
 * @brief Start the web server once Matter has finished bringing its fabrics up.
 *
 * Boot is a memory peak: every fabric resumes its subscription at once, because
 * CHIP fires them together by design (see connectedhomeip issue 25439). Holding
 * the web server's ~16 kB through that is what pushes a multi-fabric lock into
 * "PacketBuffer: pool EMPTY" and leaves controllers showing it as Updating.
 *
 * Only call this when Matter is actually running. A board with no Wi-Fi
 * credentials never starts Matter, and there the web server is the setup
 * portal -- deferring it would leave that board with no way in at all.
 *
 * Bounded on both sides: web_server_note_stack_ready() shortens the wait, and
 * an absolute deadline starts the server even if that never arrives.
 *
 * @param hooks Web server hooks, as for web_server_start()
 * @param after Optional. Invoked once the deferred start fires, for other
 *              network services that should wait for the same moment. Called
 *              even if the server itself failed to start, so nothing queued
 *              behind it is held hostage by an httpd that could not bind.
 */
esp_err_t web_server_start_deferred(const web_server_hooks_t *hooks, void (*after)(void));

/**
 * @brief Tell a deferred start that the Matter stack is up.
 *
 * Shortens the wait to a settle window rather than starting immediately: the
 * stack being ready to talk to other nodes is not the same as having finished
 * doing so. Harmless when nothing is deferred.
 */
void web_server_note_stack_ready(void);

/**
 * @brief Give the web server's RAM back while a controller commissions us.
 *
 * Commissioning is this firmware's peak memory moment, and on a classic ESP32
 * there is not much left to peak into -- a reader in the field had 18 KB free
 * while pairing, and pairing failed partway (issue #13). Nobody is using the
 * configuration UI while they are pairing from their phone, so it stands down
 * and comes back afterwards.
 *
 * Always paired. The caller signals both edges, and the server also resumes on
 * its own if the end signal never arrives: a lock nobody can configure is a
 * worse outcome than a commissioning attempt short of memory.
 *
 * @param active true while commissioning is under way, false once it ends
 *               however it ends
 */
void web_server_set_commissioning_active(bool active);

/** @brief Register an event observer to receive notifications of config changes and lock events. */
/**
 * @brief Watch events the web server publishes.
 *
 * @c data is a @c cJSON object. It is typed void here so that the public
 * header does not drag cJSON into every consumer.
 */
typedef void (*web_server_event_cb_t)(const char *event_type, const void *data);

void web_server_register_event_observer(web_server_event_cb_t observer);

#ifdef __cplusplus
}
#endif
