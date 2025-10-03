# BYTE-90: A Retro PC and MAC Inspired Interactive Designer Art Toy

BYTE-90 is a retro PC and Mac inspired interactive designer art toy that displays animated emotes through various interactions. It detects motion, responds to taps and orientation changes, pairs and communicates with other BYTE-90 devices to exchange animated conversations.

Designed and 3D printed by ALXV LABS (Alex Vong) as a designer toy, BYTE-90 is available as either a dev-kit or a collectible designer toy. This product is a limited run, and your support enables continued future development.

While the base software is open-source, **the animations, original designs, and 3D printed files remain proprietary** to maintain brand consistency and authenticity. You can purchase BYTE-90 that includes all hardware, software, and animations, or build your own using the off-the-shelf hardware specified below.

[BYTE-90 by ALXV Labs](https://labs.alxvtoronto.com/)

> **⚠️ Important**: Before contributing, forking, and or using this project commercially, please read our [Legal & Contributing Guidelines](CONTRIBUTING.md).

## Open Source Development

**This GitHub repository provides the open-source firmware foundation for**:

- Custom development and modifications
- Building your own DIY BYTE-90 hardware
- Learning and educational purposes
- Community contributions and improvements

**⚠️ Warning**: Compiling custom modifications to a purchased BYTE-90 device may cause:

- Loss of proprietary animations and visual effects
- Incompatibility with your specific hardware revision
- Potential device malfunction or boot issues

**Use the pre-built firmware releases for purchased devices, or proceed with custom firmware only if you understand the risks and have development experience.**

## Features

- **Interactive Animations**: Displays various animated GIFs through interactions and sequence events
- **Retro Screen Effects**: Choose from different vintage display effects including scanlines, dithering overlay, retro green, or classic orange CRT aesthetics
- **Motion Detection**: Advanced accelerometer-based detection for taps, double-taps, shakes, tilts, and orientation changes
- **Device-to-Device Communication**: Seamless pairing with other BYTE-90 devices via ESP-NOW protocol for animated conversations
- **Intelligent Power Management**: Progressive sleep modes with automatic inactivity detection to maximize battery life
- **Over-the-Air Updates**: Wireless firmware and animation updates via dedicated web interface
- **Web Serial API Updater**: Connect your BYTE-90 and update firmware and animations using Web Serial API on supported browsers at [https://install.alxv.dev/](https://install.alxv.dev/)

### Series 1 Firmware Version
- Version: 1.0.3 (Series 1)
- Release Date: June 16, 2025
- Compatibility: XIAO ESP32S3 hardware (Other ESP requires modification to pins)

### Current Firmware Version
- Version: 2.0.0 (Series 1 and 2)
- Release Date: October 3, 2025
- Compatibility: Only XIAO ESP32S3 hardware and BYTE-90 Series 2 PCB.

## Hardware Components (Series 1)

**Microcontroller**: [Seeedstudio XIAO ESP32S3](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- Dual-core Xtensa LX7 processor (240MHz)
- 8MB Flash / 512KB SRAM
- Built-in Wi-Fi and Bluetooth
- Compact form factor (21×17.5mm)
- Optimized for GIF processing and animation storage

**Display**: [Waveshare 1.5" RGB OLED Module (SSD1351)](https://www.waveshare.com/wiki/1.5inch_RGB_OLED_Module)
- 128×128 pixel full-color display
- RGB565 color format (65,536 colors)
- SPI interface with 20MHz frequency
- 16-level brightness control (6.25% to 100%)
- High contrast ratio perfect for retro aesthetics
- Low power consumption with display on/off control

**Motion Sensor**: ADXL345 Triple-Axis Accelerometer
- ±16g measurement range with configurable sensitivity
- I2C interface for communication
- Hardware interrupt capability for deep sleep wake
- Tap/double-tap detection with configurable thresholds
- Activity/inactivity detection for power management
- Orientation sensing for tilt/flip detection

**User Interface**: Single tactile push button
- Supports single click, long press (3+ seconds), and double-click detection
- Software debouncing (50ms) with 300ms double-click window
- Mode switching and interaction control

**Power System**: 
- USB-C charging and data interface (1-2 hours for full charge)
- Integrated battery management with visual charge indicators:
  - Red flashing light: Charging in progress
  - Solid red light: Charging complete
- Optional LiPo battery (103040 1200mAh, 3.7V with PH 2.0 connector)
- Battery life: Up to 2 days with intelligent power management
- Can operate via USB-C power without battery

**Storage**: 8MB Internal Flash Memory
- Custom partition layout for firmware and animations
- LittleFS filesystem for efficient file management
- Supports 2-3MB of animation data

## BYTE-90 Series 2 Hardware
BYTE-90 Series 2 introduces significant hardware enhancements that expand the device's capabilities beyond the original design. Built around a custom PCB architecture, Series 2 introduces new features providing enhanced user interaction, timekeeping, and audio feedback.

### Custom PCB Architecture ###
Series 2 is engineered around a fully integrated custom PCB that consolidates the XIAO ESP32S3, display, RTC, haptic driver, audio amplifier, and all supporting circuitry onto a single optimized board. This professional design eliminates external wiring while delivering:

- **Integrated Power Management**: Custom charging and power management system
- **Compact Form Factor**: Streamlined design with improved component integration

### New Hardware Features

**Real-Time Clock (RTC) Module**
- **PCF8563 Precision RTC**: High-accuracy timekeeping with low power consumption
- **Low Power Design**: Optimized for battery-powered applications
- **I2C Interface**: Seamless integration with existing communication bus
- **Clock Mode**: Dedicated display mode showing current time with customizable timezone support
- **NTP Synchronization**: Automatic time sync via WiFi when connected to network

**Haptic Feedback System**
- **DRV2605 Haptic Driver**: Advanced haptic motor controller with 123 built-in effects
- **ERM Motor Support**: Eccentric Rotating Mass motor for strong, tactile feedback
- **Custom Effect Library**: 20+ specialized haptic patterns for different interactions
- **Power Management**: Intelligent haptic timeout system to preserve battery life
- **Motion-Responsive**: Haptics automatically enable/disable based on device activity
- **User Preferences**: Configurable haptic intensity and enable/disable options

**Audio System**
- **MAX98357A I2S Amplifier**: High-quality digital audio amplifier with I2S interface
- **MP3 Decoder**: Hardware-accelerated MP3 playback with low CPU overhead
- **Sound Effects Library**: Comprehensive collection of system sounds and feedback
- **Audio Themes**: Context-aware sound effects for different interaction types
- **Volume Control**: Software-based volume management with user preferences
- **Power Optimization**: Audio system automatically powers down during inactivity

### Series 2 Enhanced Interactions

**Tactile Feedback**
- **Button Presses**: Haptic confirmation for all button interactions
- **Motion Events**: Contextual haptic feedback for taps, shakes, and orientation changes
- **Menu Navigation**: Subtle haptic cues for menu selection and navigation
- **System Events**: Audio and haptic feedback for power state changes and notifications

**Time Display Mode**
- **Always-On Clock**: Dedicated mode for continuous time display
- **Multiple Timezones**: Support for 10+ timezone configurations
- **Auto-Sync**: Automatic time synchronization when WiFi is available
- **Power Efficient**: Optimized display and minimal processing for extended battery life

**Enhanced Audio Experience**
- **System Sounds**: Audio feedback for power on/off, menu interactions, and notifications
- **Emotion Sounds**: Contextual audio effects that complement visual animations
- **Typing Sounds**: Retro keyboard click sounds for menu navigation
- **Ambient Audio**: Subtle background audio to enhance the retro computing experience

## Series 2 Compatibility

### Hardware Requirements ###
- Enhanced features require Series 2 custom PCB configuration
- All Series 2 features are automatically detected and enabled
- Full backward compatibility with Series 1 functionality and accessories

### Software Integration ###
- Series 2 capabilities integrated directly into core firmware architecture
- No additional configuration required for basic functionality
- Advanced customization available through expanded settings interface

## Safety & Device Usage

### Battery Safety
- Use ONLY the specified 3.7V lithium battery (103040 size) with PH 2.0 connector
- Verify 3.7V voltage (NOT 3.9V) before installation
- **Check connector polarity alignment carefully**
- Handle battery connector with care to avoid damage

### Environmental Safety
**BYTE-90 is designed as a desktop device for indoor use only.**
- ⚠️ Do NOT use in vehicles or automotive environments - Heat, temperature fluctuations, and vibrations in cars can cause device overheating, battery damage, or malfunction.
- ⚠️ Avoid high-temperature environments - Operating temperature should remain below 35°C (95°F) to prevent overheating.
- ⚠️ Keep away from direct sunlight - Prolonged sun exposure can cause overheating and display damage.

## Pin Configuration

- [WaveShare Display Pinout](https://www.waveshare.com/img/devkit/LCD/1.5inch-RGB-OLED-Module/1.5inch-RGB-OLED-Module-details-3.jpg)
- [XIAO ESP32S3 Pinout](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/2.jpg)

```
ESP32S3 XIAO Pin Assignments:
├── Display (SSD1351) - SPI Interface
│   ├── VCC -> 3.3V (or 5V if PCB supports)
│   ├── GND -> GND
│   ├── DIN -> D10 (GPIO9) - MOSI/SPI Data
│   ├── SCK -> D8 (GPIO7) - SPI Clock  
│   ├── CS  -> D7 (GPIO44) - Chip Select
│   ├── DC  -> D6 (GPIO43) - Data/Command
│   └── RST -> D0 (GPIO1) - Reset
├── Accelerometer (ADXL345) - I2C Interface
│   ├── VCC -> 3.3V
│   ├── GND -> GND
│   ├── SDA -> D4 (GPIO5) - I2C Data
│   ├── SCL -> D5 (GPIO6) - I2C Clock
│   └── INT -> D1 (GPIO2) - Interrupt for Deep Sleep Wake
├── Button Interface
│   ├── Input -> A3 (GPIO4) - with internal pull-up
│   └── Ground -> GND
└── Power (Optional LiPo Battery)
    ├── Positive -> BAT+ (Red wire)
    └── Negative -> BAT- (Black wire)
```

**Pin Reference Table:**
| XIAO Pin | GPIO | Function | Component |
|----------|------|----------|-----------|
| D0 | GPIO1 | RST | Display Reset |
| D1 | GPIO2 | INT | ADXL345 INT1 Interrupt |
| D4 | GPIO5 | SDA | ADXL345 I2C Data |
| D5 | GPIO6 | SCL | ADXL345 I2C Clock |
| D6 | GPIO43 | DC | Display Data/Command |
| D7 | GPIO44 | CS | Display Chip Select |
| D8 | GPIO7 | SCK | Display SPI Clock |
| D10 | GPIO9 | MOSI | Display SPI Data |
| A3 | GPIO4 | INPUT | Button (with pull-up) |

## System Architecture

### Operating Modes

**ESP Mode (Default Runtime)**
- Primary operational state for interactive use
- Continuous animation playback and motion monitoring
- ESP-NOW communication active for device pairing
- Automatic power state transitions based on activity
- Real-time response to user interactions

**Update Mode (Configuration & Maintenance)**
- Activated via Settings Menu
- Creates Wi-Fi access point: "BYTE90_Setup" (password: `00000000`)
- Web-based configuration interface at `192.168.4.1`
- Enables firmware updates, animation uploads, and system diagnostics
- Automatically restarts device after successful updates

### Power Management System

BYTE-90 implements progressive power management with four distinct states:

**Active State** - Full Operation
- Current draw: ~80-120mA
- All sensors and communication active
- Immediate response to interactions
- Full display brightness

**Display Dimming** - 30 Minutes of Inactivity
- Display brightness reduced to 25%
- Estimated: ~90-110mA (25% display brightness)
- All functionality remains active
- Motion detection continues normally

**Sleep Mode** - 1 Hour of Inactivity
- Sleep animations displayed
- Current draw: ~40-60mA

**Deep Sleep** - 1.5 Hours of Inactivity
- 20-second countdown preparation time
- Current draw: ~35-40μA
- **Only accelerometer interrupt active**
- Wake on tap or significant motion
- Essential for 2-day battery life

## Interactive Animation System

### Motion Detection & Responses

The BYTE-90 uses sophisticated motion detection with hardware-based tap detection and software-based motion analysis:

**Tap Detection** (Hardware ADXL345 Interrupt)
- **Single Tap**: 14.0G threshold, 30ms duration window
- **X/Y-axis taps**: Trigger acknowledgment animations
- **Hardware debouncing**: Prevents false triggers

**Double-Tap Detection** (Within 250ms Window)
- **X/Y-axis double-tap**: Trigger surprised or shocked animations
- **Latency window**: 100ms between tap detections

**Shake Detection** (Continuous Motion Analysis)
- **Threshold**: 8.0 m/s² average magnitude
- **Triggers**: Dizzy/confused animations
- **Lockout period**: 500ms after tap events to prevent false positives
- **Sample averaging**: Multiple readings for stability

**Orientation Detection** (Multi-Axis Tilt Sensing)
- **Full Tilt**: ±9.0 m/s² threshold for left/right orientation (±90°)
- **Half Tilt**: ±4.2 m/s² threshold for 45-degree angle detection
- **Upside Down**: Z-axis ≤ -8.0 m/s² (accounts for PCB mounting)
- **Triggers**: Crash animations, with recovery sequences

**Sudden Acceleration Detection** (Dynamic Movement)
- **Acceleration threshold**: 6.0 m/s² for rapid movement
- **Change threshold**: 4.0 m/s² difference between readings
- **Lockout protection**: 600ms cooldown after tap/shake events
- **Gravity compensation**: Removes static 9.8 m/s² component
- **Triggers**: Startle animations, surprise reactions

### Device Pairing (ESP-NOW Communication)

- **Range**: Up to 200 meters in open space
- **Latency**: <50ms for animation triggers
- **Auto-discovery**: Automatic pairing with nearby BYTE-90 devices (max 2 connections)
- **Conversations**: Paired devices engage in animated sequences
- **Activation**: Toggles via Settings Menu

## User Interface & Controls

### Button Operations
- **Single Click**: Toggles Settings Menu
- **Double Click**: Enters or Selects menu setting
- **Long Press (3+ seconds)**: Enter deep sleep

### Motion Interactions
- **Single Tap**: X/Y-axis acknowledgment animations
- **Double Tap**: X/Y-axis shocked/surprised animations  
- **Shake**: Dizzy/confused animations
- **Tilt**: Crash and recovery animations based on angle
- **Sudden Movement**: Startle/surprise reactions

## Configuration & Updates

### Web Interface Access

1. **Enter Update Mode**: Toggle via Settings Menu
2. **Connect to Wi-Fi**: Join "BYTE90_Setup" network (password: `00000000`)
3. **Access Interface**: Navigate to `http://192.168.4.1`
4. **Configure**: Set up Wi-Fi, upload firmware/animations
5. **Exit**: Exit via Settings Menu

**⚠️ Windows 11 compatibility**: Windows 11 has a compatibility issue with DHCP access points, it fails or takes a long time to assign IP preventing connection to the Web Interface. A workaround is to manually assign the IP address once connected or use an iOS or Android device.

### Update Methods
- **Over-the-Air**: Upload `.bin` files via web interface
- **Web Serial API**: Direct USB connection at [https://install.alxv.dev/](https://install.alxv.dev/)
- **Automatic validation**: File integrity and format verification
- **Rollback protection**: Safe update process with error recovery

## Animation Asset Protection Notice

- **⚠️ Important for BYTE-90 Purchasers**: Animations are paid proprietary assets included only with purchased BYTE-90 devices. When performing firmware updates or flashing operations, be careful not to erase the animation partition from flash memory.

- **⚠️ If animations are accidentally erased**: Contact ALXV Labs support with your purchase information including order number for assistance in restoring your animations.

- **For DIY builders**: This open source firmware **does not include animations**. Animations are available **ONLY with purchased BYTE-90 devices**.

## Development & Building

### Prerequisites
- [PlatformIO](https://platformio.org/) (VS Code extension recommended)
- Git version control
- Python 3.7+ for build tools
- USB-C cable for programming and debugging

### Required Libraries
```ini
lib_deps = 
    bitbank2/AnimatedGIF@2.1.1                    # GIF decoding
    adafruit/Adafruit GFX Library@^1.11.11         # Graphics
    adafruit/Adafruit SSD1351 library@^1.3.2       # Display driver
    adafruit/Adafruit ADXL345@^1.3.4               # Accelerometer
```
**⚠️ Important note**: Latest version of AnimatedGIF V2.2.0 introduces breaking changes and creates GIF artifacts on display. I have not updated to this version and don't see a reason too until I explore the breaking changes.

### Build Configuration
```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino
monitor_speed = 115200
build_flags = 
	-DCORE_DEBUG_LEVEL=5
  -DFIRMWARE_VERSION=\"1.0.X\"
board_build.filesystem = littlefs
board_build.partitions = custom_partitions.csv
lib_deps = 
	bitbank2/AnimatedGIF@2.1.1
	adafruit/Adafruit GFX Library@^1.11.11
	adafruit/Adafruit SSD1351 library@^1.3.2
	adafruit/Adafruit ADXL345@^1.3.4
```

### Animation Asset Requirements
- **Resolution**: Exactly 128×128 pixels
- **Frame rate**: 16 FPS recommended
- **Color depth**: 8-bit indexed color (256 colors max)
- **Format**: Optimized GIF with LZW compression [Use EZgif.com](https://ezgif.com/)

### Personality Modes
Configure different retro computing aesthetics to call mode based animations:

- **BYTE_MODE (Default)** - Original BYTE-90 character designs
- **MAC_MODE** - Retro Macintosh-inspired Happy Mac expressions  
- **PC_MODE** - Retro PC Happy Mac expressions

## Troubleshooting

### Common Issues

**Device Not Responding**
- **Charge first**: New devices need 1-2 hours initial charging
- **Reset**: Press "Reset" button on Xiao ESP32-S3 board
- **USB power**: Try direct USB-C power without battery

**Random Restarts or Freeze**
- **Cause**: Critically low battery
- **Solution**: Charge immediately for 1-2 hours
- **Hardware Reset**: Press reset button on board

**No Display (Black Screen)**
- **Cause**: Empty battery for extended periods requires a hard reset via the Reset button on the Xiao ESP32-S3 board
- **For DIYers**: Make sure you have your wiring correct and using the supported display, other aftermarket displays will require custom PIN configurations.

**Update Mode Issues**
- **Windows 11**: Known compatibility issue with ESP32 access points
- **Workaround**: Use macOS/iOS devices or manual IP assignment
- **Check**: Ensure "BYTE90_Setup" network is visible
- **Password**: `00000000` (eight zeros)
- **Alternative**: Use USB flashing method via PlatformIO (Development knowledge required)
- **Important**: Early BYTE-90 devices may have poor antenna signals due to the WiFi antenna placement. If you are not seeing "BYTE90_Setup" network, open the backcover and reposition the WiFi antenna over top the battery.

## Frequently Asked Questions

**Q: How long does the battery last?**
Up to 2 days with intelligent power management and progressive sleep modes.

**Q: Can I check battery percentage?**
Currently not available due to hardware limitations. Feature planned for future releases.

**Q: How do I turn off BYTE-90?**
No power switch - device uses automatic sleep modes. For complete shutdown, disconnect battery. This is a hardware and space limitation.

**Q: Why do interactions seem delayed?**
Intelligent lockout periods prevent false triggers:
- 500ms after tap events
- 600ms after acceleration detection
- Priority system: shake > tap > acceleration > orientation

**Q: Can I modify the animations or sensitivity?**
Requires programming knowledge. Animations remain proprietary, but motion thresholds can be adjusted in firmware.

**Q: Can I create commercial products using this firmware?**
Yes, under GPL v3.0 terms, but you **cannot use BYTE-90 branding or any BYTE-90 proprietary assets including original designs, 3D printed models, and animations** See [Legal Guidelines](CONTRIBUTING.md) for details.

**Q: What's the difference between the open source firmware and commercial version?**
- Open source includes core functionality
- Commercial version includes full hardware and proprietary animations with device support and exclusive access to new feature releases.

## Support & Community

### What We Support
- **Core firmware** functionality and documented bugs
- Hardware compatibility issues with **OFFICIAL BYTE-90 hardware**
- Basic troubleshooting for common issues

### What We Don't Support
- Custom animation development
- Hardware modifications or alternative component integration
- Third-party component compatibility
- Commercial deployment assistance or licensing guidance
- Extensive debugging of modified or forked firmware

**Official Resources:**
- [Support Page](https://labs.alxvtoronto.com/pages/support) - User guides and FAQ
- [GitHub Repository](https://github.com/alxv2016/Byte90-alxvlabs) - Open source firmware
- [ALXV Labs](https://labs.alxvtoronto.com/) - Product information
- [Legal & Contributing Guidelines](CONTRIBUTING.md) - Licensing and contribution rules

**Support Level**: Limited technical implementation support. Users expected to have Arduino/ESP32 development experience.

## Contributing

We welcome contributions from the community! Before contributing, please:

1. Read our [Legal & Contributing Guidelines](CONTRIBUTING.md)
2. Review the contribution guidelines for code quality standards
3. Test thoroughly on actual hardware before submitting
4. Follow our attribution requirements for derivative works

For bug reports, feature requests, and detailed contribution guidelines, see [Legal & Contributing Guidelines](CONTRIBUTING.md).

### Acknowledgements
- **Adafruit Industries**: Hardware libraries and sensor drivers
- **Bitbank2**: AnimatedGIF library for efficient GIF rendering
- **Espressif Systems**: ESP32 development framework and tools
- **Seeedstudio**: XIAO ESP32S3 development board design
- **Community Contributors**: Open source development and testing

## BYTE-90 Intellectual Property Rights

### Brand Protection Notice
**BYTE-90 is a brand name of ALXV Labs.** Use of the BYTE-90 name or branding in derivative projects requires explicit written permission. For more information on BYTE-90 Legal & Community Guidelines: [CONTRIBUTING.md](CONTRIBUTING.md)

### Protected Assets (© 2025 Alex Tieu Long Vong, ALXV Labs):
- BYTE-90 logo and brand identity
- All animations and character designs
- Visual identity and design elements
- 3D printed models and digital files
- Product packaging and marketing materials

### Prohibited Actions:
1. Distribution of BYTE-90 animations, designs, or 3D models
2. Modification and distribution of proprietary assets
3. Creating competing commercial products using **BYTE-90 branding**
4. Commercial exploitation without authorization using BYTE-90 branding
5. Sharing proprietary assets through other platforms or media
6. Creation of derivative works based on proprietary assets

### Permitted Fair Use:
- Educational discussion and analysis
- Non-commercial review and commentary
- Technical documentation referencing functionality
- Academic research and study

---

*Designed and developed by Alex Vong, ALXV LABS. This project represents the intersection of retro computing nostalgia and modern interactive design, creating a unique interactive designer toy experience for makers and collectors alike.*

**Version**: 2.0.0 | **Last Updated**: October 3, 2025 | **License**: GPL v3.0 | **Legal**: [CONTRIBUTING.md](CONTRIBUTING.md)