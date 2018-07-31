Lightbringer boards
-------------------

* Start with setup instructions in https://github.com/aphelps/HMTL/wiki/First-steps

* Depending on the board, these are the configuration files to use (configs directory):
  - Arduino nano: arduino_nano_hmtl_v1r5.json
  - Arduino mini:
  - Custom ATMEGA328:
  - Custom ATMEGA1284:

* ...

amp@AMP-MBP-home:~/Dropbox/Arduino/HMTL/platformio/HMTLPythonConfig [master]$ cd ../HMTLPythonConfig/
amp@AMP-MBP-home:~/Dropbox/Arduino/HMTL/platformio/HMTLPythonConfig [master]$ platformio run -t upload -e nano
amp@AMP-MBP-home:~/Dropbox/Arduino/Lightbringer/configs [master]$ HMTLConfig -v -f arduino_nano_hmtl_v1r5.json  -w -a 2 -i 2

*** 328 ***

amp@AMP-MBP-home:~/Dropbox/Arduino/HMTL/platformio/HMTLPythonConfig [master]$ cd ../HMTLPythonConfig/
amp@AMP-MBP-home:~/Dropbox/Arduino/HMTL/platformio/HMTLPythonConfig [master]$ platformio run -t upload -e uno
amp@AMP-MBP-home:~/Dropbox/Arduino/Lightbringer/configs [master]$ HMTLConfig -v -f /Users/amp/Dropbox/Arduino/Lightbringer/configs/HMTLv10_328_5x69.json -a XXX -i XXX -w

amp@AMP-MBP-home:~/Dropbox/Arduino/HMTL/platformio/HMTL_Module [master]$ platformio run -e lightbringer_328 -t upload