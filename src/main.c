#include <common/bitbase.h>
#include <common/unittest.h>
#include <graphics/renderer.h>
#include <graphics/window.h>


int main(int argc, const char **argv)
{
	init_bitbase_tables();

	/* TODO: add build option to add unittests - no need to be present in release versions of
	 * the program.
	 */
	if (argc == 2 && strcmp(argv[1], "--unittest") == 0) {
		do_unit_tests();
		return 0;
	}

	Window *window = init_window("Muon pre-alpha version");
	init_imgui(window);

	start_window_loop(window);

	destroy_imgui();
	destroy_window(window);
}

