# 3ds-controller
A 3DS homebrew application that allows you to send input to a remote machine.

Functionality is currently experimental, and does not simulate input.

There is only support for Windows at this time. Input is sent as raw data, and does not (Currently) comply with network byte-order.

Basic data layout:
* circlePosition **{ int16_t, int16_t }**
* kDown **{ uint32_t }**
* kHeld **{ uint32_t }**
* kUp **{ uint32_t }**

Input data is sent over TCP.
