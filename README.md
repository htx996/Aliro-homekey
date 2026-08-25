<div align="center">

<img src="https://ruhanpaco.github.io/Aliro-homekey/assets/aliro-mark.svg" width="120" alt="Aliro HomeKey logo">

# Aliro HomeKey

### Open-source Aliro access reader for ESP32

[![Status](https://img.shields.io/badge/status-v0.5%20beta-ff79c6?style=for-the-badge)](https://github.com/Ruhanpaco/Aliro-homekey/releases)
[![ESP32](https://img.shields.io/badge/ESP32-supported-8be9fd?style=for-the-badge)](https://www.espressif.com/en/products/socs/esp32)
[![Aliro](https://img.shields.io/badge/Aliro-supported-50fa7b?style=for-the-badge)](docs/ARCHITECTURE.md)
[![Matter](https://img.shields.io/badge/Matter-supported-8be9fd?style=for-the-badge)](docs/ARCHITECTURE.md)
[![License](https://img.shields.io/badge/license-Apache--2.0-f1fa8c?style=for-the-badge)](LICENSE)

**One reader. One standard. Multiple wallet ecosystems.**

[Website](https://ruhanpaco.github.io/Aliro-homekey) · [Browser Flasher](https://ruhanpaco.github.io/Aliro-homekey) · [Architecture](docs/ARCHITECTURE.md) · [Roadmap](docs/ROADMAP.md) · [Releases](https://github.com/Ruhanpaco/Aliro-homekey/releases)

</div>

---

## What is Aliro HomeKey?

An ESP32 firmware that turns the board into an experimental **Aliro access reader**. A phone or wearable presents an Aliro credential, the reader runs the transaction, and the ESP32 drives an access-control output — relay, strike, or test LED.

It's built on **Aliro**, the Connectivity Standards Alliance standard for interoperable mobile access credentials, so the reader isn't locked to one wallet vendor.

```text
Apple / Google / Samsung Wallet ──▶ Aliro protocol ──▶ ESP32 Reader ──▶ Lock / Relay
                                                       (NFC · Matter · MQTT)
```

> **Build the reader once. Let the wallet ecosystem handle the credential experience.**

## Current status

> **v0.5 beta:** a phone opens the door on tested hardware, and the surrounding product layer works alongside it.

Verified on an **ESP32-WROOM-32 with a PN532**: commissioned into Apple Home, provisioned with an Aliro credential over Matter, and driven through both transaction paths — **569 ms** on the fast path, **2047 ms** on the standard path. Home app lock/unlock, OTA with rollback, the configuration UI, MQTT, and Home Assistant discovery have all been exercised on the same board.

`main` also carries fixes not yet in a tagged release: the Apple ECP beacon defaults back off after it was found to wedge the NFC bus, Wi-Fi power save is disabled once connected (it was silently timing out Matter subscriptions), and a duplicate-notification bug in Matter lock reporting is fixed.

This remains a **research/beta project, not a certified access-control product**. Apple shows an uncertified-accessory warning during commissioning. Google and Samsung wallet flows are untested, and Apple Wallet Express Mode still needs confirmation on the physical test lock.

## Get firmware onto a board

Four ways in — pick whichever fits.

<details open>
<summary><strong>Build from source</strong> — full control, requires the ESP-IDF toolchain</summary>

Install **ESP-IDF 5.2–6.0**, with `idf.py` and `openssl` on your PATH.

```bash
idf.py set-target esp32
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;boards/sdkconfig.defaults.esp32" build
idf.py -p /dev/ttyUSB0 flash monitor
```

The first build generates a development reader identity under `main/certs/` (gitignored — see [`main/certs/README.md`](main/certs/README.md) before using it for anything but development).

</details>

<details>
<summary><strong>Flash a released build</strong> — no toolchain, one file, esptool only</summary>

Grab the matching pair from [Releases](https://github.com/Ruhanpaco/Aliro-homekey/releases) for your target (`esp32`, `esp32c3`, or `esp32s3`):

```bash
# Wipes the board and installs everything: bootloader, partitions, app
python -m esptool --chip esp32 write_flash 0x0 esp32.firmware.factory.bin

# Or, over the device's own OTA page once it's already running:
# open http://<device-ip>/#/ota and upload esp32.firmware.bin
```

</details>

<details>
<summary><strong>Browser flasher</strong> — no toolchain, no CLI, generates a unique identity in-tab</summary>

<div align="center">

### [Open the Aliro HomeKey Browser Flasher →](https://ruhanpaco.github.io/Aliro-homekey)

**WebSerial · Browser Crypto · No backend**

</div>

Generates a unique P-256 reader identity locally in the browser and writes it over WebSerial. Private keys never leave the tab. Needs desktop Chrome, Edge, or Opera.

</details>

<details>
<summary><strong>Matter build</strong> — same firmware, plus a Matter Door Lock endpoint</summary>

Optional, off by default. Lets a controller (Apple Home, Google Home, Home Assistant) provision the reader identity and credentials directly — the NFC transaction itself doesn't change.

```bash
docker run --rm -it -v "$PWD:/work" -w /work \
  espressif/esp-matter:latest_idf_v5.5.4 bash -lc \
  '. "$IDF_PATH/export.sh" && . "$ESP_MATTER_PATH/export.sh" && \
   idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.matter" \
   set-target esp32 build'
```

GitHub Actions runs the same build via the `matter-firmware` workflow and publishes factory + OTA images to [Releases](https://github.com/Ruhanpaco/Aliro-homekey/releases).

</details>

## How a tap works

Aliro transactions run over **ISO 7816 APDUs on ISO 14443-4 NFC** (this project's current focus; BLE/UWB are part of the broader spec but not yet implemented here).

1. Reader detects the device and selects the Aliro applet.
2. An expedited P-256 handshake authenticates both sides.
3. The device identifies its credential via an Aliro key slot.
4. The reader checks that key slot against local access data.
5. Transaction completes, access output fires.

A previously-seen credential can use the **fast path**, skipping most of the handshake.

## Stack

| Layer | Technology |
| --- | --- |
| Access protocol | **Aliro** (via Espressif's ESP-Aliro SDK) |
| Firmware | **ESP-IDF**, on the **ESP32** family |
| NFC | **PN532** / compatible frontends |
| Smart home | **Matter** |
| Messaging | **MQTT** + **Home Assistant** discovery |

The ESP-Aliro SDK handles the protocol, crypto, and transaction state. Aliro HomeKey builds everything around it: NFC transport, credentials, access decisions, lock control, config, OTA, MQTT, Matter, and the device UI.

## Board support

<details open>
<summary><strong>ESP32-WROOM-32</strong> — ✅ hardware-tested</summary>

The primary development and transaction-testing platform. Everything under [Current status](#current-status) was verified on this board.

</details>

<details>
<summary><strong>ESP32-C3 / ESP32-S3</strong> — builds and releases, not yet hardware-verified</summary>

CI builds and publishes firmware for both on every release, and both pass the OTA-slot-fit check. Neither has run on real hardware here yet — pin maps exist in [`boards/`](boards/) and `components/app_config/Kconfig`, but treat them as unverified until someone confirms a tap actually opens a lock.

</details>

<details>
<summary><strong>ESP32-C6 / ESP32-H2 / ESP32-P4</strong> — SDK-supported, no board work done</summary>

The Espressif Aliro SDK targets these chips, so extending Aliro HomeKey to them is possible, but no pin defaults, partition tuning, or testing exists here yet.

</details>

> Board-level validation ≠ SDK target support. A target above may still need its own pin map, partitioning, wiring, and hardware validation before it's fully supported.

Typical hardware: an **ESP32-WROOM-32** (or other supported target), a **PN532** or compatible NFC frontend, a relay/strike/LED for the access output, and a USB connection for flashing.

## Features

- **Access** — Aliro transactions, fast + standard paths, key-slot access decisions, lock/relay GPIO output
- **Device** — browser config UI, Wi-Fi setup AP, NVS-backed config, OTA with rollback, GPIO validation
- **Integrations** — Matter Door Lock endpoint, Aliro provisioning over Matter, MQTT, Home Assistant discovery
- **Developer tools** — ESP-IDF based, browser flasher, serial diagnostics, per-board defaults, architecture docs

## Configuration

On first boot without Wi-Fi credentials, the reader raises its own setup network:

```text
Aliro-Setup-XXXX
password: aliro1234
```

Open `http://192.168.4.1/`. Config lives in NVS, so changing wiring or network settings never needs a rebuild — NFC bus, GPIO assignments, lock polarity, Wi-Fi, MQTT, and system settings are all in there.

MQTT is optional and off by default. Enabled, it publishes lock state and tap events, accepts unlock commands, and announces itself to Home Assistant. Full API in [`docs/WEB.md`](docs/WEB.md).

## Repository layout

```text
Aliro-homekey/
├── site/                    # GitHub Pages: browser flasher + project website
├── main/                    # Firmware entry point (+ certs/: generated dev identity)
├── components/
│   ├── app_config/          # Runtime configuration + NVS
│   ├── nfc_transport/       # NFC hardware abstraction
│   ├── aliro_reader/        # Aliro transaction integration
│   ├── access_control/      # Credentials + access decisions + lock output
│   ├── net_manager/         # Wi-Fi + setup AP
│   ├── mqtt_manager/        # MQTT + Home Assistant integration
│   ├── matter_lock/         # Matter Door Lock + Aliro provisioning
│   └── web_server/          # REST API + embedded configuration UI
├── boards/                  # Board-specific ESP-IDF defaults
├── tools/                   # Development and firmware tooling
└── docs/                    # Architecture, API and roadmap
```

## Documentation

| Document | Purpose |
| --- | --- |
| [`ARCHITECTURE.md`](docs/ARCHITECTURE.md) | System boundaries, components and data flow |
| [`WEB.md`](docs/WEB.md) | Configuration service, REST API and web UI |
| [`FIRST-TEST.md`](docs/FIRST-TEST.md) | Hardware flashing and healthy boot procedure |
| [`ROADMAP.md`](docs/ROADMAP.md) | Current milestones, validation work and known gaps |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | Contribution workflow and project rules |

## Contributing

NFC drivers, board support, wallet testing, protocol research, docs, and bug fixes are all useful. Read [`CONTRIBUTING.md`](CONTRIBUTING.md) and check the roadmap before opening a PR.

## Contributors

- [**grapefizz**](https://github.com/grapefizz) — restored Apple Wallet Express Mode polling ([#1](https://github.com/Ruhanpaco/Aliro-homekey/pull/1), [#3](https://github.com/Ruhanpaco/Aliro-homekey/pull/3)), fixed Apple Home status resuming after a restart ([#4](https://github.com/Ruhanpaco/Aliro-homekey/pull/4)), and added Matter lock activity event publishing ([#5](https://github.com/Ruhanpaco/Aliro-homekey/pull/5)).

## Important disclaimer

Aliro HomeKey is a **research and hobbyist implementation**, not certified for production access control.

Aliro is a trademark of the Connectivity Standards Alliance. This project is not affiliated with or endorsed by the CSA, Espressif, Apple, Google, or Samsung.

A shipping access-control product requires the appropriate ecosystem provisioning, certification, security review, and hardware validation.

## License

Released under the **Apache License 2.0**. See [`LICENSE`](LICENSE).

<div align="center">

**Aliro HomeKey**

`ESP32` · `Aliro` · `ESP-IDF` · `Matter` · `MQTT` · `Open Source`

[Back to top](#aliro-homekey)

</div>
