cp lib/libwsclient* /usr/local/lib
cp lib/libcurl /usr/local/lib
ln -sf /usr/local/lib/libwsclient.so.0.0.0 /usr/local/lib/libwsclient.so.0
ln -sf /usr/local/lib/libwsclient.so.0.0.0 /usr/local/lib/libwsclient.so
ln -sf /usr/local/lib/libcurl.so.4.6.0 /usr/local/lib/libcurl.so.4
ln -sf /usr/local/lib/libcurl.so.4.6.0 /usr/local/lib/libcurl.so
ldconfig -v

