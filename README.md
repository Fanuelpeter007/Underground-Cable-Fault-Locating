# Underground Cable Fault Locator

## Introduction
This project focuses on developing an **Underground Cable Fault Locator** designed to enhance the accuracy and efficiency of detecting faults in underground power transmission cables. The solution utilizes both traditional adaptive filtering techniques (such as LMS filters) and modern deep learning-based audio classification methods to isolate and detect thumping sounds emitted from fault points, despite interference from environmental noise.

The system integrates key components, including a **STM32 microcontroller**, **Raspberry Pi**, and custom software for real-time signal analysis and fault detection.

---

## Methodology

### Problem Definition
Locating faults in underground cables is challenging due to interference from background noise, imprecise traditional methods, and high costs associated with advanced detection equipment. This project aims to:
- Reduce background noise using an adaptive filtering algorithm (LMS filter).
- Implement a deep learning model to classify thumping sounds and distinguish them from noise.
- Deliver a cost-effective solution using widely available hardware and software.

### Proposed Design
The system includes:
1. **Least Mean Squares (LMS) Adaptive Filter**: Removes noise to isolate the thumping sound.
2. **Deep Learning Audio Classification Model**: Enhances accuracy by classifying thumping sounds using features like Mel-Frequency Cepstral Coefficients (MFCCs).
3. **Hardware Implementation**: STM32 microcontroller for real-time filtering and Raspberry Pi for audio classification.
4. **Integration with Existing Systems**: Designed for compatibility with current underground fault-detection equipment.

---

## Key Features
- **Real-time Noise Cancellation**: Adaptive filtering using LMS ensures effective background noise suppression.
- **Accurate Fault Detection**: Deep learning model trained to identify fault-related sounds with a confidence level of 86% in prototype testing.
- **Cost-Effectiveness**: Developed with affordable components to keep costs below 5000 AED.
- **User-Friendly Interface**: Outputs include a graphical display for detected signals and integration with audio equipment for listening to filtered sounds.

---

## Components and Specifications
### Hardware
- **STM32F401RE Microcontroller**: Handles adaptive filtering and audio signal processing.
- **Raspberry Pi**: Runs the deep learning model for audio classification.
- **INMP441 Microphone**: Captures sound with clarity and minimal noise susceptibility.
- **Pmod I2S2 Module**: Interfaces the microphone with the microcontroller.
- **3D-Printed Enclosure**: Protects internal components during operation.

### Software
- **MATLAB Simulink**: Used for LMS algorithm simulation and initial testing.
- **Edge Impulse**: Platform for training the deep learning audio classification model.
- **Python**: Programming language for implementing the real-time classifier on Raspberry Pi.

---

## Results
- **MATLAB Simulation**: Demonstrated effective noise cancellation for simulated and real-time audio inputs.
- **Deep Learning Model Training**: Achieved 96% accuracy in classifying thumping sounds during training.
- **Prototype Testing**: Successfully detected fault sounds buried under environmental noise, confirming the system's practicality.

---

## Contributors
- Fanuel Petros 
- Tomas Yemane 


