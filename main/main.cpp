extern "C" void init_wifi();
extern "C" void instantiate_demo_http();

extern "C" int app_main(void)
{
	init_wifi();
        instantiate_demo_http();
	return 0;
}
