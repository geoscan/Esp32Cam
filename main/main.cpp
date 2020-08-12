extern "C" void wifiStart();
extern "C" void httpStart();

extern "C" int app_main(void)
{
	wifiStart();
	httpStart();
	//        instantiate_demo_http();
	return 0;
}
