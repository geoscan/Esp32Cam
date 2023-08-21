# ZD35

Driver for [ZD35 SPI flash memory](http://en.zettadevice.com/uploads/files/SPI%20NAND/2GC/1645752130ca09dfea53b1dbb0.pdf)

# MX35

ZD35 shares a great chunk of similarities w/
[MX35](https://www.macronix.com/Lists/Datasheet/Attachments/8198/MX35LF2G14AC,%203V,%202Gb,%20v1.1.pdf)
regarding SPI API. This is the reason why it uses `spi_flash_chip_mxic.c`
driver from ESP-IDF's `spi_flash` component.
