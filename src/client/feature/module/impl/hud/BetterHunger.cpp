#include "pch.h"
#include "BetterHunger.h"
#include "client/event/impl/RenderLayerEvent.h"
#include "sdk/common/client/gui/controls/VisualTree.h"
#include "sdk/common/client/gui/controls/UIControl.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>

namespace {
	struct Food { int hunger; float saturation; };

	// food values by item id (same numbers vanilla uses)
	const std::map<std::string, Food> foods = {
		{"apple", {4, 2.4f}}, {"baked_potato", {5, 6.f}}, {"beetroot", {1, 1.2f}},
		{"beetroot_soup", {6, 7.2f}}, {"bread", {5, 6.f}}, {"carrot", {3, 3.6f}},
		{"chorus_fruit", {4, 2.4f}}, {"cooked_beef", {8, 12.8f}}, {"cooked_chicken", {6, 7.2f}},
		{"cooked_cod", {5, 6.f}}, {"cooked_mutton", {6, 9.6f}}, {"cooked_porkchop", {8, 12.8f}},
		{"cooked_rabbit", {5, 6.f}}, {"cooked_salmon", {6, 9.6f}}, {"cookie", {2, 0.4f}},
		{"dried_kelp", {1, 0.6f}}, {"enchanted_golden_apple", {4, 9.6f}}, {"glow_berries", {2, 0.4f}},
		{"golden_apple", {4, 9.6f}}, {"golden_carrot", {6, 14.4f}}, {"honey_bottle", {6, 1.2f}},
		{"melon_slice", {2, 1.2f}}, {"mushroom_stew", {6, 7.2f}}, {"poisonous_potato", {2, 1.2f}},
		{"potato", {1, 0.6f}}, {"pufferfish", {1, 0.2f}}, {"pumpkin_pie", {8, 4.8f}},
		{"rabbit_stew", {10, 12.f}}, {"beef", {3, 1.8f}}, {"chicken", {2, 1.2f}},
		{"cod", {2, 0.4f}}, {"mutton", {2, 1.2f}}, {"porkchop", {3, 1.8f}},
		{"rabbit", {3, 1.8f}}, {"salmon", {2, 0.4f}}, {"rotten_flesh", {4, 0.8f}},
		{"spider_eye", {2, 3.2f}}, {"suspicious_stew", {6, 7.2f}}, {"sweet_berries", {2, 0.4f}},
		{"tropical_fish", {1, 0.2f}}
	};
}

BetterHunger::BetterHunger() : Module("BetterHunger", L"Better Hunger", L"Shows your saturation and what held food will give you", HUD, nokeybind) {
	this->listen<RenderLayerEvent>(&BetterHunger::onRenderLayer);
	addSetting("satColor", L"Saturation color", L"", satColor);
	addSetting("showPredicted", L"Show predicted saturation", L"Pulses the saturation the held food would give", showPredicted);
	addSliderSetting("fadeSpeed", L"Pulse speed", L"", fadeSpeed, FloatValue(1.f), FloatValue(15.f), FloatValue(1.f));
	addSliderSetting("xOffset", L"X offset", L"Move the bar sideways if it doesn't line up", xOffset, FloatValue(0.f), FloatValue(200.f), FloatValue(1.f));
	addSliderSetting("yOffset", L"Y offset", L"Move the bar up/down if it doesn't line up", yOffset, FloatValue(0.f), FloatValue(100.f), FloatValue(1.f));
}

void BetterHunger::onRenderLayer(Event& evG) {
	RenderLayerEvent& ev = reinterpret_cast<RenderLayerEvent&>(evG);
	SDK::ScreenView* screenView = ev.getScreenView();

	auto ci = SDK::ClientInstance::get();
	auto plr = ci->getLocalPlayer();
	if (!plr) return;
	if (screenView->visualTree->rootControl->name != "hud_screen") return;

	float hunger = plr->getHunger();
	float sat = std::floor(plr->getSaturation());

	// predicted values from the held food
	float predicted = 0.f;
	if (std::get<BoolValue>(showPredicted) && plr->supplies && plr->supplies->inventory) {
		auto stack = plr->supplies->inventory->getItem(plr->supplies->selectedSlot);
		if (stack && stack->item && *stack->item) {
			auto it = foods.find((*stack->item)->id.getString());
			if (it != foods.end()) {
				predicted = std::floor(std::min(sat + it->second.saturation, hunger + static_cast<float>(it->second.hunger)));
				predicted = std::min(predicted, 20.f);
			}
		}
	}

	if (sat <= 0.f && predicted <= 0.f) return;

	auto gui = ci->getGuiData();
	float gs = gui->guiScale;
	Vec2 screen = gui->screenSize;
	float baseX = screen.x / 2.f + static_cast<float>(std::get<FloatValue>(xOffset)) * gs;
	float baseY = screen.y - static_cast<float>(std::get<FloatValue>(yOffset)) * gs;

	StoredColor c = std::get<ColorValue>(satColor).getMainColor();

	double t = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
	float pulse = 0.5f * (1.f + static_cast<float>(std::sin(t * static_cast<float>(std::get<FloatValue>(fadeSpeed)))));

	MCDrawUtil dc{ ev.getUIRenderContext(), ci->minecraftGame->minecraftFont };

	// one slim bar above each hunger icon (drawn right to left like the icons),
	// so nothing depends on the texture pack
	for (int i = 0; i < 10; i++) {
		float iconLeft = baseX - static_cast<float>(i) * 8.f * gs;
		float segRight = iconLeft + 8.f * gs;
		float top = baseY;
		float bottom = baseY + 2.f * gs;

		float predFill = std::clamp(predicted - 2.f * static_cast<float>(i), 0.f, 2.f) / 2.f;
		float curFill = std::clamp(sat - 2.f * static_cast<float>(i), 0.f, 2.f) / 2.f;

		if (predFill > curFill) {
			dc.fillRectangle({ segRight - 7.f * gs * predFill, top, segRight, bottom }, d2d::Color(c.r, c.g, c.b, 0.2f + 0.6f * pulse));
		}
		if (curFill > 0.f) {
			dc.fillRectangle({ segRight - 7.f * gs * curFill, top, segRight, bottom }, d2d::Color(c.r, c.g, c.b, 1.f));
		}
	}
}
