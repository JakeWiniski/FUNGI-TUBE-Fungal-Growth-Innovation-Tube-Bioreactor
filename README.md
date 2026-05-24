# FUNGI-TUBE: Fungal-Growth-Innovation-Tube-Bioreactor
FUNGI-TUBE (Fungal Growth & Innovation Tube Bioreactor) is a modular solid-state bioreactor system that integrates passive aeration and low-cost, non-invasive sensing to monitor and optimize fungal growth and metabolism across diverse applications—including mycelium materials, mushroom cultivation, and solid-state fermentation. Designed to maximize data richness relative to unit cost, the system supports large experimental arrays (high n) with a deep feature space, enabling time-resolved insight into both physical growth and metabolic activity. By coupling structural, optical, thermal, and gas-phase monitoring, FUNGI-TUBE functions as a low-cost “budget biofoundry” for adaptive learning, trait screening, and myco-centric R&D.

## FRAME — *Fungal-Response-And-Mycelium-Evolution Module*

The **FRAME** is the core unit of the system: a 50 mL conical centrifuge tube holder modified to support **non-invasive monitoring** of growth dynamics in solid-state fermentation. This sensorized tube holder houses the fungal culture in a standard conical 50mL centrifuge tube and enables quantitative tracking of growth development over time.

Each **FRAME** module integrates:

- **Capacitance sensor**  
  Detects changes in the dielectric properties of the solid substrate, providing a proxy signal for **mycelial growth, substrate colonization, and moisture dynamics**. The capacitance sensor in the FUNGI-TUBE system provides a non-invasive, quantitative proxy for fungal growth by detecting changes in the electrical properties of the substrate. As fungal mycelium colonizes the substrate, it alters the local dielectric environment through the accumulation of biomass, extracellular matrix, and moisture redistribution. These biological and physical changes effect the overall capacitance measured by the sensor, which is positioned externally along the substrate tube. This method allows for continuous, non-invasive, and low-cost tracking of mycelial expansion. Capacitance data, when analyzed alongside other sensor outputs such as CO₂, VOCs, and temperature, offers a powerful window into both structural growth dynamics and underlying metabolic activity.

- **RGB color sensor**  
  Measures the **relative brightness, pigmentation, and density** of surface mycelium. Useful for monitoring visual colonization density, morphological transitions, and the onset of **secondary metabolite** production (e.g., pigments).

- **Temperature sensor**  
  Logs **tube surface temperature**, enabling detection of **metabolic heat generation** and aiding in the interpretation of growth phase and respiration kinetics. Additional integrated external sensors also capture environment temperature. 

These sensors are mounted externally and aligned with the body of the tube, enabling continuous data acquisition without disturbing the culture. Sensor data is logged locally (or can be transmitted via ESP-NOW), supporting high-resolution time series analysis of fungal growth evolution.

When paired with the **FOGM** (Fungal Off-Gas Monitor) module, the FUNGI-TUBE system offers a low-cost, modular platform for studying fungal physiology, growth kinetics, and environmental response in real time.

## FOGM — *Fungal Off-Gas Monitor Module*

**FOGM** is a modular sensor chamber designed to monitor the gaseous byproducts of fungal growth in small-scale, solid-state fermentation systems — specifically the 50 mL **FUNGI-TUBE** bioreactor platform.

This detachable cap integrates multiple environmental sensors to passively sample the headspace gas diffusing from the growth tube, providing real-time data on:

- **CO₂ concentration** 
- **Relative humidity (RH)**  
- **Volatile organic compounds (VOCs)**  

The FOGM chamber features a tortuous internal gas path with inlet and outlet ports that encourage mixing, prevent direct gas escape, and stabilize readings. It is designed to be 3D printable and easy to assemble.

FOGM enables detailed monitoring of fungal metabolic activity and environmental response in micro-cultivation experiments. Paired with the FRAME this allows for hollistic co-featurization of mycelium and metabolic evolution. 

## FUNGI-TUBE Shield & Firmware

