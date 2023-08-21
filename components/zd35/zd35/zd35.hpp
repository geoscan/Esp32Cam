//
// zd35.hpp
//
// Created: 2023-08-21
//  Author:
//

#ifndef COMPONENTS_ZD35_HPP_
#define COMPONENTS_ZD35_HPP_

namespace Zd35 {

void init();

/// \brief Checks MX35 presence on SPI bus. Due to siminrities w/ MX35 chip,
/// the SPI APIs are partially replaceable.
//
// In the case of a successful check, it will produce an output like the
// following one:
//
// ```
// E (2369) spi: spi_bus_initialize(756): SPI bus already initialized.
// I (2369) spi_flash: detected chip: generic
// I (2369) spi_flash: flash io: fastrd
// ```
//
// \pre it requires `#if !CONFIG_SPI_FLASH_OVERRIDE_CHIP_DRIVER_LIST` to be
// True due to how ESP-IDF's SPI flash driver is implemented. For more info on
// that, see `<espidf>/components/spi_flash (v4.1 through v5.2, maybe other
// ones too)`
void testAsMx35();

static inline const char *debugTag()
{
	return "[zd35]";
}

}  // namespace Zd35

#endif  // COMPONENTS_ZD35_HPP_
