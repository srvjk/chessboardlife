#include "chessboardlife.h"
#include <iostream>
#include <sfml/Window.hpp>
#include <sfml/Graphics.hpp>

using namespace std;

static const string ChessboardName = "Chessboard";
static const string NeighborhoodSensorName = "NeighborhoodSensor";

struct Agent::Private
{
	bool isFirstStep = true;
	int energy = 0;
	std::list<std::shared_ptr<Basis::Entity>> actions;
	int maxTimeFrames = 10;
};

Agent::Agent(Basis::System* s) :
	Basis::Entity(s),
	_p(std::make_unique<Private>())
{
	auto spt = addFacet<Basis::Spatial>();
}

void Agent::makeActions()
{
	int n = 5;

	for (int i = 0; i < n; ++i) {
		int v = system()->randomInt(0, 4);
		std::shared_ptr<Entity> newEnt = nullptr;
		switch (v) {
		case 0:
			newEnt = system()->newEntity<StandByAction>();
			break;
		case 1:
			newEnt = system()->newEntity<MoveNorthAction>();
			break;
		case 2:
			newEnt = system()->newEntity<MoveSouthAction>();
			break;
		case 3:
			newEnt = system()->newEntity<MoveEastAction>();
			break;
		case 4:
			newEnt = system()->newEntity<MoveWestAction>();
			break;
		}

		if (newEnt) {
			auto mv = newEnt->as<MoveAction>();
			mv->setObject(this);
			_p->actions.push_back(newEnt);
		}
	}
}

void Agent::memorize(std::shared_ptr<Basis::Entity> ent)
{
	// �������� ent (������ ��� ������� ��� ��������) ����������� � ������� ��������� �����
	// �������, ���������� � ������.
	shared_ptr<Basis::Container> cont = Basis::toSingle<Basis::Container>(findEntitiesByName("History"));
	if (!cont)
		return;

	shared_ptr<Entity> timeFrameEnt = cont->lastItem();
	cont = timeFrameEnt->as<Basis::Container>();
	if (cont)
		cont->addItem(ent);
}

int64_t Agent::maxTimeFrames() const
{
	return _p->maxTimeFrames;
}

void Agent::constructHelpers()
{
	shared_ptr<Entity> history = newEntity<Entity>();
	history->setName("History");
	history->addFacet<Basis::Container>();

	shared_ptr<Entity> neighborhoodSensor = newEntity<NeighborhoodSensor>();
	neighborhoodSensor->setName(NeighborhoodSensorName);
}

void Agent::step()
{
	if (_p->isFirstStep)
		constructHelpers();

	shared_ptr<Basis::Container> histCont = Basis::toSingle<Basis::Container>(findEntitiesByName("History"));
	if (!histCont)
		return;

	// ������ ����� ��������� �����, ��� ������������� ������ ����� ������
	shared_ptr<Entity> timeFrame = newEntity<Entity>();
	timeFrame->addFacet<Basis::Container>();
	histCont->addItem(timeFrame);
	if (histCont->size() > _p->maxTimeFrames) {
		histCont->popFront();
	}

	// �������������, ��������� ����������, ��������� �������...
	{
		auto spt = this->as<Basis::Spatial>();
		Basis::point3d pos = spt->position();
		int x = (int)pos.get<0>();
		int y = (int)pos.get<1>();

		for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
			auto stone = iter.value()->as<Stone>();
			if (stone) {
				auto stoneSpt = stone->as<Basis::Spatial>();
				Basis::point3d stonePos = stoneSpt->position();
				int stoneX = (int)stonePos.get<0>();
				int stoneY = (int)stonePos.get<1>();

				if (x == stoneX && y == stoneY) {
					auto collisionEvent = newEntity<Basis::Entity>();
					collisionEvent->setName("Collision");
					memorize(collisionEvent);
				}
			}
		}

		auto vision = Basis::toSingle<NeighborhoodSensor>(findEntitiesByName(NeighborhoodSensorName));
		if (vision) {
			vision->step();
		}
	}

	// �������� ��������� �������� �� �������;
	// ���� ������� ��������, ��������� �����.
	if (_p->actions.empty()) {
		makeActions();
	}
	if (_p->actions.empty())
		return;

	// ��������� ��������;
	// ������ ����������� �������� �������� � �������
	auto act = _p->actions.front();
	_p->actions.pop_front();
	auto moveAct = act->as<MoveAction>();
	if (moveAct) {
		moveAct->step();
	}
	system()->removeEntity(act->id());

	memorize(act);
}

