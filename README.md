# TrafficSim: Real-Time Traffic Network & Telemetry Simulation

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.x-black?logo=unrealengine)
![C++](https://img.shields.io/badge/C++-17-00599C?logo=c%2B%2B)
![Python](https://img.shields.io/badge/Python-3.x-3776AB?logo=python)
![Flask](https://img.shields.io/badge/Flask-Microservice-black?logo=flask)

**TrafficSim** is a high-performance, agent-based traffic simulation built in Unreal Engine 5 (C++) featuring a decoupled, real-time Python/Flask telemetry dashboard. 

Designed for urban planners and data scientists, it allows users to dynamically generate road networks, trigger localized traffic surges, and monitor grid congestion via a live REST API microservice.

---

## Core Features

* **A* Agent-Based Routing:** Vehicles autonomously navigate complex procedural grids, adapting to roadblocks and capacity limits in real-time.
* **Procedural Network Editing:** In-game Build/Delete modes allow for dynamic modification of intersections and road segments on the fly.
* **Dynamic State Management:** Complex UI layer interacting with C++ actor states to manage spawner nodes, destination linking, and active congestion events (Rush Hour / Stadium events).
* **Decoupled Telemetry Microservice:** Unreal Engine broadcasts live JSON simulation states via HTTP POST requests to an external lightweight Python backend.
* **Command Center Dashboard:** A responsive, dark-mode web application (HTML5/Chart.js) visualizing active vehicles, grid congestion limits, and average travel times in real-time.
* **Automated Data Logging:** The backend automatically generates chronological, timestamped `.csv` logs for post-simulation data science analysis.

---

## Technical Architecture

TrafficSim separates the heavy graphical and mathematical simulation from data visualization to ensure maximum CPU efficiency.

1.  **The Engine (UE5 / C++):** Handles procedural generation, collision matrices, A* pathing algorithms, and UI state logic.
2.  **The Network Layer (HTTP):** UE5 gathers actor statistics every 500ms and fires lightweight JSON payloads asynchronously.
3.  **The Backend (Python / Flask):** Catches telemetry payloads, sanitizes the data, appends to the CSV log file, and serves the REST API.
4.  **The Frontend (JS / Chart.js):** Pings the Flask API and animates the live data streams.

---

## Getting Started

### Prerequisites
* Windows 10/11
* Python 3.x (with `flask` installed)
* Unreal Engine 5 (If building from source)

### Running the Simulation (Pitch Mode)
For the pre-packaged presentation build, we have automated the server and client deployment.

1. Navigate to the game directory.
2. Double-click `Launch_Pitch.bat`. 
   *(This will silently initialize the Flask server in the background and launch the Unreal Engine simulation).*
3. Inside the simulation UI, click **Launch Dashboard** to open the Command Center at `http://localhost:5000`.
4. After closing the simulation, check the project directory for your timestamped log files.

---

## Development & Manual Testing

If you are running the project from inside the Unreal Editor:
1. Run `Start_Dashboard.bat` to spin up the Python listener.
2. Open the project in Unreal Engine.
3. Press **Play**. The server will automatically begin catching JSON payloads as soon as traffic is spawned.

---
*Built for the 2026 Hackathon.*
