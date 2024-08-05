#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <array>
#include <time.h>
#include "Random.h"
#include "Evolution.h"


float sigmoid(float x) {
	return 1 / (1 + exp(-x));
}


class NeuralNetwork {
	// Dense Neural Network
	std::vector<std::vector<std::vector<float>>> weights;
	std::vector<int> shape;
	std::vector<std::vector<float>> values;

public:
	NeuralNetwork(std::vector<int>& shape) : shape(shape) {
		for (int i = 0; i < shape.size()-1; i++) {
			weights.push_back(std::vector<std::vector<float>>());
			for (int j = 0; j < shape[i]; j++) {
				weights[i].push_back(std::vector<float>());
				for (int k = 0; k < shape[i + 1]; k++) {
					weights[i][j].push_back(random2());
				}
			}
		}

		for (int i = 0; i < shape.size(); i++) {
			values.push_back(std::vector<float>());
			for (int j = 0; j < shape[i]; j++) {
				values[i].push_back(0.0f);
			}
		}
	}

	std::vector<float>& evaluate(std::vector<float>& input) {
		for (int i = 0; i < shape[0]; i++) {
			values[0][i] = input[i];
		}

		for (int layer = 1; layer < shape.size(); layer++) {
			for (int b = 0; b < shape[layer]; b++) {
				double sum = 0;
				for (int a = 0; a < shape[layer - 1]; a++) {
					sum += values[layer - 1][a] * weights[layer - 1][a][b];
				}
				values[layer][b] = sigmoid(sum);
			}
		}

		return values[shape.size() - 1];
	}

	void mutate(float chance) {
		const float lr = 0.2f;
		for (int i = 0; i < shape.size() - 1; i++) {
			for (int j = 0; j < shape[i]; j++) {
				for (int k = 0; k < shape[i + 1]; k++) {
					if (chance >= random()) {
						//Approach 1
						/*float target = random2();
						float delta = (target - weights[i][j][k]) * lr;
						weights[i][j][k] += delta;*/

						//Approach 2
						weights[i][j][k] += random2() * lr;
					}
				}
			}
		}
	}

	NeuralNetwork intercourse(NeuralNetwork& partner) {
		NeuralNetwork child(shape);
		for (int i = 0; i < shape.size() - 1; i++) {
			for (int j = 0; j < shape[i]; j++) {
				for (int k = 0; k < shape[i + 1]; k++) {
					if (random() > 0.5f) {
						child.weights[i][j][k] = weights[i][j][k];
					}
					else {
						child.weights[i][j][k] = partner.weights[i][j][k];
					}
				}
			}
		}
		return child;
	}
};


class Bird : public Agent {
	static const float thrust;
	static const float gravity;

	NeuralNetwork brain;

	void flap() {
		v.y += thrust;
	}

public:
	olc::vf2d pos;
	olc::vf2d v;
	float r = 20;
	

	Bird(std::vector<int>& brainShape) : brain(brainShape) {}

	void decideFlap(std::vector<float>& nnInput) {
		if (brain.evaluate(nnInput)[0] > 0.5f) {
			flap();
		}
	}

	void update(float elapsedTime) {
		v.y -= gravity * elapsedTime;
		pos.y += v.y * elapsedTime;
	}

	void draw(olc::PixelGameEngine* canvas) {
		canvas->FillCircle(pos, r, olc::GREY);
	}

	void mutate(float chance) override {
		brain.mutate(chance);
	}

	std::shared_ptr<Agent> intercourse(const std::shared_ptr<Agent>& partner) override {

	}
};
const float Bird::thrust = 10;
const float Bird::gravity = 10;

class Obstacle {
public:
	olc::vi2d pos;
	int gap = 30;
	int width = 30;

	void draw(olc::PixelGameEngine* canvas) {
		canvas->FillRect({ pos.x, 0 }, { width, pos.y - gap });
		canvas->FillRect({ pos.x, pos.y + gap }, { width, canvas->ScreenHeight() - (pos.y + gap) });
	}

	bool is_colliding(Bird& bird) {
		if (bird.pos.x + bird.r < pos.x || bird.pos.x-bird.r > pos.x+width)
			return false;

		return bird.pos.y - bird.r < pos.y - gap || bird.pos.y + bird.r > pos.y + gap;
	}
};

class FlappyBirdEvolution : public Evolution {
	std::vector<Obstacle> obstacles;
	std::vector<std::shared_ptr<Bird>> birds;

protected:

public:
	void draw(olc::PixelGameEngine* canvas) {
		for (auto& o : obstacles) {
			o.draw(canvas);
		}
		for (auto& b : birds) {
			b->draw(canvas);
		}
	}

	void update(float elapsedTime) override {
		for (auto& b : birds) {
			b->update(elapsedTime);
		}
	}
};

// Override base class with your custom functionality
class Window : public olc::PixelGameEngine
{
	const int nAgentsPerGen = 1000;
	std::vector<std::shared_ptr<Bird>> birds;

public:
	Window()
	{
		// Name your application
		sAppName = "Window";
	}

public:
	bool OnUserCreate() override
	{
		// Called once at the start, so create things here
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		// Called once per frame, draws random coloured pixels
		for (int x = 0; x < ScreenWidth(); x++)
			for (int y = 0; y < ScreenHeight(); y++)
				Draw(x, y, olc::Pixel(rand() % 256, rand() % 256, rand() % 256));
		return true;
	}
};




//class TestBase {
//public:
//	std::string sayHello() {
//		return "yeahhh...";
//	}
//};
//
//class TestDerived : public TestBase {
//public:
//	int sayHello() {
//		return 69;
//	}
//};

int main()
{
	srand(time(0));

	//TestDerived td;
	//TestDerived* tdp = &td;
	//TestBase* tbp = &td;
	//tdp->sayHello();
	//tbp->sayHello();
	
	std::vector<int> shape = { 2,3,1 };
	NeuralNetwork nn(shape);
	std::vector<float> input = { 0.5f, 0.2f };
	auto& output = nn.evaluate(input);

	Window win;
	if (win.Construct(256, 240, 4, 4))
		win.Start();
	return 0;
}