int Agent::energy() const
{
	return _p->energy;
}

void Agent::setEnergy(int e)
{
	_p->energy = e;
}

struct ChessboardTypes::Image 
{
	Image(int w, int h)
	{
		width = w;
		height = h;
		int length = width * height;
		if (length > 0)
			pixels = std::vector<sf::Color>(length, sf::Color(0, 0, 0));
	}

	int width = 0;
	int height = 0;
	std::vector<sf::Color> pixels;
};

struct NeighborhoodSensor::Private
{
	std::shared_ptr<ChessboardTypes::Image> neighborhoodImage;
};

NeighborhoodSensor::NeighborhoodSensor(Basis::System* s) :
	Basis::Entity(s),
	_p(std::make_unique<Private>())
{
}

void NeighborhoodSensor::step()
{
	Basis::Spatial* spt = firstSuchParent<Basis::Spatial>();
	if (!spt)
		return;

	Basis::point3d pos = spt->position();
	int x = (int)pos.get<0>();
	int y = (int)pos.get<1>();
	auto chessboard = Basis::toSingle<Chessboard>(system()->findEntitiesByName(ChessboardName));
	if (!chessboard)
		return;

	if (!_p->neighborhoodImage) {
		_p->neighborhoodImage = std::make_shared<ChessboardTypes::Image>(3, 3);
	}

	//chessboard->getImage(x - 1, y - 1, _p->neighborhoodImage.get());

	shared_ptr<ChessboardLifeViewer> viewer = nullptr;
	for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
		auto ent = iter.value()->as<ChessboardLifeViewer>();
		if (ent) {
			viewer = ent;
			break;
		}
	}
	if (!viewer)
		return;

	viewer->getImage(x - 1, y - 1, _p->neighborhoodImage.get());
}

std::tuple<int, int> NeighborhoodSensor::getSize() const
{
	if (!_p->neighborhoodImage)
		return std::make_tuple(0, 0);

	return std::make_tuple(_p->neighborhoodImage->width, _p->neighborhoodImage->height);
}

std::tuple<int, int, int> NeighborhoodSensor::getPixel(int x, int y) const
{
	if (!_p->neighborhoodImage)
		return std::make_tuple(0, 0, 0);

	if (x < 0 || x >= _p->neighborhoodImage->width)
		return std::make_tuple(0, 0, 0);
	if (y < 0 || y >= _p->neighborhoodImage->height)
		return std::make_tuple(0, 0, 0);

	sf::Color c = _p->neighborhoodImage->pixels.at((size_t)(y * _p->neighborhoodImage->width) + x);
	return std::make_tuple(c.r, c.g, c.b);
}

struct MoveAction::Private 
{
	Basis::Entity* object = nullptr;
	std::function<void()> moveFunction = nullptr;
};

MoveAction::MoveAction(Basis::System* s) :
	Basis::Entity(s),
	_p(std::make_unique<Private>())
{
}

void MoveAction::setObject(Basis::Entity* ent)
{
	_p->object = ent;
}

Basis::Entity* MoveAction::getObject() const
{
	return _p->object;
}

void MoveAction::step()
{
	if (_p->moveFunction)
		_p->moveFunction();
}

void MoveAction::setMoveFunction(std::function<void()> func)
{
	_p->moveFunction = func;
}

StandByAction::StandByAction(Basis::System* s) :
	Basis::Entity(s)
{
	addFacet<MoveAction>();
}

MoveNorthAction::MoveNorthAction(Basis::System* s) :
	Basis::Entity(s)
{
	auto mov = addFacet<MoveAction>();
	if (mov)
		mov->setMoveFunction(std::bind(&MoveNorthAction::moveFunction, this));
}

void MoveNorthAction::moveFunction()
{
	auto mov = this->as<MoveAction>();
	if (!mov)
		return;

	Basis::Entity* obj = mov->getObject();
	if (!obj)
		return;

	auto spt = obj->as<Basis::Spatial>();
	if (!spt)
		return;

	Basis::point3d p = spt->position();
	double y = p.get<1>();
	if (y >= 1.0) {
		p.set<1>(y - 1.0);
		spt->setPosition(p);
	}
}

