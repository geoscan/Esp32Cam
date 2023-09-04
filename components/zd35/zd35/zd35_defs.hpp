//
// zd35_defs.hpp
//
// Created on: Aug 23, 2023
//     Author: Dmitry Murashov (dmtr <DOT> murashov <AT> <GMAIL> <DOT> <COM>)
//

#ifndef COMPONENTS_ZD35_ZD35_DEFS_HPP
#define COMPONENTS_ZD35_ZD35_DEFS_HPP

// Memory layout

enum Zd35x2 {
	// Service constants
	/// 2Gbit NAND flash memory
	Zd35x2ChipId = 0x2C24,

	// Memory layout constants

	/// unit: bytes. The actual size is 2048 + 128 (spare area), but the latter
	/// is not used for data storage.
	Zd35x2PageSize = 2048,

	/// Spare memory region is a part of the page that is not used for direct
	/// data storage, but rather contains service data such as bad block
	/// markers, and other stuff like that.
	Zd35x2SpareMemorySizeBytes = 128,

	Zd35x2CacheSizeBytes = Zd35x2PageSize,
	Zd35x2BlockSizePages = 64,
	Zd35x2BlockSizeBytes = Zd35x2BlockSizePages * Zd35x2PageSize,  // unit: bytes
	Zd35x2PaneSizeBlocks = 1024,
	Zd35x2PlaneSizeBytes = Zd35x2PaneSizeBlocks * Zd35x2BlockSizeBytes,

	// Capacity constants
	Zd35x2CapacityPanes = 2,
	Zd35x2CapacityBlocks = Zd35x2CapacityPanes * Zd35x2PaneSizeBlocks,  // 2 panes x 1024 blocks
	Zd35x2CapacityBytes = Zd35x2CapacityBlocks * Zd35x2BlockSizeBytes,
	Zd35x2CapacityPages = Zd35x2CapacityPanes * Zd35x2PaneSizeBlocks * Zd35x2BlockSizePages,

	Zd35x2FullPageSize = Zd35x2PageSize + Zd35x2SpareMemorySizeBytes,
};

// Commands

enum Zd35Command {
	Zd35CommandGetFeatures = 0x0F,
	Zd35CommandSetFeatures = 0x1F,
	Zd35CommandPageRead = 0x13,  // Read page into cache
	Zd35CommandReadFromCache = 0x03,
	Zd35CommandProgramLoad = 0x02,
	Zd35CommandProgramExecute = 0x10,
	Zd35CommandBlockErase = 0xD8,
};

// Registers

enum Zd35Address {
	Zd35AddressBlockLock = 0xA0,
	Zd35AddressStatus = 0xC0,
};

// Bits

enum Zd35Register {
	Zd35RegisterBlockLockBrwdMask = 1 << 7,  // 8-bit BRWD register
	Zd35registerBlockLockBp3 = 1 << 6,
	Zd35registerBlockLockBp2 = 1 << 5,
	Zd35registerBlockLockBp1 = 1 << 4,
	Zd35registerBlockLockBp0 = 1 << 3,
	Zd35registerBlockLockTb = 1 << 2,
	Zd35RegisterStatusWel = 1 << 1,  // WritE Enable
	Zd35RegisterStatusOip = 1 << 0,  // Operation in progress
};

#endif // COMPONENTS_ZD35_ZD35_DEFS_HPP
