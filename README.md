# Charlotte Client

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

2. Fill in your boat ID and API key in `/etc/charlotte.conf`.

- To get the ID and the key, create a boat on https://charlotte.lc

3. Start the service 

```
sudo systemctl start charlotte
```

4. Enable automatic start at boot

```
sudo systemctl enable charlotte
```

## Distribution Bundle

The charlotte client binary distribution is a bundle that contains
the following binaries with their separate and individual licenses: 

### analyzer, actisense-serial
https://github.com/canboat/canboat

(C) 2009-2015, Kees Verruijt, Harlingen, The Netherlands.

This file is part of CANboat.

CANboat is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CANboat is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CANboat.  If not, see <http://www.gnu.org/licenses/>.

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