MoveSouthAction::MoveSouthAction(Basis::System* s) :
	Basis::Entity(s)
{
	auto mov = addFacet<MoveAction>();
	if (mov)
		mov->setMoveFunction(std::bind(&MoveSouthAction::moveFunction, this));
}

void MoveSouthAction::moveFunction()
{
	auto mov = this->as<MoveAction>();
	if (!mov)
		return;

	Basis::Entity* obj = mov->getObject();
	if (!obj)
		return;

	auto spt = obj->as<Basis::Spatial>();
	if (!spt)
		return;

	auto chessboard = Basis::toSingle<Chessboard>(system()->findEntitiesByName(ChessboardName));
	if (!chessboard)
		return;

	int boardSize = chessboard->size();

	Basis::point3d p = spt->position();
	double y = p.get<1>();
	if (y < boardSize - 1)
		p.set<1>(y + 1.0);
	spt->setPosition(p);
}

MoveEastAction::MoveEastAction(Basis::System* s) :
	Basis::Entity(s)
{
	auto mov = addFacet<MoveAction>();
	if (mov)
		mov->setMoveFunction(std::bind(&MoveEastAction::moveFunction, this));
}

void MoveEastAction::moveFunction()
{
	auto mov = this->as<MoveAction>();
	if (!mov)
		return;

	Basis::Entity* obj = mov->getObject();
	if (!obj)
		return;

	auto spt = obj->as<Basis::Spatial>();
	if (!spt)
		return;

	auto chessboard = Basis::toSingle<Chessboard>(system()->findEntitiesByName(ChessboardName));
	if (!chessboard)
		return;

	int boardSize = chessboard->size();

	Basis::point3d p = spt->position();
	double x = p.get<0>();
	if (x < boardSize - 1)
		p.set<0>(x + 1.0);
	spt->setPosition(p);
}

MoveWestAction::MoveWestAction(Basis::System* s) :
	Basis::Entity(s)
{
	auto mov = addFacet<MoveAction>();
	if (mov)
		mov->setMoveFunction(std::bind(&MoveWestAction::moveFunction, this));
}

void MoveWestAction::moveFunction()
{
	auto mov = this->as<MoveAction>();
	if (!mov)
		return;

	Basis::Entity* obj = mov->getObject();
	if (!obj)
		return;

	auto spt = obj->as<Basis::Spatial>();
	if (!spt)
		return;

	Basis::point3d p = spt->position();
	double x = p.get<0>();
	if (x >= 1.0) {
		p.set<0>(x - 1.0);
		spt->setPosition(p);
	}
}

Stone::Stone(Basis::System* s) :
	Basis::Entity(s)
{
	auto spt = addFacet<Basis::Spatial>();
}

struct Chessboard::Private 
{
	int size = 0;
	float squareSize = 10;
	sf::Vector2f topLeft;
	vector<std::shared_ptr<ChessboardTypes::Square>> squares;
};

Chessboard::Chessboard(Basis::System* sys) :
	Basis::Entity(sys),
	_p(std::make_unique<Private>())
{
}

void Chessboard::create(int size)
{
	_p->size = size;
	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			auto square = std::make_shared<ChessboardTypes::Square>();
			square->x = x;
			square->y = y;
			square->left = x * _p->squareSize;
			square->top = y * _p->squareSize;
			square->width = _p->squareSize;
			square->height = _p->squareSize;

			_p->squares.push_back(square);
		}
	}
}

int Chessboard::size() const
{
	return _p->size;
}

float Chessboard::squareSize() const
{
	return _p->squareSize;
}

void Chessboard::setTopLeft(float top, float left)
{
	_p->topLeft = sf::Vector2f(left, top);
}

float Chessboard::top() const
{
	return _p->topLeft.y;
}

float Chessboard::left() const
{
	return _p->topLeft.x;
}

std::shared_ptr<ChessboardTypes::Square> Chessboard::getSquare(int x, int y)
{
	int index = y * _p->size + x;
	if (index < _p->squares.size())
		return _p->squares[index];

	return nullptr;
}

struct ChessboardLife::Private
{
	int boardSize = 16; /// ������� "��������� �����"
};

