# 🎸 Pad Hero – Arduino MIDI Game

**Pad Hero** is an Arduino-based rhythm game inspired by Guitar Hero. Players must hit physical buttons in sync with incoming MIDI notes while colorful LEDs guide the timing and progress of each note.

---

## 🚀 Features

- 🎵 **Real-time MIDI Input** via USB (MIDIUSB library)
- 💡 **Visual Gameplay with NeoPixel LED Matrix** (5x10 columns)
- 🔘 **5 Physical Buttons** for note matching
- ⏱️ **Adjustable Timing, Velocity & Pitch Range**
- 🔊 **MIDI Playback Support**
- 📟 **Two TM1637 7-Segment Displays** for time, score & feedback
- 🌗 **Brightness and Mode Switches** for day/night and game modes
- 🔄 **Circular MIDI Buffer** for efficient note tracking
- 📊 **Scoring and Accuracy Tracking** with animated visual feedback

---

## 🎮 How to Play

1. Upload the code to your Arduino board (Mega/Uno with USB MIDI support recommended).
2. Connect:
   - 5 buttons to the specified pins
   - TM1637 displays
   - NeoPixel strip (50 LEDs)
3. Send MIDI notes via a DAW or MIDI sequencer over USB.
4. Match the falling notes by pressing the correct buttons in time.
5. Watch your score and accuracy on the displays.

---

## 📦 Hardware Requirements

- Arduino board (with USB support, e.g., Leonardo, Micro, or Mega)
- Adafruit NeoPixel (50 LEDs)
- TM1637 7-segment displays ×2
- 5 push buttons (for gameplay)
- Additional buttons for mode/brightness (optional)
- 5 LEDs for feedback (optional)
- USB MIDI connection

---

## 🛠️ Libraries Used

Make sure you have these libraries installed:

- [`Adafruit_NeoPixel`](https://github.com/adafruit/Adafruit_NeoPixel)
- [`TM1637Display`](https://github.com/avishorp/TM1637)
- [`TM1637TinyDisplay`](https://github.com/bxparks/TM1637TinyDisplay)
- [`MIDIUSB`](https://github.com/arduino-libraries/MIDIUSB)

---

## 🧠 Game Modes & Controls

| Button         | Function                     |
|----------------|------------------------------|
| Button 1-5     | Gameplay input / settings nav |
| Brightness Btn | Toggle day/night brightness  |
| SmallMode Btn  | Switch between 4 or 5 lanes  |
| Chords Btn     | Toggle chord filtering mode  |
| Timer Btn      | Enable/disable game timer    |

---

## 🔧 Configuration

You can also set parameters via the serial monitor:

