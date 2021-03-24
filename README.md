# charlotte-client

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

