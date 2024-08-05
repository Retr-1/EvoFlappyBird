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

	NeuralNetwork intercourse(const NeuralNetwork& partner) {
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

class Bird {
	static const float thrust;
	static const float gravity;

	NeuralNetwork brain;

	void flap() {
		v += thrust;
	}

public:
	olc::vf2d pos;
	float v = 0;
	float r = 20;
	bool alive = true;
	float fitness = 0;

	Bird(float x, float y, std::vector<int>& brainShape) : brain(brainShape), pos(x,y) {}
	Bird(float x, float y, const NeuralNetwork& nn) : brain(nn), pos(x,y) {}

	void decide(std::vector<float>& nnInput) {
		if (brain.evaluate(nnInput)[0] > 0.5f) {
			flap();
		}
	}

	void update(float elapsedTime) {
		v -= gravity * elapsedTime;
		pos.y += v * elapsedTime;
	}

	void draw(olc::PixelGameEngine* canvas) {
		canvas->FillCircle(pos, r, olc::GREY);
	}

	void mutate(float chance) {
		brain.mutate(chance);
	}

	Bird intercourse(const Bird& partner) {
		Bird child(pos.x, pos.y, brain.intercourse(partner.brain));
		return child;
	}
};
const float Bird::gravity = -1000;
const float Bird::thrust = 50;

class Obstacle {
public:
	olc::vf2d pos;
	int gap;
	int width;

	Obstacle(float x = 0, float y = 0, int gap = 30, int width = 30) : pos(x, y), gap(gap), width(width) {}

	void draw(olc::PixelGameEngine* canvas) {
		canvas->FillRect(olc::vi2d( pos.x, 0 ), olc::vd2d( width, pos.y - gap ));
		canvas->FillRect(olc::vi2d( pos.x, pos.y + gap ), olc::vd2d( width, canvas->ScreenHeight() - (pos.y + gap) ));
	}

	bool is_colliding(Bird& bird) {
		if (bird.pos.x + bird.r < pos.x || bird.pos.x-bird.r > pos.x+width)
			return false;

		return bird.pos.y - bird.r < pos.y - gap || bird.pos.y + bird.r > pos.y + gap;
	}
};



// Override base class with your custom functionality
class Window : public olc::PixelGameEngine
{
	const int nAgentsPerGen = 100;
	std::vector<Bird> birds;
	std::vector<int> brainShape = { 3,6,2 };
	std::vector<Obstacle> obstacles;
	float speed = 50;
	int obstacleGap = 300;
	int birdX = 50;

	Bird& getWeightedSelection(float fitnessSum) {
		float chance = 0;
		float randval = random();
		for (Bird& b : birds) {
			chance += b.fitness / fitnessSum;
			if (randval <= chance) {
				return b;
			}
		}
	}

	void makeNextGeneration() {
		float fitnessSum = 0;
		for (Bird& b : birds) {
			fitnessSum += b.fitness;
		}

		std::vector<Bird> nextGen;
		for (int i = 0; i < nAgentsPerGen; i++) {
			Bird& parent1 = getWeightedSelection(fitnessSum);
			Bird& parent2 = getWeightedSelection(fitnessSum);
			auto child = parent1.intercourse(parent2);
			child.pos.y = ScreenHeight() / 2;
			nextGen.emplace_back(child);
		}
	}

	void pushObstacle() {
		const int verGap = 50;
		const int width = 30;
		const int start = 200;
		if (obstacles.size() == 0) {
			obstacles.emplace_back(Obstacle(start, randint(verGap / 2, ScreenHeight() - verGap / 2), verGap, width));
		}
		else {
			Obstacle& o = obstacles[obstacles.size() - 1];
			obstacles.emplace_back(Obstacle(o.pos.x + o.width + obstacleGap, randint(verGap / 2, ScreenHeight() - verGap / 2), verGap, width));
		}
	}

	void updateObstacles(float elapsedTime) {
		for (Obstacle& o : obstacles) {
			o.pos.x -= speed * elapsedTime;
		}
		while (obstacles.size() > 0 && obstacles[0].pos.x+obstacles[0].width < 0) {
			obstacles.erase(obstacles.begin());
			std::cout << "ERASED\n";
		}
		while (obstacles.size() == 0 || obstacles[obstacles.size() - 1].pos.x < ScreenWidth()) {
			pushObstacle();
			std::cout << "CRATED\n";
		}
	}

	void draw() {
		for (Bird& b : birds) {
			b.draw(this);
		}
		for (Obstacle& o : obstacles) {
			o.draw(this);
		}
	}

public:
	Window()
	{
		// Name your application
		sAppName = "Window";
	}

public:
	bool OnUserCreate() override
	{
		for (int i = 0; i < nAgentsPerGen; i++) {
			birds.emplace_back(Bird(birdX, ScreenHeight() / 2, brainShape));
		}

		return true;
	}

	bool OnUserUpdate(float elapsedTime) override
	{
		Clear(olc::BLACK);

		updateObstacles(elapsedTime);
		for (Bird& b : birds) {
			b.update(elapsedTime);
		}

		draw();

		return true;
	}
};

int main()
{
	srand(time(0));
	
	//std::vector<int> shape = { 2,3,1 };
	//NeuralNetwork nn(shape);
	//std::vector<float> input = { 0.5f, 0.2f };
	//auto& output = nn.evaluate(input);

	Window win;
	if (win.Construct(1000, 600, 1, 1))
		win.Start();
	return 0;
}