/* Allow our functions to be callable from our C codebase
 */
extern "C" {
#include "renderer.h"
#include "textures.h"
#include <common/gametree.h>
}

#include <stdio.h>
#include <stdint.h>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/backends/imgui_impl_sdlrenderer.h"


/* Initialise IMGUI context to begin renderering.
 */
void init_imgui(Window *window)
{
	/* Setup imgui */
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	/* Setup imgui sdl_renderer backend */
	ImGui_ImplSDL2_InitForSDLRenderer(window->frame, window->renderer);
	ImGui_ImplSDLRenderer_Init(window->renderer);
}


/* Cleanup IMGUI context. Again the OS will do this automatically for us when the program ends but
 * we might as well do it properly.
 */
void destroy_imgui()
{
	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}


/* Helper function to convert RGB literals to ImGui colour vectors, setting alpha to opaque.
 */
static inline
constexpr ImVec4 get_color_from_hex(uint32_t hex_color)
{
	uint32_t red   = (hex_color >> 16) & 0xFF;
	uint32_t green = (hex_color >>  8) & 0xFF;
	uint32_t blue  = (hex_color >>  0) & 0xFF;

	return ImVec4(red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f); 
}


static inline
void handle_clicked_square(Window *window, square sq)
{
	if (window->selected == SQUARE_NONE) {
		window->selected = sq;
		return;
	}

	square init = window->selected;
	square dest = sq;

	/* Rotate square back if it is black to move */
	if (!window->true_white)
		init ^= 56, dest ^= 56;

	/* TODO: add selection for promoted piece */
	move move = { init, dest, NONE, false };

	if (gametree_make_move(move))
		window->true_white = !window->true_white;

	window->selected = SQUARE_NONE;
}


/* Render the chess board itself. For now, this fills the entire window.
 */
static inline
void render_board(Window *window)
{
	/* At the moment the board fills the entire window */
	const auto board_size = ImGui::GetIO().DisplaySize;
	const auto square_size = ImVec2(board_size.x / 8.0f, board_size.y / 8.0f);

	/* Global settings for entire board */
	ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(board_size, ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

	const auto main_window_flags = ImGuiWindowFlags_NoTitleBar
	                             | ImGuiWindowFlags_NoResize
	                             | ImGuiWindowFlags_NoMove
	                             | ImGuiWindowFlags_NoScrollbar
	                             | ImGuiWindowFlags_NoCollapse;

	/* Start creating the main window */
	ImGui::Begin("main window", nullptr, main_window_flags);

	/* Default colors for chess board (stolen from lichess) */
	constexpr auto CLEAR = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	constexpr auto BOARD_BLACK = get_color_from_hex(0x8CA2AD);
	constexpr auto BOARD_WHITE = get_color_from_hex(0xDEE3E6);

	constexpr auto SELECTED_BLACK = get_color_from_hex(0x507c65);
	constexpr auto SELECTED_WHITE = get_color_from_hex(0x799c82);

	constexpr auto HIGHLIGHT_WHITE = get_color_from_hex(0xc3d887);
	constexpr auto HIGHLIGHT_BLACK = get_color_from_hex(0x93b166);


	static char name[3];

	for (square i = 0; i < 64; i += 1) {
		/* The square is the index with the rank flipped, as we render the board from top to
		 * bottom, whereas the squares are indexed bottom-up with A1 being 0.
		 */
		square sq = i ^ 56;
		square file = sq & 7;

		/* We also need a square for the side to move as we store rotated bitboards. If the
		 * board is from black's perspective, we need to flip the square to get the piece
		 * and colour information.
		 */
		square stm_square = sq ^ (!window->true_white * 56);

		if (file != 0)
			ImGui::SameLine(file * square_size.x); // render rank on same line

		/* Chessboard square is black if the evenness of the file is equal to the evenness
		 * of the rank, hence is white if not equal (xor) */
		bool white = (sq ^ (sq >> 3)) & 1;
		auto color = white ? BOARD_WHITE : BOARD_BLACK;

		/* The last move was generated from the other sides pov, so we always flip it */
		move *last_move = gametree_get_last_move();

		bool is_last_move = (last_move != nullptr)
		                 && ((stm_square ^ 56) == last_move->init
		                 ||  (stm_square ^ 56) == last_move->dest);

		if (is_last_move) color = white ? HIGHLIGHT_WHITE : HIGHLIGHT_BLACK;
		if (sq == window->selected) color = white ? SELECTED_WHITE : SELECTED_BLACK;

		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

		/* Prefix label name with ## to specifiy to IMGUI that it is an ID, and not label
		 * text */
		sprintf(name, "##%u", sq);

		Board board = gametree_get_board();
		piecetype piece = extract_piece(board, stm_square);

		/* Again, we need to take care in getting the true colour of a piece */
		bool friendly_piece = (board.white >> stm_square) & 1;
		bool white_piece = friendly_piece == window->true_white;

		size_t offset = white_piece ? WHITE_OFFSET : BLACK_OFFSET;

		ImTextureID texture = nullptr;
		if (piece != NONE) texture = load_texture(piece + offset, window);

		constexpr auto uv0 = ImVec2(0.0f, 0.0f);
		constexpr auto uv1 = ImVec2(1.0f, 1.0f);
		constexpr auto no_tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

		bool pressed = (piece == NONE)
			? ImGui::Button(name, square_size)
			: ImGui::ImageButton(name, texture, square_size, uv0, uv1, CLEAR, no_tint);

		if (pressed) handle_clicked_square(window, sq);
	}

	/* Remove styling for next object, TODO: find a better way to keep track of the number of
	 * pushed styles, or find a way to clear all of them
	 */
	ImGui::PopStyleVar(4);
	ImGui::PopStyleColor(192);
	ImGui::End();
}


void render_window(Window *window)
{
	/* Initialise a new IMGUI window frame to start drawing stuff too */
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	/* Render individual components: there will be more to follow! */
	render_board(window);

	/* Perform SDLRenderer draw calls to put stuff to the screen */
	ImGui::Render();
	ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
	SDL_RenderPresent(window->renderer);

}


/* Thin wrapper function to get SDL_Event from main window loop and hand it off to IMGUI. This will
 * buttons to respond to keypressed and other similar events.
 */
void renderer_handle_event(Window *window, SDL_Event *event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
	bool toggle = false;

	switch (event->type)
	{
		case SDL_KEYUP: switch (event->key.keysym.sym)
		{
			case SDLK_LEFT:  toggle = gametree_undo_move(); break;
			case SDLK_RIGHT: toggle = gametree_redo_move();  break;
		}
	}

	if (toggle)
		window->true_white = !window->true_white;
}

