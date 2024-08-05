#pragma once
#include <memory>
#include <vector>
#include "Random.h"

class Agent {
public:
	float fitness;
	virtual void mutate(float chance) {};
	virtual std::shared_ptr<Agent> intercourse(const std::shared_ptr<Agent>& partner) = 0;
};


template <typename Agent>
class Evolution {
protected:
	std::vector<std::shared_ptr<Agent>> agents;
	int nAgentsPerGen = 100;

	int getWeightedSelection(float sum) {
		float chance = 0;
		float randval = random();
		for (int i = 0; i < agents.size(); i++) {
			chance += agents[i]->fitness / sum;
			if (randval <= chance)
				return i;
		}
	}

	virtual void evaluate(float elapsedTime) {

	}

public:
	void make_next_generation() {
		float fitnessSum = 0;
		for (int i = 0; i < agents.size(); i++) {
			fitnessSum += agents[i]->fitness;
		}

		std::vector<std::shared_ptr<Agent>> nextGen;
		for (int i = 0; i < nAgentsPerGen; i++) {
			int a = getWeightedSelection(fitnessSum);
			int b = getWeightedSelection(fitnessSum);
			auto child = agents[a]->intercourse(agents[b]);
			nextGen.push_back(child);
		}

		agents = nextGen;
	}

	virtual void update(float elapsedTime) {
		evaluate(elapsedTime);
	}

	void addAgent(Agent& agent) {
		agents.push_back(std::make_shared<Agent>(agent));
	}
};

/*
class Birb : public Agent {
	int kk = 10000;
};

class EvoDerived : public Evolution {
	std::vector<Birb> furries;
	std::shared_ptr<Agent> createAgent() override {
		return std::make_shared<Birb>();
	}
};
*/