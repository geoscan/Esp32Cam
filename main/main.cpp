extern "C" void init_wifi();

extern "C" int app_main(void)
{
	init_wifi();
	return 0;
}
