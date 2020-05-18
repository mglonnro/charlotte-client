echo Copying libraries to /usr/local/lib.
cp lib/libwsclient* /usr/local/lib
cp lib/libcurl* /usr/local/lib
ln -sf /usr/local/lib/libwsclient.so.0.0.0 /usr/local/lib/libwsclient.so.0
ln -sf /usr/local/lib/libwsclient.so.0.0.0 /usr/local/lib/libwsclient.so
ln -sf /usr/local/lib/libcurl.so.4.6.0 /usr/local/lib/libcurl.so.4
ln -sf /usr/local/lib/libcurl.so.4.6.0 /usr/local/lib/libcurl.so

echo Configuring libraries.
ldconfig -v > install.log 2>&1

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


