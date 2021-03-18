//
// camera_recorder.h
//
// Created on: Mar 22, 2021
//     Author: Dmitry Murashov (d.murashov@geoscan.aero)
//

#ifndef CAMERA_RECORDER_CAMERA_RECORDER_H
#define CAMERA_RECORDER_CAMERA_RECORDER_H

#ifdef __cplusplus
# define C_EXTERN extern "C"
#else
# define C_EXTERN extern
#endif

C_EXTERN void cameraRecorderInit();

#undef C_EXTERN

#endif  // CAMERA_RECORDER_CAMERA_RECORDER_H
