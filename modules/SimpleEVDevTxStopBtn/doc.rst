.. _everest_modules_handwritten_SimpleEVDevTxStopBtn:

********************
SimpleEVDevTxStopBtn
********************

This module uses a Linux evdev interface to receive a "key or button press"
which in turn is used to gracefully stop an ongoing charging session by calling
the corresponding method of EvseManager.

The module uses the C library `libevdev` as external dependency.

By using the Linux kernel's `gpio-keys` driver, it is possible to create
an input event device which sends the expected (virtual) key presses.
Here is an example sniplet:

.. code-block:: dts

   &am33xx_pinmux {
       gpio_keys_pins: pinmux_gpio_keys {
           pinctrl-single,pins = <
               0x060 (PIN_INPUT_PULLUP | MUX_MODE7)  // GPIO1_28
           >;
       };
   };

   gpio_keys {
       compatible = "gpio-keys";
       pinctrl-names = "default";
       pinctrl-0 = <&gpio_keys_pins>;

       key-stop {
           label = "Stop Charging Button";
           linux,code = <KEY_STOP>;
           gpios = <&gpio1 28 GPIO_ACTIVE_LOW>;
           debounce-interval = <25>;
       };
   };



Module Configuration
====================

The EVerest module provides only two configuration options:

* ``device``: This is the device name to operate on, usually the full path to the
              device name, e.g. /dev/input/event1
* ``debug``: Just a debug helper to log a recognized event.
