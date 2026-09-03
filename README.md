# Smart Infusion Pump

An embedded infusion pump prototype designed for real-time flow monitoring, closed-loop flow control, local status display, and remote telemetry using Wi-Fi and MQTT.

> **Project Status:** Prototype / Simulation-oriented engineering project
> **Note:** This project is intended for educational and engineering portfolio purposes and is **not a clinically validated or certified medical device**.

---

## Overview

The **Smart Infusion Pump** is an embedded system prototype built around an **ESP32** microcontroller.

The system combines sensor-based monitoring, closed-loop control, local user interaction, safety-related alarm logic, and MQTT-based remote telemetry into a single embedded platform.

The main objective is to maintain a desired infusion flow rate while continuously monitoring relevant system parameters and providing local and remote status information.

---

## System Architecture

The overall system architecture is shown below.

![System Architecture](Images/system_architecture.png)

The **ESP32** acts as the central controller, coordinating:

* Sensor acquisition and processing
* PID-based flow control
* System state management
* Safety and alarm logic
* Local user interface
* MQTT-based telemetry

---

## Key Features

* Real-time infusion flow monitoring
* Closed-loop flow-rate control using a **PID controller**
* Flow signal smoothing using a **5-sample moving average**
* Fluid weight monitoring using a **Load Cell + HX711**
* Pump current monitoring using an **INA219**
* Local status display using an **SSD1306 OLED**
* Push-button user interaction
* Audible and visual alarm indication
* State-machine-based system operation
* Wi-Fi connectivity
* MQTT-based remote telemetry
* Target-volume completion detection
* Basic abnormal-condition detection and alarm handling

---

## Hardware Components

| Component           | Purpose                                    |
| ------------------- | ------------------------------------------ |
| ESP32 DevKitC       | Main microcontroller and system controller |
| YF-S201 Flow Sensor | Infusion flow measurement                  |
| Load Cell           | Fluid weight measurement                   |
| HX711               | Load-cell signal interface                 |
| INA219              | Pump current monitoring                    |
| SSD1306 OLED        | Local system status display                |
| Push Buttons        | User input and system control              |
| Buzzer              | Audible alarm indication                   |
| LED                 | Visual status/alarm indication             |
| Infusion Pump       | Controlled fluid delivery                  |

---

## Control System

The infusion flow is controlled using a **PID controller** implemented on the ESP32.

The control loop follows the general signal path:

```text
Flow Sensor
     ↓
Pulse Measurement
     ↓
Flow Rate Calculation
     ↓
5-Sample Moving Average
     ↓
PID Controller
     ↓
PWM Output
     ↓
Infusion Pump
```

The PID controller is updated periodically and uses the measured flow rate as feedback to adjust the pump PWM output.

The implementation also includes integral windup limiting and constrains the PWM output to the available range.

---

## Software Architecture

The firmware is implemented as a single Arduino `.ino` application and is organized around dedicated functions for the main system tasks.

Major software functions include:

* `setup()` — system initialization
* `loop()` — main execution loop
* `readSensors()` — sensor acquisition
* `readFlow()` — flow measurement and processing
* `runPID()` — closed-loop control
* `handleButtons()` — user input handling
* `handleStateButtons()` — state-related user interaction
* `checkSafety()` — abnormal-condition monitoring
* `runPriming()` — priming operation
* `updateDisplay()` — OLED interface updates
* `sendMQTT()` — telemetry transmission

---

## System State Machine

The firmware uses a state-machine structure to organize the main operating modes:

```text
        ┌─────────┐
        │  IDLE   │
        └────┬────┘
             │
             ▼
       ┌───────────┐
       │  PRIMING  │
       └─────┬─────┘
             │
             ▼
       ┌───────────┐
       │ INFUSION  │
       └─────┬─────┘
             │
       ┌─────┴──────┐
       │            │
       ▼            ▼
┌────────────┐  ┌─────────┐
│  COMPLETE  │  │  ALARM  │
└────────────┘  └─────────┘
```

This structure separates normal operation, priming, completion, and abnormal conditions into clearly defined states.

---

## Monitoring and Alarm Logic

The system continuously monitors multiple parameters during operation.

Examples of implemented conditions include:

### Overcurrent / Low Flow

An abnormal condition is detected when the monitored pump current exceeds the configured threshold while the measured flow remains below its corresponding threshold.

### Low Weight

A low measured fluid weight can trigger the alarm state.

### Target Volume

When the accumulated infusion volume reaches the configured target volume, the system transitions to the `COMPLETE` state.

These mechanisms represent **prototype-level monitoring and control logic** and should not be interpreted as clinically validated safety mechanisms.

---

## IoT Communication

The ESP32 uses **Wi-Fi** and **MQTT** for remote telemetry.

The system publishes monitored information through MQTT topics such as:

```text
infusion/flow
infusion/volume
infusion/current
infusion/weight
```

Telemetry is transmitted periodically to allow external monitoring of the system.

The project implements **remote telemetry/monitoring**, not remote clinical control.

---

## Circuit Design

The circuit was designed using **Cirkit Designer**.

![Circuit Design](Images/circuit_design.png)

**Original interactive circuit design:**

[View the project on Cirkit Designer](https://app.cirkitdesigner.com/project/dd6367ed-e05c-49b9-9265-42930e5ee764)

---

## Repository Structure

```text
Smart-Infusion-Pump/
│
├── Arduino_Code/
│   └── Smart_Infusion_Pump.ino
│
├── Images/
│   ├── system_architecture.png
│   └── circuit_design.png
│
├── README.md
└── LICENSE
```

---

## Technologies Used

* **ESP32**
* **Arduino Framework**
* **C/C++**
* **PID Control**
* **PWM**
* **Interrupt-based pulse measurement**
* **Moving Average Filtering**
* **Wi-Fi**
* **MQTT**
* **Embedded State Machine**
* **Sensor Integration**

---

## Engineering Highlights

This project demonstrates the integration of several embedded-systems concepts within a single healthcare-oriented prototype:

* Sensor acquisition and signal processing
* Closed-loop control
* Embedded state-machine design
* Actuator control using PWM
* Real-time monitoring
* Local human-machine interaction
* IoT communication
* Basic fault and alarm handling
* Integration of multiple hardware interfaces

---

## Limitations

This project is an **engineering prototype** and has not undergone clinical validation, medical-device certification, or formal safety verification.

The implemented thresholds and control logic are intended for the project's prototype environment and should not be used as clinical operating parameters.

The MQTT implementation provides telemetry and monitoring functionality rather than remote clinical control.

---

## Future Improvements

Potential future improvements include:

* More robust fault handling and recovery mechanisms
* Improved communication reconnection handling
* More comprehensive system validation
* Additional monitoring and diagnostic capabilities
* Hardware-level safety mechanisms
* Formal testing under different infusion conditions
* Development of a dedicated monitoring dashboard

---

## License

This project is licensed under the **MIT License**.

See the [LICENSE](LICENSE) file for details.

