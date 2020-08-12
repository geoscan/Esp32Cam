#ifndef COMPONENTS_HTTP_CAMERADEMO_PAGES_H
#define COMPONENTS_HTTP_CAMERADEMO_PAGES_H

#include <esp_http_server.h>

#if !defined __cplusplus
# define __ext extern
#else
# define __ext
#endif

__ext const httpd_uri_t cameraDemo;

#undef __ext

#endif // COMPONENTS_HTTP_CAMERADEMO_CAMERADEMO_H
