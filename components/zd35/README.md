# ZD35

Driver for [ZD35 SPI flash memory](http://en.zettadevice.com/uploads/files/SPI%20NAND/2GC/1645752130ca09dfea53b1dbb0.pdf)

The driver is rather tightly integrated w/ ESP IDF Flash SPI API, especially its "chip generic" part, and makes calls
to functions of that module where possible

## Support for ESP-IDF flash API

- [x] Write (per page, otherwise an error will be produced)
- [x] Read (per page, otherwise an error will be produced)
- [x] Erase block (per block)
- [ ] Erase chip
- [ ] Get chip size

## Supported chips

- ZD35Q2GC

It MAY be at least partially compatible w/ other Zetta SPI memory chips.

# MX35

ZD35 shares a great chunk of similarities w/
[MX35](https://www.macronix.com/Lists/Datasheet/Attachments/8198/MX35LF2G14AC,%203V,%202Gb,%20v1.1.pdf)
regarding its SPI API.