ChessboardLife::ChessboardLife(Basis::System* sys) :
	Basis::Entity(sys),
	_p(std::make_unique<Private>())
{
	auto exe = addFacet<Basis::Executable>();
	if (exe)
		exe->setStepFunction(std::bind(&ChessboardLife::step, this));

	auto board = sys->newEntity<Chessboard>();
	board->setName(ChessboardName);
	if (board)
		std::cout << "Board created: " << board->name() << endl;
	board->create(_p->boardSize);
	
	// �����
	{
		int numStones = 10;
		for (int i = 0; i < numStones; ++i) {
			auto stone = sys->newEntity<Stone>();
			auto spt = stone->as<Basis::Spatial>();
			if (spt) {
				float x = (float)sys->randomInt(0, _p->boardSize - 1);
				float y = (float)sys->randomInt(0, _p->boardSize - 1);
				spt->setPosition({ x, y });
			}
		}
	}

	// ������� ������ � �������� ��� � ����� ����
	{
		auto agent = sys->newEntity<Agent>();
		agent->setName("Agent");
		agent->setEnergy(100);
		auto spt = agent->as<Basis::Spatial>();
		if (spt) {
			float x = _p->boardSize / 2;
			float y = _p->boardSize / 2;
			spt->setPosition({ x, y });
		}
	}
}

void ChessboardLife::step()
{
	//std::cout << "ChessboardLife::step()" << endl;
	shared_ptr<Agent> agent = nullptr;
	for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
		auto ag = iter.value()->as<Agent>();
		if (ag) {
			agent = ag;
			break;
		}
	}

	if (agent)
		agent->step();
}

sf::Color newColor(const std::map<Basis::tid, sf::Color>& colors)
{
	int numIter = 5;
	std::vector<sf::Color> testColors;
	int maxMetrics = 0;
	int bestColorIndex = 0;

	for (int i = 0; i < numIter; ++i) {
		sf::Color col;
		col.r = Basis::System::randomInt(50, 255);
		col.g = Basis::System::randomInt(50, 255);
		col.b = Basis::System::randomInt(50, 255);
		testColors.push_back(col);

		int metrics = 0;
		for (auto it = colors.begin(); it != colors.end(); ++it) {
			sf::Color c = it->second;
			int dr = col.r - c.r;
			int dg = col.g - c.g;
			int db = col.b - c.b;
			metrics += (dr*dr + dg*dg + db*db);
		}

		if (metrics > maxMetrics) {
			bestColorIndex = i;
			maxMetrics = metrics;
		}
	}

	return testColors[bestColorIndex];
}

struct ChessboardLifeViewer::Private
{
	std::unique_ptr<sf::RenderWindow> window = nullptr;
	std::shared_ptr<sf::Texture> windowTexture = nullptr;
	std::unique_ptr<sf::RenderWindow> visionWindow = nullptr;
	std::unique_ptr<sf::RenderWindow> historyWindow = nullptr;
	sf::Font generalFont;
	std::string activeAgentName = "Agent";
	std::map<Basis::tid, sf::Color> entityColors;

	void drawChessboard(sf::RenderTarget* tgt);
	void drawHistory(sf::RenderTarget* tgt);
	void drawTimeFrame(sf::RenderTarget* tgt, std::shared_ptr<Entity> timeFrame, float left, float top, float width, float height);
	void drawVisualField(sf::RenderTarget* tgt);
};

