# Loguino
Libraries, scripts and manuals for the Loguino

1. Loguino

The Loguino is a low budget and open-source data acquisition device based on an Arduino Pro Mini designed for monitoring e.g. tree growth dynamics, climate or other environmental parameter at remote locations. It is capable of reading analog inputs as voltage signals at a resolution of 18-bit and any kind of digital sensors can be connected using standard protocols as I²C, SPI, 1-Wire or SDI-12. Logging interval can be set from some seconds to hours and data are stored as comma seperated .csv file to a 4GB (or 8GB) micro sd-card. While the Loguino is in a low power sleep mode in between logs, consuming only about 200µA, a Real-Time-Clock Module (RTC DS3231) keeps track of time and wakens the Loguino for the next log event. Interrupts can independently be counted without interrupting main routines, in order to monitor rainfall or windspeed. The Loguino board togheter with a standard Lithium-Ion 18650 akku (keeps the Loguino running for about a year) is mounted within a rugged enclosure (IP65) that can be attached to a tree or pole, for instance.

The eagle files for the PCB are available in this repository and all parts are available on the internet, however, ready-to-use (assembled, programmed and tested) Loguinos are for sale by sending a request to armin.niessner@taysira.org. All Arduino libraries and sketches needed for operation of the Loguino are available in this repository or at other git repositories (listed below). The Arduino software needed to program the Loguino is available at https://www.arduino.cc/.

2. Parts

3. Assembly

4. Sensors

5. Libraries

6. Troubleshooting

