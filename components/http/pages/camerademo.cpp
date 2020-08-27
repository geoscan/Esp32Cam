#include <esp_err.h>
#include <esp_http_server.h>

#include "pages.h"
#include "Ov2640.hpp"

extern "C" esp_err_t cameraDemoHandler(httpd_req_t *req)
{
	esp_err_t res;

	res = httpd_resp_set_type(req, "image/jpeg");
	if (res == ESP_OK) {
		httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
	}

	auto img = Ov2640::instance().jpeg();

	if (res == ESP_OK && img->valid()) {
//		res = httpd_resp_send(req, static_cast<const char *>(img.getData().data), img.getData().len);
		res = httpd_resp_send(req, static_cast<const char *>(img->data().data()), img->data().size());
	}

	return res;

}



