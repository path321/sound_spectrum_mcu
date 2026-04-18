# TODO
Check the following in order to keep track of tasks

## Device FW & HW
  - [ ] Optimize USB transmit
  - [ ] Measure duration of each process
  - [ ] Display Average value

## PC SW (plotting)
 - [ ] Use Bar graphs instead of lines

## Backlog
- [ ] Redesign USB lines, as they are unstable
- [ ] Detect L/R orientation of larger frequency
- [ ] Add 'checksum' in frame / Ensure frame integrity

## Notes for next version
- Redesign USB lines, as they are unstable
- Add audio codec IC
- Use larger resistor for Green Led (too bright now)
- Use MCU with larger Flash

## Done
- [x] Demo firmware on STM32401RE-NUCLEO + ICS43434 board
- [x] Design HW board for STM32G473CBT
- [x] Plot data with Python real time-like
- [x] Show only >50Hz and <20.3kHz in X-axis
- [x] Average signal between transmit
- [X] Show error message in case device is not found
- [X] Add buttons for Run/Stop plotting
- [X] Code restructure with PySide
- [X] Detect COM port of HW board automatically via USB descriptors (need improving)
