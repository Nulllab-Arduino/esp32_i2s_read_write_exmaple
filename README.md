# ESP32 I2S Audio Echo Example

This is an Arduino example program for ESP32/ESP32-S3 demonstrating I2S audio capture and playback (audio loopback/echo).

## Hardware Requirements

- **MCU**: ESP32 or ESP32-S3
- **Microphone**: ZTS6631 or INMP441 (I2S interface)
- **Amplifier**: NS4168
- **Speaker**: Connected to the amplifier output

## Hardware Connection

### Speaker / Amplifier (NS4168) - I2S TX (I2S0)

| Signal Name | ESP32 Pin | ESP32-S3 Pin | Common Silk Screen Aliases |
| :--- | :--- | :--- | :--- |
| BCLK | GPIO27 | GPIO4 | BCK / BCL |
| WS | GPIO26 | GPIO5 | LRCK / LRC / LRCLK |
| DOUT | GPIO25 | GPIO6 | DAT / DATA / DIN |

> Note: MCLK is not used in this example, no wiring is needed.

### Microphone (ZTS6631 / INMP441) - I2S RX (I2S1)

| Signal Name | ESP32 Pin | ESP32-S3 Pin | Common Silk Screen Aliases |
| :--- | :--- | :--- | :--- |
| BCLK | GPIO23 | GPIO40 | BCK / BCL |
| WS | GPIO32 | GPIO41 | LRCK / LRC / LRCLK |
| DIN | GPIO33 | GPIO42 | DAT / DATA / DOUT |

> Note: MCLK is not used in this example, no wiring is needed.

### ⚠️ Important Notices

#### 1. Acoustic Feedback (Howling)

**Please keep the microphone physically away from the speaker.**
If the microphone is too close to the speaker, it will pick up the output sound and create a **positive feedback loop**. This results in a loud, high-pitched, and extremely unpleasant squealing noise (howling).
To avoid this, ensure there is sufficient distance between the mic and the speaker, or reduce the speaker volume.

#### 2. Power Supply Requirement

**Please use an external power supply (5V @ 1A+ recommended).**
Do not rely solely on the 5V rail from a computer's USB port. The NS4168 audio amplifier consumes significant current, especially at high volumes. Insufficient power supply can cause **voltage drops**, leading to ESP32 crashes, random restarts, or I2S transmission errors.
If you encounter stability issues, always check your power source first.

## Software Requirements

- **IDE**: Arduino IDE
- **Board Package**: `esp32 by Espressif Systems` (Version >= 3.3.10-CN or >= 3.3.10)
- **Dependency Library**: `cyfney-cpp` (Search and install via Arduino Library Manager)

## Setup Instructions

1. Install [Arduino IDE](https://www.arduino.cc/en/software).
2. Add ESP32 board support:
   - Go to `Tools` -> `Board` -> `Boards Manager`, search for `esp32` and install `esp32 by Espressif Systems` (ensure version >= 3.3.10).
3. Install the dependency library:
   - Go to `Sketch` -> `Include Library` -> `Manage Libraries...`
   - Search for `cyfney-cpp` and install it.
4. Select your board in `Tools` -> `Board`:
   - For ESP32: Select `ESP32 Dev Module`
   - For ESP32-S3: Select `ESP32S3 Dev Module`
5. Compile and upload the sketch to your device.

## Expected Behavior

- Upon booting, the device will play a **"Hello World"** audio prompt through the speaker.
- After that, it will continuously capture audio from the microphone and immediately play it back through the speaker, forming an **audio echo (loopback) test**.
- You can speak into the microphone, and the sound will be played back from the speaker with a slight delay.

## Code Overview

- Uses the new ESP-IDF I2S driver (`driver/i2s_std.h`).
- TX (Speaker): 16-bit, 16kHz, Mono (Slot Mask: BOTH).
- RX (Microphone): 32-bit, 16kHz, Mono (Slot Mask: LEFT).
- Reads 20ms of audio data per loop, processes the 32-bit microphone data into 16-bit samples, and writes them to the I2S transmitter.

### 📝 Bit-shifting and Gain Adjustment

In the `loop()` function, you will notice the following line:

```cpp
int32_t sample = raw_32bit_samples[i] >> 14;
```

**Why shift by 14 instead of 16?**

- MEMS microphones (ZTS6631/INMP441) output **24-bit valid audio data**, left-aligned in a 32-bit I2S frame. The lower 8 bits of the 32-bit value are zero-padding, so we first discard them by right-shifting 8 bits to get the raw 24-bit audio data.
- To convert 24-bit data to 16-bit for the I2S transmitter, the standard approach is to right-shift another 8 bits (total shift of 16), scaling the signal to the 16-bit signed integer range (-32768 ~ 32767).
- However, MEMS microphones have inherently low analog sensitivity, and the NS4168 amplifier requires a higher input level to drive the speaker to an audible volume. By shifting only 14 bits in total (instead of 16), we reduce the total shift by 2 bits, effectively applying a **4x gain (2^(16-14) = 4)** to the signal. This intentionally amplifies the mic input to compensate for hardware sensitivity limitations.

**Adjusting the Echo Volume:**

- **Smaller number (e.g., 12 or 13)**: Fewer shifts, higher gain (16x or 8x). The echo will be louder, but clipping/distortion is very likely.
- **Larger number (e.g., 15 or 16)**: More shifts, lower gain (2x or 1x). The echo will be quieter; 16 is the standard lossless conversion value (no gain applied).

You can tweak this value to match your specific microphone sensitivity and speaker volume.

## License

MIT License
