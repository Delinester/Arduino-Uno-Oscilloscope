# Arduino Uno Oscilloscope with TFT Display

This project transforms an Arduino Uno with a 2.4" TFT display into a functional oscilloscope.

## Dependencies

This project requires the following libraries:

- Adafruit_GFX
- MCUFRIEND_kbv
- TouchScreen

## Hardware

- Arduino Uno
- 2.4" TFT Display

## Setup

1. Connect the 2.4" TFT Display to your Arduino Uno.
2. Connect the analog input pin (A5 by default) to the signal you want to measure.

## Usage

Upload the provided code to your Arduino Uno. The oscilloscope will start automatically.

The oscilloscope has the following features:

- **Pause/Resume**: Press the "P" button on the screen to pause/resume the oscilloscope.
- **Time Step Adjustment**: Use the "+" and "-" buttons on the screen to adjust the time step of the oscilloscope.

## Code Explanation

The code works by continuously reading from the analog input pin and plotting the values on the TFT display. The values are scaled to fit on the display and are drawn in real-time, creating an oscilloscope effect.

The time step (the delay between each reading) can be adjusted with the "+" and "-" buttons on the screen. The oscilloscope can also be paused and resumed with the "P" button.

The code also calculates and displays the current, average, peak, and minimum voltages.

## Note

This is a simple oscilloscope suitable for low-frequency signals. It is not intended to replace a professional oscilloscope for high-frequency or complex signal analysis.
