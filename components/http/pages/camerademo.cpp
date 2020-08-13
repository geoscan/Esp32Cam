#include <esp_err.h>
#include <esp_http_server.h>

#include "pages.h"
#include "Ov2640.hpp"

esp_err_t cameraDemoHandler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "image/jpeg");
	httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

	Image jpg = Ov2640::instance().jpeg();

	if (jpg.isValid()) {
		Image::Data jpgData = jpg.data();
		return httpd_resp_send(req, reinterpret_cast<const char *>(jpgData.data), jpgData.len);
	}

	return httpd_resp_send(req, nullptr, 0);
}