void ChessboardLifeViewer::Private::drawChessboard(sf::RenderTarget* tgt)
{
	tgt->clear();

	sf::RectangleShape outRect;
	sf::FloatRect boardRect; // ������� "��������� �����"

	Basis::System* sys = Basis::System::instance();

	vector<shared_ptr<Entity>> ents = sys->findEntitiesByName(ChessboardName);
	if (!ents.empty()) {
		shared_ptr<Chessboard> board = static_pointer_cast<Chessboard>(ents[0]);
		float squareSize = board->squareSize();
		float boardRectSize = squareSize * board->size();

		// "��������� �����"
		{
			sf::Color bkColor = sf::Color(30, 30, 30);
			sf::Color foreColor = sf::Color(100, 100, 100);

			boardRect.left = 0;
			boardRect.top = 0;
			boardRect.width = boardRectSize;
			boardRect.height = boardRectSize;
			board->setTopLeft(boardRect.top, boardRect.left);

			sf::RectangleShape rectangle;
			rectangle.setPosition(sf::Vector2f(boardRect.left, boardRect.top));
			rectangle.setSize(sf::Vector2f(boardRect.width, boardRect.height));

			rectangle.setFillColor(bkColor);
			rectangle.setOutlineColor(foreColor);
			tgt->draw(rectangle);

			vector<shared_ptr<Entity>> ents = sys->findEntitiesByName(ChessboardName);
			if (!ents.empty()) {
				shared_ptr<Chessboard> board = static_pointer_cast<Chessboard>(ents[0]);
				float y = boardRect.top;
				for (int row = 0; row < board->size(); ++row) {
					float x = boardRect.left;
					for (int col = 0; col < board->size(); ++col) {
						sf::RectangleShape rectShape;
						rectShape.setPosition(sf::Vector2f(x, y));
						rectShape.setSize(sf::Vector2f(squareSize, squareSize));
						rectShape.setFillColor(bkColor);
						rectShape.setOutlineColor(foreColor);
						rectShape.setOutlineThickness(1.0);
						tgt->draw(rectShape);
						x += squareSize;
					}
					y += squareSize;
				}
			}
		}

		// ������
		{

			for (auto iter = sys->entityIterator(); iter.hasMore(); iter.next()) {
				auto agent = iter.value()->as<Agent>();
				if (agent) {
					sf::Color bkColor = sf::Color(200, 50, 50);
					sf::Color foreColor = sf::Color(100, 100, 100);
					sf::RectangleShape rectShape;

					auto spt = agent->as<Basis::Spatial>();
					std::shared_ptr<ChessboardTypes::Square> square = board->getSquare(spt->position().get<0>(), spt->position().get<1>());

					float x = boardRect.left + square->x * squareSize;
					float y = boardRect.top + square->y * squareSize;

					rectShape.setPosition(sf::Vector2f(x, y));
					rectShape.setSize(sf::Vector2f(squareSize - 1, squareSize - 1));
					rectShape.setFillColor(bkColor);
					tgt->draw(rectShape);
				}

				auto stone = iter.value()->as<Stone>();
				if (stone) {
					sf::Color bkColor = sf::Color(70, 70, 70);
					sf::Color foreColor = sf::Color(70, 70, 70);

					auto spt = stone->as<Basis::Spatial>();
					std::shared_ptr<ChessboardTypes::Square> square = board->getSquare(spt->position().get<0>(), spt->position().get<1>());

					float x = boardRect.left + square->x * squareSize;
					float y = boardRect.top + square->y * squareSize;
					sf::RectangleShape rectShape;

					rectShape.setPosition(sf::Vector2f(x, y));
					rectShape.setSize(sf::Vector2f(squareSize - 1, squareSize - 1));
					rectShape.setFillColor(bkColor);
					tgt->draw(rectShape);
				}
			}
		}
	}
}

void ChessboardLifeViewer::Private::drawHistory(sf::RenderTarget* tgt)
{
	tgt->clear();

	sf::Vector2u size = tgt->getSize();

	auto activeAgent = Basis::toSingle<Agent>(Basis::System::instance()->findEntitiesByName(activeAgentName));
	if (activeAgent) {
		sf::Color bkColor = sf::Color(60, 60, 60);
		sf::Color foreColor = sf::Color(100, 100, 100);
		float margin = 5.0;

		sf::RectangleShape rectangle;
		//rectangle.setPosition(sf::Vector2f(rect.left, rect.top));
		rectangle.setSize(sf::Vector2f(size.x, size.y));

		rectangle.setFillColor(bkColor);
		rectangle.setOutlineColor(foreColor);
		tgt->draw(rectangle);
	}

	if (activeAgent) {
		auto histCont = Basis::toSingle<Basis::Container>(activeAgent->findEntitiesByName("History"));
		if (histCont) {
			sf::Color bkColor = sf::Color(20, 20, 20);
			sf::Color foreColor = sf::Color(200, 200, 200);
			int64_t numFrames = histCont->size();
			int64_t maxFrames = activeAgent->maxTimeFrames();
			float margin = 2.0;

			float frameHeight = (float)size.y / maxFrames - margin;
			float currentY = 0;

			auto items = histCont->items();
			for (auto it = items.cbegin(); it != items.cend(); ++it) {
				sf::FloatRect itemRect;
				itemRect.left = 0;
				itemRect.top = currentY;
				itemRect.width = size.x;
				itemRect.height = frameHeight;

				sf::RectangleShape rectangle;
				rectangle.setPosition(sf::Vector2f(itemRect.left, itemRect.top));
				rectangle.setSize(sf::Vector2f(itemRect.width, itemRect.height));

				rectangle.setFillColor(bkColor);
				rectangle.setOutlineColor(foreColor);
				tgt->draw(rectangle);

				currentY += (frameHeight + margin);

				drawTimeFrame(tgt, *it, itemRect.left, itemRect.top, itemRect.width, itemRect.height);
			}
		}
	}
}

