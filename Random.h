#pragma once
#include <random>

// returns [0,1]
float random() {
	return rand() / (float)RAND_MAX;
}
// returns [-1,1]
float random2() {
	return random() * 2 - 1.0f;
}