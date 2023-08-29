//
// zd35_defs.hpp
//
// Created on: Aug 23, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_ZD35_ZD35_DEFS_HPP
#define COMPONENTS_ZD35_ZD35_DEFS_HPP

// Memory layout

enum {
	Zd35x2PageSize = 2176,  // unit: bytes
	Zd35x2BlockSize = 64 * Zd35x2PageSize,  // unit: bytes
	Zd35x2ChipId = 0x2C24,
	Zd35x2BlocksNumber = 1024 * 2,  // 2 panes x 1024 blocks
	Zd35x2CapacityBytes = Zd35x2BlocksNumber * Zd35x2BlockSize,
};

// Commands

enum Zd35Command {
	Zd35CommandGetFeatures = 0x0F,
};

// Registers

enum Zd35Address {
	Zd35AddressBlockLock = 0xA0,
};

// Bits

enum Zd35Register {
	Zd35RegisterBlockLockBrwdMask = 1 << 7,  // 8-bit BRWD register
	Zd35registerBlockLockBp3 = 1 << 6,
	Zd35registerBlockLockBp2 = 1 << 5,
	Zd35registerBlockLockBp1 = 1 << 4,
	Zd35registerBlockLockBp0 = 1 << 3,
	Zd35registerBlockLockTb = 1 << 2,
};

#endif // COMPONENTS_ZD35_ZD35_DEFS_HPP
