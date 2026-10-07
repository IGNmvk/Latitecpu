#pragma once
#include "client/feature/module/HUDModule.h"

class BetterHunger : public Module {
public:
	BetterHunger();
private:
	void onRenderLayer(Event& ev);

	ValueType satColor = ColorValue(1.f, 0.73f, 0.f, 1.f);
	ValueType showPredicted = BoolValue(true);
	ValueType fadeSpeed = FloatValue(7.f);
	ValueType xOffset = FloatValue(82.f);
	ValueType yOffset = FloatValue(44.f);
};
