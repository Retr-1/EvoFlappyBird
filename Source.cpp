#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <random>
#include <array>
#include <time.h>

float random() {
	// returns [0,1]
	return rand() / (float)RAND_MAX;
}

float sigmoid(float x) {
	return 1 / (1 + exp(-x));
}

struct Node {
	struct Connection {
		Node* node;
		float weight;
	};

	std::vector<Connection> connections;
	float value = 0;
	//should store activation function, but lets assume everywhere will be sigmoid

	Node(std::vector<Node*>& to_connect) {
		for (Node* node : to_connect) {
			connections.push_back({ node, random() - 0.5f });
		}
	}

	void evaluate() {
		double sum = 0;
		for (Connection& c : connections) {
			sum += c.node->value * c.weight;
		}
		value = sigmoid(sum);
	}
};

class Layer {
public:
	std::vector<Node*> nodes;
	virtual void evaluate() {};
};

class DenseLayer : public Layer {
public:
	DenseLayer(int count, Layer* prevLayer) {
		for (int i = 0; i < count; i++) {
			nodes.push_back(new Node(prevLayer->nodes));
		}
	}

	void evaluate() override {
		for (Node* node : nodes) {
			node->evaluate();
		}
	}
};


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
					weights[i][j].push_back(random()*2 - 0.5f);
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


};

// Override base class with your custom functionality
class Window : public olc::PixelGameEngine
{
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

int main()
{
	srand(time(0));
	
	std::vector<int> shape = { 2,3,1 };
	NeuralNetwork nn(shape);
	std::vector<float> input = { 0.5f, 0.2f };
	auto& output = nn.evaluate(input);
	

	Window win;
	if (win.Construct(256, 240, 4, 4))
		win.Start();
	return 0;
}