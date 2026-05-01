# TrafficSim: Agent-Based Traffic Resilience Testing

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.x-black?logo=unrealengine)
![C++](https://img.shields.io/badge/C++-17-00599C?logo=c%2B%2B)
![Python](https://img.shields.io/badge/Python-3.x-3776AB?logo=python)
![Flask](https://img.shields.io/badge/Flask-Microservice-black?logo=flask)

**TrafficSim** is a high-performance, agent-based traffic simulation built in Unreal Engine 5 (C++) featuring a decoupled, real-time Python/Flask telemetry dashboard. 

Built for the **TinkerQuest 26 Hackathon**, this tool moves beyond static urban planning models. It allows users to dynamically generate road networks, trigger real-world disruptions (like artery collapses or roadblocks), and evaluate infrastructure resilience via a live REST API microservice.

---

## Core Features

* **A* Agent-Based Routing:** Vehicles are modeled as autonomous agents that navigate complex procedural grids, generating decentralized, emergent traffic patterns.
* **Interactive Resilience Testing:** Planners can dynamically modify infrastructure in real-time by placing roadblocks, disabling traffic lights, or simulating massive traffic surges to evaluate network stress.
* **Dynamic Environment & Smart Lighting:** Native UE5 SkyAtmosphere integration with an interactive time-of-day slider. Features procedural smart streetlights that trigger only when the sun dips below the horizon.
* **Cinematic 'God Mode' Camera:** A custom algorithmic drone camera calculates the mathematical centroid of the procedural city to execute flawless 360-degree presentation orbits.
* **Decoupled Telemetry Microservice:** Unreal Engine broadcasts live JSON simulation states via HTTP POST requests to an external, lightweight Python backend.
* **Interpretable Outputs:** A responsive web application visualizes active vehicles, throughput vs. load, congestion heatmaps, and average travel times in real-time.

---
## How We Addressed the Problem Statement

TrafficSim was purpose-built to solve **Problem Statement 1: Agent-Based Traffic Resilience Testing**. Here is how our architecture directly maps to the core requirements:

*   **Structured Spatial Networks:** We implemented procedural grid generation and A* graph-based pathfinding, allowing planners to create complex road networks on the fly.
*   **Autonomous Agents & Speed Dynamics:** Instead of scripted paths, up to 5,000 vehicles operate as independent agents. Each calculates its own gap-distance, lane offsets, and speed based on the environment (DOD architecture). 
*   **Emergent Decentralized Patterns:** Traffic jams occur naturally in our simulation. Furthermore, civilian agents autonomously detect approaching emergency vehicles via array lookups and dynamically swerve to the curb to create a bypass lane.
*   **Environmental Factors & Signal Timing:** Planners can use the interactive Control Panel to simulate accidents (stray dynamic obstacles), force Artery Closures, trigger Stadium Surges, or completely override global **Traffic Signal Timings** to observe how the grid recovers.
*   **Interpretable Outputs:** The decoupled Python/Flask dashboard provides live metrics requested by the prompt, including exact Average Travel Times, Congestion Percentages, and dynamic visual heatmaps on the UE5 roads.

## Technical Architecture

TrafficSim completely bypasses traditional Object-Oriented engine bottlenecks to achieve massive scale (5,000+ agents at 60 FPS).

1.  **The Engine (UE5 C++ / DOD):** Built using pure **Data-Oriented Design (DOD)** and a custom **Struct of Arrays (SoA)** memory pool. We replaced standard physics and collision volumes with pure 1D array math (1 clock cycle per vehicle).
2.  **GPU Batching & Rendering:** Uses Hardware Instanced Static Meshes (HISM) and distance culling. The C++ logic writes thousands of vehicle transforms to a raw memory buffer and hands it to the GPU in **one single batch update per frame**.
3.  **The Backend (Python / Flask):** A decoupled REST API catches telemetry payloads asynchronously, logs them to `.csv`, and serves the live dashboard without impacting the UE5 render thread.

## Getting Started

> **Note:** The executable contains a fully interactive, in-engine tutorial to guide users through generating their first city and operating the simulation.

### Prerequisites
* Windows 10/11
* Python 3.x (with `flask` installed)
* Unreal Engine 5 (If building from source)

### Running the Simulation (Pitch Mode)
For the pre-packaged presentation build, we have automated the server and client deployment.

1. Navigate to the game directory.
2. Double-click `Launch_Pitch.bat`. 
   *(This will silently initialize the Flask server in the background and launch the Unreal Engine simulation).*
3. Inside the simulation UI, click **Launch Live Dashboard** to open the Command Center at `http://localhost:5000`.
4. After closing the simulation, check the project directory for your timestamped `.csv` log files.

---

## Development & Manual Testing

If you are running the project from inside the Unreal Editor:
1. Run `Start_Dashboard.bat` to spin up the Python listener.
2. Open the project in Unreal Engine.
3. Press **Play**. The server will automatically begin catching JSON payloads as soon as traffic is populated.

---
*Developed for TinkerQuest 26.*
