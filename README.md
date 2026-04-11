# TrafficSim: Traffic Resilience & Analytics Engine

![Unreal Engine](https://img.shields.io/badge/Unreal_Engine-5.0+-black?style=for-the-badge&logo=unrealengine)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)

## Overview
TrafficSim is a decentralized, agent-based traffic simulation engine built entirely in C++ for Unreal Engine. Designed as a stress-testing tool for urban planners and civil engineers, it allows users to procedurally generate city infrastructure, simulate high-density traffic surges, trigger infrastructure failures, and export real-time analytics to CSV for external analysis.

## Core Features

* **Procedural Infrastructure Generation:** Instantly generate interconnected grid cities of varying scales (up to 20x20 blocks) with randomized wobble and organic dead-ends using custom C++ algorithms.
* **Decentralized Agent AI (A* Pathfinding):** Vehicles do not follow pre-baked spline tracks. Every agent autonomously calculates its own optimal route to its destination using a custom implementation of the A* algorithm, dynamically rerouting in real-time if paths become congested or blocked.
* **Interactive Stress Testing (Scenarios):**
  * **Road Closures:** Simulate catastrophic artery collapses by blocking specific road segments, forcing the AI network to organically find detours.
  * **Traffic Surges:** Trigger localized capacity surges (e.g., Stadium Events) with queued, time-gated spawners.
* **Real-Time Analytics & Heatmaps:** Roads dynamically update a custom Shader pipeline to display accessibility heatmaps based on real-time congestion ratios. 
* **Professional CAD Controls:** Built-in RTS Drone Camera featuring edge-scrolling, smooth zoom, and dual-axis orbiting, designed specifically for professional engineering workflows.
* **Data Export:** Aggregate simulation metrics (Average Travel Time, Active Agents, Completed Trips) directly to formatted `.csv` files at runtime.

## Technical Architecture

The backend is driven by highly optimized C++ systems to handle hundreds of autonomous agents without frame drops:

* **`UTrafficNetworkSubsystem`:** A custom Unreal Subsystem that builds a directed graph of the city layout at runtime. It handles all A* pathfinding requests asynchronously.
* **`UTrafficSpawnerComponent`:** A modular actor component that handles localized agent queuing, ensuring vehicles do not clip into each other during massive crowd events. It utilizes time-dilation-aware timers for perfectly paced spawning.
* **Physics-Culled Agents:** To maximize CPU performance, vehicle agents (`ATrafficVehicle`) do not use the heavy Unreal physics engine. They are driven by custom spline-interpolation math, calculating distance, following gaps, and emergency vehicle yielding entirely through pure C++ logic.

## How to Use (Simulation Controls)

1. **Boot Sequence:** Upon launching, the built-in interactive tutorial will guide you through the UI and controls.
2. **Camera:** Use `WASD` or mouse edge-scrolling to pan. Hold `Right-Click` to orbit and tilt the camera. Use the `Scroll Wheel` to zoom.
3. **Control Panel:** Press `M` to open the master UI to generate cities, populate traffic, trigger disaster scenarios, and export your data.
4. **Advanced Intersection Control:**
   * **`Shift + Left Click`:** Trigger a localized Rush Hour surge at that specific intersection.
   * **`Alt + Left Click`:** Disable the traffic lights at an intersection to create a seamless highway bypass.
   * **`Right Click`:** Open the specific routing overlay for that node.

## Installation & Building
1. Clone this repository.
2. Right-click the `.uproject` file and select **Generate Visual Studio project files**.
3. Open the `.sln` file and build the project in **Development Editor** configuration.
4. Launch the Unreal Editor and press Play.
