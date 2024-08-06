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

	void draw(olc::PixelGameEngine* canvas, int x, int y) {
		const int nodeR = 10;
		const int layerGap = 60;
		const int nodeGap = 40;
		
		int biggest = 0;
		for (int s : shape) {
			biggest = std::max(biggest, s);
		}

		int maxHeight = biggest * nodeR * 2 + (biggest - 1) * nodeGap;
		std::vector<olc::vi2d> positions;

		int sx = x;
		for (int layer = 0; layer < shape.size(); layer++) {
			int height = shape[layer] * nodeR * 2 + (shape[layer] - 1) * nodeGap;
			int sy = y + (maxHeight - height) / 2;
			for (int n = 0; n < shape[layer]; n++) {
				canvas->FillCircle({ sx + nodeR, sy + nodeR }, nodeR, olc::GREY);
				positions.push_back(olc::vi2d(sx + nodeR, sy + nodeR));
				sy += nodeR * 2 + nodeGap;
			}

			sx += nodeR * 2 + layerGap;
		}

		int c = 0;
		for (int layer = 0; layer < shape.size()-1; layer++) {
			for (int n = 0; n < shape[layer]; n++) {
				for (int n2 = 0; n2 < shape[layer + 1]; n2++) {
					auto& positionA = positions[c + n];
					auto& positionB = positions[c + shape[layer] + n2];
					float weight = weights[layer][n][n2];
					float shade = (weight + 1) / 2 * 255;
					olc::Pixel color(shade,shade,shade);
					//std::cout << weight << ' ' << (weight + 1) / 2 * 255 <<' '<< (int)color.g << '\n';
					canvas->DrawLine(positionA, positionB, color);
				}
			}
			c += shape[layer];
		}
	}
};

class Bird {
	static const float thrust;
	static const float gravity;

	NeuralNetwork brain;

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
			v += thrust;
		}
	}

	void update(float elapsedTime) {
		v += gravity * elapsedTime;
		pos.y += v * elapsedTime;
	}

	void draw(olc::PixelGameEngine* canvas) {
		canvas->FillCircle(pos, r, olc::GREY);
	}

	void drawBrain(olc::PixelGameEngine* canvas, int x, int y) {
		brain.draw(canvas, x, y);
	}

	void mutate(float chance) {
		brain.mutate(chance);
	}

	Bird intercourse(const Bird& partner) {
		Bird child(pos.x, pos.y, brain.intercourse(partner.brain));
		return child;
	}
};
const float Bird::gravity = 1000;
const float Bird::thrust = -500;

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
	std::vector<int> brainShape = { 4,8,2 };
	std::vector<Obstacle> obstacles;
	float speed = 50;
	int obstacleGap = 300;
	int birdX = 50;
	int frameSkips = 1;
	bool should_draw = true;
	unsigned generation = 0;
	float genTime = 0;

	Bird& getWeightedSelection(float fitnessSum) {
		float chance = 0;
		float randval = random();
		for (Bird& b : birds) {
			chance += b.fitness / fitnessSum;
			if (randval <= chance) {
				return b;
			}
		}
		return birds[birds.size() - 1];
	}

	void makeNextGeneration() {
		generation++;
		genTime = 0;

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

		birds = nextGen;
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
		}
		while (obstacles.size() == 0 || obstacles[obstacles.size() - 1].pos.x < ScreenWidth()) {
			pushObstacle();
		}
	}

	void draw() {
		for (Bird& b : birds) {
			if (b.alive)
				b.draw(this);
		}
		for (Obstacle& o : obstacles) {
			o.draw(this);
		}
	}

	void drawStats() {
		std::string text = "Generation: " + std::to_string(generation) + "\nFrameSkips: " + std::to_string(frameSkips) + "\nGenBest: " + std::to_string(genTime);
		DrawString({ 10,10 }, text, olc::RED);
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

	bool OnUserUpdate(float ET) override
	{

		if (GetKey(olc::UP).bHeld) {
			frameSkips++;
			std::cout << "SKIPS: " << frameSkips << '\n';
		}
		if (GetKey(olc::DOWN).bHeld && frameSkips > 0) {
			frameSkips--;
			std::cout << "SKIPS: " << frameSkips << '\n';
		}
		if (GetKey(olc::D).bPressed) {
			if (should_draw)
				Clear(olc::BLACK);
			should_draw = !should_draw;
		}
		

		float elapsedTime = 0.016f;

		for (int f = 0; f < frameSkips; f++) {
			genTime += elapsedTime;

			updateObstacles(elapsedTime);

			int i = 0;
			while (obstacles[i].pos.x + obstacles[i].width < birdX - birds[0].r) {
				i++;
			}
			Obstacle& nearest = obstacles[i];

			bool allDead = true;
			for (Bird& b : birds) {
				if (!b.alive)
					continue;

				allDead = false;

				float distance = nearest.pos.x + nearest.width - (b.pos.x + b.r);
				distance /= ScreenWidth();
				float ybpos = b.pos.y / ScreenHeight();
				float yvel = b.v / (ScreenHeight() * 2);
				float yppos = nearest.pos.y / ScreenHeight() * 0.98f + 0.01;
				std::vector<float> input = { ybpos, yvel, distance, yppos };
				b.decide(input);
				b.update(elapsedTime);
				b.fitness += elapsedTime;
				if (nearest.is_colliding(b) || b.pos.y + b.r < 0 || b.pos.y - b.r > ScreenHeight()) {
					b.alive = false;
				}
			}

			if (allDead) {
				std::cout << "NEXT GENERATION, SCORE: " + std::to_string(genTime) + "\n";
				makeNextGeneration();
				obstacles.clear();
			}
		}

		if (should_draw) {
			Clear(olc::BLACK);
			draw();
			drawStats();
			birds[0].drawBrain(this, ScreenWidth()-200, 10);
		}

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