void ChessboardLifeViewer::Private::drawTimeFrame(sf::RenderTarget* tgt, std::shared_ptr<Entity> timeFrame, float left, float top, float width, float height)
{
	float margin = 2.0;
	float itemWidth = 10.0;
	float itemHeight = 10.0;
	sf::Color collisionColor = sf::Color(255, 50, 50);
	sf::Color foreColor = sf::Color(200, 200, 200);

	shared_ptr<Basis::Container> cont = timeFrame->as<Basis::Container>();
	auto items = cont->items();
	float currentX = left;
	float currentY = top;
	for (auto it = items.cbegin(); it != items.cend(); ++it) {
		sf::FloatRect rect;
		rect.left = currentX;
		rect.top = currentY;
		rect.width = itemWidth;
		rect.height = itemHeight;

		sf::RectangleShape rectangle;
		rectangle.setPosition(sf::Vector2f(rect.left, rect.top));
		rectangle.setSize(sf::Vector2f(rect.width, rect.height));

		sf::Color color;
		Basis::tid typeId = (*it)->typeId();
		auto colorIter = entityColors.find(typeId);
		if (colorIter != entityColors.end()) {
			color = colorIter->second;
		}
		else {
			color = newColor(entityColors);
			entityColors.insert({ typeId, color });
		}
		rectangle.setFillColor(color);
		rectangle.setOutlineColor(foreColor);

		if ((*it)->name() == "Collision") {
			rectangle.setFillColor(collisionColor);
		}

		tgt->draw(rectangle);

		currentX += (itemWidth + margin);
	}
}

void ChessboardLifeViewer::Private::drawVisualField(sf::RenderTarget* tgt)
{
	auto activeAgent = Basis::toSingle<Agent>(Basis::System::instance()->findEntitiesByName(activeAgentName));
	if (!activeAgent)
		return;

	auto chessboard = Basis::toSingle<Chessboard>(Basis::System::instance()->findEntitiesByName(ChessboardName));
	if (!chessboard)
		return;

	auto spt = activeAgent->as<Basis::Spatial>();
	auto square = chessboard->getSquare(spt->position().get<0>(), spt->position().get<1>());

	sf::View view;
	view.setCenter(chessboard->left() + square->left + square->width / 2.0, chessboard->top() + square->top + square->height / 2.0);
	view.setSize(square->width * 5, square->height * 5);
	tgt->setView(view);

	drawChessboard(tgt);
}

ChessboardLifeViewer::ChessboardLifeViewer(Basis::System* s) :
	Basis::Entity(s),
	_p(std::make_unique<Private>())
{
	auto exe = addFacet<Basis::Executable>();
	if (exe)
		exe->setStepFunction(std::bind(&ChessboardLifeViewer::step, this));
}

