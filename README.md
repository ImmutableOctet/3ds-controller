# 3ds-controller
A 3DS homebrew application that allows you to send input to a remote machine. For a likely more functional implementation, made by CTurt, [click here](https://github.com/CTurt/3DSController). I didn't realize that existed before putting this together, but I intend to make a *nicer version*. At least, code-wise.

Functionality is currently experimental, and input simulation could be broken. This project is based on an input-demo from the official ctrulib examples. The ["ftbrony"](https://github.com/mtheall/ftbrony) application was used as a reference for the SOC initialization.

There is only support for Windows at this time. Input is sent as raw data, and does not (Currently) comply with network byte-order.

The basic data-layout can be found [here](https://github.com/Sonickidnextgen/3ds-controller/blob/master/3ds_controller/source/shared.h) ('fullInputData' structure). Input-data is sent over ***TCP***, and is currently hard-coded to ***port 4865***. You may specifiy an IP address / hostname using the ["address.txt"](/3ds_controller/address.txt) file in the application's directory.
