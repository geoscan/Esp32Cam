//
// zd35_defs.hpp
//
// Created on: Aug 23, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_ZD35_ZD35_DEFS_HPP
#define COMPONENTS_ZD35_ZD35_DEFS_HPP

enum {
	Zd35x2PageSize = 2176,
	Zd35x2BlockSize = 64 * Zd35x2PageSize,
	Zd35x2ChipId = 0x2C24,
	Zd35x2BlocksNumber = 1024 * 2,  // 2 panes x 1024 blocks
};

#endif // COMPONENTS_ZD35_ZD35_DEFS_HPP