// ���� ������� ���� ������ � ����� ����, ������ ������ ����� ���� (��� ����� ���� ������� � �������� ����������).
// ���� ������� ���� �� ������, ��������� � ������������� �����.
// � ����� ������ ��������� ������� ����������� ��������� � ������ ������ �������, ����������� ������ ��������� ���������,
// � �� ������ ������ ���� �������.
void ChessboardLifeViewer::step()
{
	if (!_p->window) {
		_p->window = make_unique<sf::RenderWindow>(sf::VideoMode(1024, 768), "Chessboard Life");
		sf::View view(sf::Vector2f(0, 0), sf::Vector2f(1024, 768));

		//if (!_p->generalFont.loadFromFile("EurostileBQ-BoldExtended.otf")) {
		//	cout << "could not load font" << endl;
		//	// TODO ����� ���� ������ ���������� ������ �����, � �� ������� � ��� �� ������
		//}
	}

	if (!_p->window)
		return;

	shared_ptr<ChessboardLife> core = nullptr;
	for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
		auto ent = iter.value()->as<ChessboardLife>();
		if (ent) {
			core = ent;
			break;
		}
	}
	if (!core)
		return;

	if (_p->window->isOpen()) {
		sf::Event event;
		while (_p->window->pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				_p->window->close();

			switch (event.type) {
			case sf::Event::Closed:
				_p->window->close();
				break;
			case sf::Event::Resized:
				_p->window->setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
				break;
			}
		}

		sf::Vector2u winSize = _p->window->getSize();
		unsigned int minSize = std::min(winSize.x, winSize.y);
		int left = (winSize.x - minSize) / 2;
		int top = (winSize.y - minSize) / 2;

		sf::RenderWindow* wnd = _p->window.get();

		auto chessboard = Basis::toSingle<Chessboard>(Basis::System::instance()->findEntitiesByName(ChessboardName));

		sf::View view;
		float boardSize = chessboard->size() * chessboard->squareSize();
		view.reset(sf::FloatRect(0, 0, boardSize, boardSize)); // ��� ����� ��������
		float rx = (float)minSize / (float)winSize.x; // �� ������� ��� ��������� �� �����������
		float ry = (float)minSize / (float)winSize.y; // �� ������� ��� ��������� �� ���������
		// ������� - �������, ���� ����� ��������, �� �������� � ����� �� ������� ���� (!):
		view.setViewport(sf::FloatRect((float)left / (float)winSize.x, (float)top / (float)winSize.y, rx, ry));
		wnd->setView(view);

		_p->drawChessboard(wnd);
		_p->window->display();
	}

	// ������
	if (!_p->visionWindow) {
		_p->visionWindow = make_unique<sf::RenderWindow>(sf::VideoMode(200, 200), "Vision");
	}

	if (_p->visionWindow) {
		if (_p->visionWindow->isOpen()) {
			_p->drawVisualField(_p->visionWindow.get());
			_p->visionWindow->display();
		}
	}

	// �������
	if (!_p->historyWindow) {
		_p->historyWindow = make_unique<sf::RenderWindow>(sf::VideoMode(200, 400), "History");
	}

	if (_p->historyWindow) {
		if (_p->historyWindow->isOpen()) {
			_p->drawHistory(_p->historyWindow.get());
			_p->historyWindow->display();
		}
	}
}

void ChessboardLifeViewer::getImage(int x, int y, ChessboardTypes::Image* dstImage)
{
	if (!_p->window)
		return;
	if (!dstImage)
		return;

	if (!_p->windowTexture) {
		_p->windowTexture = make_shared<sf::Texture>();
	}
	if (!_p->windowTexture)
		return;

	sf::Vector2u texSize = _p->windowTexture->getSize();
	sf::Vector2u winSize = _p->window->getSize();
	if (texSize.x != winSize.x || texSize.y != winSize.y) {
		if (!_p->windowTexture->create(winSize.x, winSize.y))
			return;
	}

	_p->windowTexture->update(*_p->window);
	sf::Image tempImage = _p->windowTexture->copyToImage();
	const sf::Uint8* pixelsPtr = tempImage.getPixelsPtr();

	// ����� ������� ����� ����������� �������
	int x0 = x - dstImage->width / 2;
	int y0 = y - dstImage->height / 2;

	for (int i = 0; i < dstImage->height; ++i) {
		int srcY = y0 + i;
		if (srcY >= 0 && srcY < winSize.y) {
			int srcRowPtr = srcY * winSize.x;
			for (int j = 0; j < dstImage->width; ++j) {
				int srcX = x0 + j;
				if (srcX >= 0 && srcX < winSize.x) {
					int srcPixelIndex = srcRowPtr + srcX;
					int srcByteIndex = srcPixelIndex * 4;
					int dstPixelIndex = i * dstImage->width + j;
					dstImage->pixels[dstPixelIndex] = sf::Color(*(sf::Uint32*)(pixelsPtr + srcByteIndex));
				}
			}
		}
	}
}

void setup(Basis::System* s)
{
	std::cout << "ChessboardLife::setup()" << endl;

	s->registerEntity<Chessboard>();
	s->registerEntity<ChessboardLife>();
	s->registerEntity<ChessboardLifeViewer>();
	s->registerEntity<Agent>();
	s->registerEntity<NeighborhoodSensor>();
	s->registerEntity<MoveAction>();
	s->registerEntity<StandByAction>();
	s->registerEntity<MoveNorthAction>();
	s->registerEntity<MoveSouthAction>();
	s->registerEntity<MoveEastAction>();
	s->registerEntity<MoveWestAction>();
	s->registerEntity<Stone>();
}
