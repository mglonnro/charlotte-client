echo Enabling executable bit on binaries.
chmod a+x charlotte-client analyzer actisense-serial

if test -e config
then
  echo Config already exists, ok.
else
  echo Creating config from template.
  echo      
  echo Please open the 'config' file and update your BOATID!
  echo
  cp config.template config
fi


