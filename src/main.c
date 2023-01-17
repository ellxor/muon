#include <common/bitbase.h>
#include <graphics/renderer.h>
#include <graphics/window.h>
#include <stdio.h>

int main()
{
	init_bitbase_tables();


	Window *window = init_window("Muon pre-alpha version");
	init_imgui(window);

	start_window_loop(window);

	destroy_imgui();
	destroy_window(window);
}

