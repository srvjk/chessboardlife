#pragma once

#include "chessboardlife.h"
#include <sfml/Graphics.hpp>
#include <vector>

struct Agent::Private
{
	bool isFirstStep = true;
	int energy = 0;
	std::list<std::shared_ptr<Basis::Entity>> actions;
	int maxTimeFrames = 10;
	sf::Image visualImage;
};

struct MoveAction::Private
{
	Basis::Entity* object = nullptr;
	std::function<void()> moveFunction = nullptr;
};

struct Chessboard::Private
{
	int size = 0;
	float squareSize = 10;
	sf::Vector2f topLeft;
	std::vector<std::shared_ptr<ChessboardTypes::Square>> squares;
};

struct ChessboardLife::Private
{
	int boardSize = 16; /// размеры "шахматной доски"
};

struct ChessboardLifeViewer::Private
{
	std::unique_ptr<sf::RenderWindow> window = nullptr;
	std::shared_ptr<sf::RenderTexture> windowTexture = nullptr;
	std::unique_ptr<sf::RenderWindow> neighborhoodWindow = nullptr;
	std::unique_ptr<sf::RenderWindow> visionWindow = nullptr;
	std::unique_ptr<sf::RenderWindow> historyWindow = nullptr;
	std::unique_ptr<sf::RenderWindow> activeAgentInfoWindow = nullptr;
	sf::Font generalFont;
	std::string activeAgentName = "Agent";
	std::map<Basis::tid, sf::Color> entityColors;

	void drawChessboard(sf::RenderTarget* tgt);
	void drawHistory(sf::RenderTarget* tgt);
	void drawTimeFrame(sf::RenderTarget* tgt, std::shared_ptr<Entity> timeFrame, float left, float top, float width, float height);
	void drawAgentNeighborhood(sf::RenderTarget* tgt);
	void drawAgentVisualField(sf::RenderTarget* tgt);
	sf::Image getAgentVisualImage(int imgWidth, int imgHeight);
};
