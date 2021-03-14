echo [Charlotte] Enabling executable bit on binaries.
chmod a+x charlotte-client analyzer actisense-serial

if test -e config
then
  echo [Charlotte] The 'config' file already exists, please open it to edit!
  exit 1
fi

echo [Charlotte] Creating config from template.
echo [Charlotte] Please open the 'config' file and update your Boat ID and API Key.
cp config.template config


