.. _everest_modules_handwritten_GenericEVDevTokenProvider:

*************************
GenericEVDevTokenProvider
*************************

This module provides authentication tokens obtained from RFID cards using RFID readers
which are connected e.g. via USB and uses Linux's evdev interfaces.

In other words, such RFID readers appear as a virtual keyboard in the system.
When an RFID tag is presented, its UID appears in hexa-decimal format as
virtual key strokes, terminated by Enter key.

The module uses the C library `libevdev` as external dependency.

These readers does not provide any meta data of the RFID tokens, so
there is a type-guessing built in based on the length of the received UID.



Module Configuration
====================

The EVerest module provides only two configuration options:

* ``device``: This is the device name to operate on, usually the full path to the
              device name, e.g. /dev/input/event1
              But usually, you want to use a specific event device, so it would look like:
              /dev/input/by-id/usb-Miao-Xiao-Zhi_IC_ID_card_reader-event-kbd
* ``debug``: Just a debug helper to log a recognized RFID token when forwarding it.
