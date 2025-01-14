.. _everest_modules_handwritten_SimpleSwitch3ph1phAPI:

*******************************************
SimpleSwitch3ph1phAPI
*******************************************

This module is similar to the existing 'API' module in EVerest core.

It acts as a replacement for both the 'EnergyManager' and 'EnergyNode'
combination. The main use-case is when you have an external energy
management system and want to control the count of phases and the
current directly, i.e. without any EVerest energy magic.
For this, this module uses the 'energy' interface of the linked
'EvseManager' instance.

Example configuration sniplet:

.. code-block:: yaml

   ...
   switch3ph1phctrl:
     module: SimpleSwitch3ph1phAPI
     connections:
       energy:
         - module_id: connector
           implementation_id: energy_grid
   ...


This module offers a single MQTT command which allows to set a limit
in ampere in combination with a phase count:

.. code-block:: json
   :caption: Topic: ``everest_api/evse_manager/cmd/set_limit_amps``

   {"ac_max_current_A":13,"ac_max_phase_count":1}

A proper BSP module configuration is recommended, i.e. the module should
publish reasonable HW capabilities with a maximum current value - otherwise
there may not exist any - this module does not limit - it just forwards.

:ref:`Link <everest_modules_SimpleSwitch3ph1phAPI>` to the module's reference.
Simple MQTT API for phase count switching setups (without EnergyManager)
