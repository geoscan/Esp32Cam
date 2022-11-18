//
// sd_ftp.h
//
// Created on: Feb 11, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//
// sd_ftp module. Initializes SD card and mounts its FAT FS. Warning: the
// FAT FS must be present on the card.
//

#ifndef SDFTP_SDFTP_H
#define SDFTP_SDFTP_H

#ifdef __cplusplus
extern "C" {
#endif

bool sdFatInit();
bool sdFatDeinit();

#ifdef __cplusplus
}
#endif


#endif // SDFTP_SDFTP_H
