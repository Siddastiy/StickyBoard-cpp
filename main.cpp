#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>

struct Sticker {
	int id;
	double x = 0, y = 0;
	char buffer[1024] = "";
};

struct StickerWindow {
	GLFWwindow* window;
	ImGuiContext* imguiContext;
	Sticker* sticker;
	bool pinned = false;
	bool isDragging = false;
	double dragStartMouseX = 0, dragStartMouseY = 0;
	int dragStartWinX = 0, dragStartWinY = 0;
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void saveStickers(const std::vector<std::unique_ptr<Sticker>>& stickers);
void loadStickers(std::vector<std::unique_ptr<Sticker>>& stickers, GLFWwindow* window, std::vector<StickerWindow>& stickerWindows);
void deleteSticker(std::vector<std::unique_ptr<Sticker>>& stickers, int id);
StickerWindow createStickerWindow(Sticker* sticker, GLFWwindow* shareWindow);
void destroyStickerWindow(StickerWindow& sw);
std::string escapeJson(const std::string& s);
std::string unescapeJson(const std::string& s);
size_t findUnescapedQuote(const std::string& s, size_t start);

int main() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

	GLFWwindow* window = glfwCreateWindow(1, 1, "StickerMain", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetWindowPos(window, 0, 0);

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	float btn1Width = 30.0f;
	float btn2Width = 30.0f;
	float btn3Width = 15.0f;
	float btn4Width = 40.0f;
	int stickerToDelete = -1;
	bool addNewSticker = false;
	bool shouldExit = false;

	std::vector<StickerWindow> stickerWindows;
	std::vector<std::unique_ptr<Sticker>> stickers;
	loadStickers(stickers, window, stickerWindows);

	for (int i = 0; i < stickers.size(); i++) {
		Sticker* s = stickers[i].get();
		stickerWindows.push_back(createStickerWindow(s, window));
	}

	while (!glfwWindowShouldClose(window)) {
		glfwWaitEventsTimeout(1.0 / 30.0);

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		bool addNewSticker = false;
		int stickerToDelete = -1;

		for (auto& sw : stickerWindows) {
			if (glfwWindowShouldClose(sw.window)) continue;

			glfwMakeContextCurrent(sw.window);
			ImGui::SetCurrentContext(sw.imguiContext);

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			int id = sw.sticker->id;

			ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

			ImGui::Begin("StickerContent", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

			float y = ImGui::GetCursorPosY();
			float spacing = ImGui::GetStyle().ItemSpacing.x;
			float availWidth = ImGui::GetContentRegionAvail().x;


			ImGui::SetCursorPosX(0 + spacing);
			if (ImGui::Button("Exit", ImVec2(btn4Width, 0))) {
				shouldExit = true;
			}

			ImGui::SetCursorPosX(availWidth - btn1Width - btn2Width - btn3Width - spacing);
			ImGui::SetCursorPosY(y);

			if (ImGui::Button("Pin", ImVec2(btn1Width, 0))) {
				sw.pinned = !sw.pinned;
			}
			ImGui::SameLine();
			if (ImGui::Button("New", ImVec2(btn2Width, 0))) {
				addNewSticker = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("X", ImVec2(btn3Width, 0))) {
				stickerToDelete = id;
			}

			bool changed = ImGui::InputTextMultiline("##text", sw.sticker->buffer, sizeof(sw.sticker->buffer), ImVec2(-1, -1));

			bool hoveringEmptyArea = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered();

			if (hoveringEmptyArea && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				sw.isDragging = true;
				glfwGetCursorPos(sw.window, &sw.dragStartMouseX, &sw.dragStartMouseY);

				int winX, winY;
				glfwGetWindowPos(sw.window, &winX, &winY);
				sw.dragStartWinX = winX;
				sw.dragStartWinY = winY;
			}

			if (sw.isDragging) {
				if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					double curMouseX, curMouseY;
					glfwGetCursorPos(sw.window, &curMouseX, &curMouseY);

					int winX, winY;
					glfwGetWindowPos(sw.window, &winX, &winY);

					double deltaX = (winX + curMouseX) - (sw.dragStartWinX + sw.dragStartMouseX);
					double deltaY = (winY + curMouseY) - (sw.dragStartWinY + sw.dragStartMouseY);

					glfwSetWindowPos(sw.window, sw.dragStartWinX + (int)deltaX, sw.dragStartWinY + (int)deltaY);
				}
				else {
					sw.isDragging = false;

					int finalX, finalY;
					glfwGetWindowPos(sw.window, &finalX, &finalY);
					sw.sticker->x = (float)finalX;
					sw.sticker->y = (float)finalY;

					saveStickers(stickers);
				}
			}

			ImGui::End();

			if (changed) {
				saveStickers(stickers);
			}

			if (sw.pinned) {
				glfwSetWindowAttrib(sw.window, GLFW_FLOATING, GLFW_TRUE);
			} else if (!sw.pinned) {
				glfwSetWindowAttrib(sw.window, GLFW_FLOATING, GLFW_FALSE);
			}

			ImGui::Render();
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(sw.window);
		}

		if (stickerToDelete != -1) {
			auto it = std::find_if(stickerWindows.begin(), stickerWindows.end(), [&](const StickerWindow& sw) { 
				return sw.sticker->id == stickerToDelete; 
			});

			if (it != stickerWindows.end()) {
				destroyStickerWindow(*it);
				stickerWindows.erase(it);
			}

			deleteSticker(stickers, stickerToDelete);
			saveStickers(stickers);
		}

		if (addNewSticker) {
			int newId = 0;
			for (auto& sticker : stickers) {
				if (sticker->id > newId) newId = sticker->id;
			}
			newId++;

			auto newSticker = std::make_unique<Sticker>();
			newSticker->id = newId;

			Sticker* rawPtr = newSticker.get();
			stickers.push_back(std::move(newSticker));

			saveStickers(stickers);
			stickerWindows.push_back(createStickerWindow(rawPtr, window));
		}

		if (shouldExit) {
			break;
		}
	}
	saveStickers(stickers);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void saveStickers(const std::vector<std::unique_ptr<Sticker>>& stickers) {
	std::ofstream file("stickers.json");
	std::cout << "Saving to: " << std::filesystem::absolute("stickers.json") << std::endl;

	file << "[\n";
	for (int i = 0; i < stickers.size(); i++) {
		file << "{";
		file << "\"id\": " + std::to_string(stickers[i]->id) + ", ";
		file << "\"xPos\": " + std::to_string(stickers[i]->x) + ", ";
		file << "\"yPos\": " + std::to_string(stickers[i]->y) + ", ";
		file << "\"text\": \"" + escapeJson(stickers[i]->buffer) + "\"";		
		file << "}";
		if (i + 1 < stickers.size()) {
			file << ",\n";
		}
	}
	file << "\n]\n";

	std::cout << "Saving " << stickers.size() << " stickers\n";
}

void loadStickers(std::vector<std::unique_ptr<Sticker>>& stickers, GLFWwindow* window, std::vector<StickerWindow>& stickerWindows) {
	if (!std::filesystem::exists("stickers.json")) std::ofstream file("stickers.json");
	std::ifstream file("stickers.json");

	if (!file.is_open()) {
		std::cout << "Can not open file\n";
		return;
	}

	std::string content;
	std::getline(file, content, '\0');
	size_t objPos = 0;
	while (true) {
		size_t objStart = content.find("{", objPos);
		if (objStart == std::string::npos) break;

		size_t objEnd = content.find("}", objStart);

		std::string obj = content.substr(objStart, objEnd - objStart);

		size_t pos = obj.find("\"text\": \"");
		pos = pos + std::string("\"text\": \"").size();
		size_t end = findUnescapedQuote(obj, pos);
		std::string text = unescapeJson(obj.substr(pos, end - pos));

		pos = obj.find("\"id\": ");
		pos = pos + std::string("\"id\": ").size();
		end = obj.find(",", pos);
		int id = std::stoi(obj.substr(pos, end - pos));

		pos = obj.find("\"xPos\": ");
		pos = pos + std::string("\"xPos\": ").size();
		end = obj.find(",", pos);
		int xPos = std::stoi(obj.substr(pos, end - pos));

		pos = obj.find("\"yPos\": ");
		pos = pos + std::string("\"yPos\": ").size();
		end = obj.find(",", pos);
		int yPos = std::stoi(obj.substr(pos, end - pos));

		auto sticker = std::make_unique<Sticker>();
		sticker->id = id;
		sticker->x = xPos;
		sticker->y = yPos;
		strncpy_s(sticker->buffer, text.c_str(), sizeof(sticker->buffer) - 1);
		sticker->buffer[sizeof(sticker->buffer) - 1] = '\0';
		stickers.push_back(std::move(sticker));

		objPos = objEnd + 1;
	}
	std::cout << "Loaded " << stickers.size() << " stickers\n";
	if (stickers.size() == 0) {
		int newId = 0;
		for (auto& sticker : stickers) {
			if (sticker->id > newId) newId = sticker->id;
		}
		newId++;

		auto newSticker = std::make_unique<Sticker>();
		newSticker->id = newId;

		Sticker* rawPtr = newSticker.get();
		stickers.push_back(std::move(newSticker));

		saveStickers(stickers);
	}
}

void deleteSticker(std::vector<std::unique_ptr<Sticker>>& stickers, int id) {
	for (auto it = stickers.begin(); it != stickers.end(); it++) {
		if ((*it)->id == id) {
			stickers.erase(it);
			break;
		}
	}
}

StickerWindow createStickerWindow(Sticker* sticker, GLFWwindow* shareWindow) {
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

	GLFWwindow* win = glfwCreateWindow(250, 250, "Sticker", nullptr, shareWindow);
	glfwSetWindowPos(win, (int)sticker->x, (int)sticker->y);

	ImGuiContext* ctx = ImGui::CreateContext();
	ImGui::SetCurrentContext(ctx);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("fonts/Roboto-Regular.ttf", 14, nullptr, io.Fonts->GetGlyphRangesCyrillic());

	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowRounding = 12.0f;
	style.FrameRounding = 8.0f;
	style.GrabRounding = 8.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowPadding = ImVec2(12, 12);

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 0.6f, 1.0f);
	colors[ImGuiCol_Text] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	colors[ImGuiCol_Button] = ImVec4(0.961f, 0.816f, 0.2f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.981f, 0.836f, 0.3f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 0.6f, 1.0f);

	glfwMakeContextCurrent(win);
	ImGui_ImplGlfw_InitForOpenGL(win, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	return { win, ctx, sticker };
}

void destroyStickerWindow(StickerWindow& sw) {
	glfwMakeContextCurrent(sw.window);
	ImGui::SetCurrentContext(sw.imguiContext);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext(sw.imguiContext);

	glfwDestroyWindow(sw.window);
}

std::string escapeJson(const std::string& s) {
	std::string result;
	for (char c : s) {
		if (c == '"') result += "\\\"";
		else if (c == '\n') result += "\\n";
		else if (c == '\\') result += "\\\\";
		else result += c;
	}
	return result;
}

std::string unescapeJson(const std::string& s) {
	std::string result;
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] == '\\' && i + 1 < s.size()) {
			char next = s[i + 1];
			if (next == '"') { result += '"'; i++; }
			else if (next == 'n') { result += '\n'; i++; }
			else if (next == '\\') { result += '\\'; i++; }
			else result += s[i];
		}
		else {
			result += s[i];
		}
	}
	return result;
}

size_t findUnescapedQuote(const std::string& s, size_t start) {
	for (size_t i = start; i < s.size(); i++) {
		if (s[i] == '"' && (i == 0 || s[i - 1] != '\\')) {
			return i;
		}
	}
	return std::string::npos;
}
