#ifndef COMPONENTS_RTSP_SERVER_HPP
#define COMPONENTS_RTSP_SERVER_HPP

struct Server {
	virtual void run() = 0;
	virtual ~Server()
	{
	}
};

#endif // COMPONENTS_RTSP_SERVER_HPP
