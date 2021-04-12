# Charlotte

[<img width="400" alt="Mapbox" src="https://storage.googleapis.com/charlotte-public/og_image_default.png">](https://beta.charlotte.lc/)

Charlotte is a system for gathering, storing and analyzing NMEA data (data from marine sensors) in the cloud. 

- DEMO: [Latest live data from s/y Carlotte](https://charlotte.lc/boats/2zGrCQC2X9X2LbkzMhFm)
- DEMO: [A trip from Hel, Poland to Klaipėda, Lithuania](https://charlotte.lc/boats/2zGrCQC2X9X2LbkzMhFm/trips/ZhYjSI2dVcXNPgcw80E5)
- SEE ALO: [How to use the UI tutorial videos](https://nakedsailor.blog/2021/03/18/charlotte-your-digital-crew-member/)

The system consists of the following components:

1. A local device (i.e. Raspberry PI) that is physically connected to the NMEA network and records *all* the data it sees. Whenever the local device is connected to the internet, it can also a) *live stream real-time data* to the cloud, and b) *upload previously recorded data* for visualization and analytics. (See the **Hardware Example** section below for more details.)
2. A cloud infrastructure to store the data and prepare+load it for analytics (implemented using Google Cloud Storage, Google Compute Engine, TimescaleDB).
3. A web UI to access the processed data (https://charlotte.lc)

The binary distribution bundle below includes all the software components of the system: 

- `actisense-serial` from the CANboat project (Apache 2.0 License, (c) Kees Verruijt), see more: https://github.com/canboat/canboat
- `analyzer` from the CANboat project (Apache 2.0 License, (c) Kees Verruijt), see more: https://github.com/canboat/canboat
- `charlotte-logger` for recording and uploading all the logged data (the "black box" functionality), see more: https://github.com/mglonnro/charlotte-logger
- `charlotte-client` for establishing the real-time data stream between the local device and the cloud, see more: see more: https://github.com/mglonnro/charlotte-client

## Installation

1. Install the binaries to a Raspberry using our repository: 

```
# Add our repository
sudo sh -c "echo 'deb https://storage.googleapis.com/charlotte-repository ./' > /etc/apt/sources.list.d/charlotte.list"
wget --quiet -O - https://storage.googleapis.com/charlotte-repository/gpgkey | sudo apt-key add -
sudo apt-get update

# Install charlotte
sudo apt-get install charlotte
```

2. Register your boat on https://charlotte.lc and get your `Boat ID` and `API Key`. 

4. Edit `/etc/charlotte.conf` and add your `Boat ID` and `API Key`.

5. Start the service 

```
sudo systemctl start charlotte
```

6. Enable automatic start at boot

```
sudo systemctl enable charlotte
```

## Distribution Bundle

The charlotte client binary distribution is a bundle that contains
the following binaries with their separate and individual licenses: 

### analyzer, actisense-serial
https://github.com/canboat/canboat

(C) 2009-2021, Kees Verruijt, Harlingen, The Netherlands.

This file is part of CANboat.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

### libcurl
https://curl.haxx.se/docs/copyright.html

COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1996 - 2020, Daniel Stenberg, daniel@haxx.se, and many contributors, see the THANKS file.

All rights reserved.

Permission to use, copy, modify, and distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without prior written authorization of the copyright holder.

### libwebsockets.org
https://libwebsockets.org/
MIT License

# Hardware Example

Here is a more detailed list of what components may be used to record the NMEA data (the *local device*): 

1. A Raspberry Pi 3 Model B+ or Pi 4, with a case. 
2. A step-down converter/regulator to get good 12V/24V => 5V power from the boat to the Raspberry. This one can be plugged into the boat's 12V DC system with normal wires, and provides an USB connector for the Raspberry power cable: https://www.amazon.com/Converter-DROK-Regulator-Inverter-Transformer/dp/B01NALDSJ0
3. From the step-converter to the Raspberry a _thick and short_ standard USB cable (the Raspberry is picky if the voltage drops).
4. A Yacht Devices USB-NMEA2000 gateway (with USB male and NMEA2000 Micro Male) that will be connected between the Raspberry and the NMEA network
https://www.yachtd.com/products/usb_gateway.html
5. A suitable NMEA2000 Micro-C cable to connect the USB-NMEA gateway to your boat's network. 
6. A Wifi/internet access point that the Raspberry can connect to a. (I am using a Huawei E5577S that I can plug into my boat's USB charging socket for constant power and 24/7 data collection: https://consumer.huawei.com/sg/support/routers/e5577s/.)

![Hardware Example](https://storage.googleapis.com/charlotte-public/hw_model.png)
