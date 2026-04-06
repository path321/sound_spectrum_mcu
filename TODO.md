# TODO

Check the following in order to keep track of tasks

## Device FW & HW
  - [ ] Optimize USB transmit
  - [ ] Add 'checksum' in frame / Ensure frame integrity
  - [ ] Average signal (?)
  - [ ] Clean up code
  - [ ] Measure duration of each process

## PC SW (plotting)
 - [ ] Detect COM port of HW board automatically via USB descriptors
 - [ ] Use Bar graphs instead of lines
 - [ ] Show error message in case device is not found
 - [ ] Add buttons for Run/Stop plotting

## Backlog

- [ ] Detect L/R orientation
- [ ] Change resistor of Green LED (too bright)
- [ ] Use MCU with larger Flash

## Notes for next version
- Add audio codec IC
- Use larger resistor for Green Led (too bright now)
- Use MCU with larger Flash

## Done
- [x] Demo firmware on STM32401RE-NUCLEO + ICS43434 board
- [x] Design HW board for STM32G473CBT
- [x] Plot data with Python real time-like
- [x] Show only >50Hz and <20.3kHz in X-axis