The FUNGI-TUBE Shield is a purpose-built expansion board designed to support operation with the ESP32 microcontroller, providing both the core functionality and flexible optionality needed for diverse fungal cultivation and monitoring scenarios. By building on the ESP32 platform, the system gains integrated Wi-Fi and Bluetooth connectivity, opening pathways for wireless data transmission, real-time monitoring, and remote experiment management. This optional connectivity allows the same hardware to operate as a completely self-contained logger for offline experiments or as part of a networked, cloud-connected array for more complex studies. The shield itself integrates a TCA9548A I²C multiplexer for scalable sensor interfacing, 4.7kΩ pull-up resistors, an ADS1115 analog-to-digital converter, and a logic level shifter to enable robust communication across both 3.3V and 5V devices. Dedicated headers for multiple I²C peripherals (e.g., SCD41, SGP40, SHT31, TCS34725), a microSD interface for local data logging, and an onboard LED for visual signaling. 

The FUNGI-TUBE firmware powers the environmental sensing platform, coordinating data collection from multiple I²C sensors via the onboard multiplexer and logging all measurements to an SD card in timestamped CSV format. Sensors monitored include CO₂ (SCD41), VOCs (SGP40), temperature and humidity (SHT31), infrared temperature (MLX90614), RGB light (TCS34725), and capacitance (via ADS1115). The firmware uses a real-time clock (RTC) to synchronize timestamps across units, supports fault-tolerant operation with retry logic and append-only logging, and uses an LED indicator for visible status signaling during idle periods. It is designed for reliable, low-maintenance deployment and can be extended to support features such as ESP-NOW wireless communication or external control modules.

## Data Processing & Featurization Pipeline

The FUNGI-TUBE system generates multi-modal time series data capturing substrate development and metabolic activity. A standardized Python processing pipeline transforms this raw data into a curated set of biologically interpretable features.

### Core Processing:

- Time alignment and signal smoothing  
- Temperature-detrending of substrate capacitance to isolate biological signals  
- Relative change calculations for CO₂, VOCs, RH, and temperature  
- RGB color analysis for surface morphology and pigmentation tracking  
- Stabilized biological ratios and composite growth/metabolic indices  
- Gas-phase concentration and flux estimation using FOGM geometry  

### Feature Set & Applications:

The resulting features provide time-resolved, interpretable insight into:

- Substrate colonization and structural change  
- Fungal respiration, VOC output, and water loss  
- Metabolic efficiency, stress response, and growth transitions   

The pipeline is provided as a reproducible Python script and Jupyter Notebook for easy application to FUNGI-TUBE datasets.

## System Overview & Application Potential

Together, the **FUNGI-TUBE** is a low-cost, scalable, and modular platform for **deep characterization of fungal growth** in solid-state culture. This system is designed to deliver rich, time-resolved data on both **internal substrate development** and **external metabolic effluent**, enabling detailed and holistic insight into fungal behavior.

By combining:

- **Substrate capacitance (growth kinetics proxy)**
- **RGB optical sensing (surface morphology and pigmentation)**
- **Localized temperature (metabolic activity)**
- **CO₂, VOC, and RH (respiration signature)**

the system provides **multi-modal, non-invasive, and non-destructive monitoring** of fungal growth across many parallel experiments.

### Applications

- **High-throughput screening** of fungal strains, substrates, additives, or environmental conditions
- **Quantitative trait measurement** for fungal R&D and bioprocess optimization
- **Data-rich input for machine learning models**, enabling predictive and adaptive control of fungal cultivation
- **Accessible prototyping tool** for researchers, educators, and startups working in **mycelium materials**, **functional mushrooms**, or **solid-state bioprocessing**
- **Off-Line Testing (Quality assurance and anomaly detection)**, by supporting functional growth forecasting models that enable early identification of deviations in substrate or spawn lot performance
- **Deployable**, by being easily assembled inexpensively using off-the-shelf components and 3D-printed parts, making it suitable for use in academic or private labs (particularly labs with cost sensitivities), mushroom farms, or distributed networks. Each unit fits within standard bench-top incubators, allowing for temperature-controlled studies without the need for specialized equipment.

This integrated monitoring framework supports the **generation of structured, multi-dimensional datasets** that can be used to train deep learning models, identify patterns in fungal behavior, and accelerate innovation in **mycelium and mushroom technologies**. Its modularity and low cost make it ideal for both small-scale lab environments and larger, automated screening arrays.

## License

The FUNGI-TUBE project is licensed under the CERN Open Hardware License v2 – Strongly Reciprocal (CERN-OHL-S-2.0).  
You are free to use, modify, and distribute the design and derivatives, provided attribution is given and derivatives remain under the same license.

See the [LICENSE](./LICENSE) file for more information.
