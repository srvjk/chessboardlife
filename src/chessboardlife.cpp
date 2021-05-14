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
	// —ущность ent (обычно это событие или действие) добавл€етс€ в текущий временной фрейм
	// истории, хран€щейс€ в агенте.
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

	// создаЄм новый временной фрейм, при необходимости удал€€ самый старый
	shared_ptr<Entity> timeFrame = newEntity<Entity>();
	timeFrame->addFacet<Basis::Container>();
	histCont->addItem(timeFrame);
	if (histCont->size() > _p->maxTimeFrames) {
		histCont->popFront();
	}

	// осматриваемс€, оцениваем обстановку, принимаем решени€...
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

	// выбираем очередное действие из очереди;
	// если очередь опустела, формируем новую.
	if (_p->actions.empty()) {
		makeActions();
	}
	if (_p->actions.empty())
		return;

	// выполн€ем действи€;
	// каждое выполненное действие помещаем в историю
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

			_p->squares.push_back(square);
		}
	}
}

int Chessboard::size() const
{
	return _p->size;
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
	int boardSize = 16; /// размеры "шахматной доски"
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
	
	// камни
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

	// создаем агента и помещаем его в центр мира
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

struct ChessboardLifeViewer::Private
{
	std::unique_ptr<sf::RenderWindow> window = nullptr;
	std::unique_ptr<sf::RenderWindow> visionWindow = nullptr;
	sf::Font generalFont;
	std::string activeAgentName = "Agent";
	std::map<Basis::tid, sf::Color> entityColors;
};

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

ChessboardLifeViewer::ChessboardLifeViewer(Basis::System* s) :
	Basis::Entity(s),
	_p(std::make_unique<Private>())
{
	auto exe = addFacet<Basis::Executable>();
	if (exe)
		exe->setStepFunction(std::bind(&ChessboardLifeViewer::step, this));
}

// ≈сли размеры окна заданы в €вном виде, создаЄм именно такое окно (это может быть полезно в процессе разработки).
// ≈сли размеры окна не заданы, переходим в полноэкранный режим.
// ¬ любом случае вычисл€ем размеры максимально возможной в данном режиме области, допускающей нужное форматное отношение,
// и всЄ рисуем внутри этой области.
void ChessboardLifeViewer::step()
{
	if (!_p->window) {
		_p->window = make_unique<sf::RenderWindow>(sf::VideoMode(1024, 768), "Chessboard Life");

		//if (!_p->generalFont.loadFromFile("EurostileBQ-BoldExtended.otf")) {
		//	cout << "could not load font" << endl;
		//	// TODO здесь надо просто подгрузить другой шрифт, а не флудить в лог об ошибке
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

	// вычисл€ем размер и положение области рисовани€, исход€ из размеров окна и нужного форматного отношени€:
	double aspectRatio = 0.5625;
	sf::Vector2f viewPos;
	sf::Vector2f viewSize;
	float textMargin = 5; // пол€ вокруг текста
	{
		sf::Vector2u actualSize = _p->window->getSize();

		double desiredWidth = actualSize.y / aspectRatio;
		if (desiredWidth <= actualSize.x) {
			viewSize.x = desiredWidth;
			viewSize.y = actualSize.y;
		}
		else {
			viewSize.x = actualSize.x;
			viewSize.y = viewSize.x * aspectRatio;
		}

		double dx = actualSize.x - viewSize.x;
		double dy = actualSize.y - viewSize.y;
		viewPos.x = dx / 2.0;
		viewPos.y = dy / 2.0;

		double viewAspectRatio = viewSize.y / viewSize.x;
	}

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

		_p->window->clear();

		sf::RectangleShape outRect;
		sf::FloatRect boardRect; // область "шахматной доски"
		sf::FloatRect histRect; // область "истории"
		sf::FloatRect visionRect; // область "зрени€" агента
		int visionRectSize = 20;

		// рисуем границы области отображени€
		{
			sf::Color bkColor = sf::Color(50, 50, 50);
			sf::Color foreColor = sf::Color(100, 100, 100);

			outRect.setPosition(viewPos);
			outRect.setSize(viewSize);

			sf::Vector2f actPos = outRect.getPosition();
			sf::Vector2f actSize = outRect.getSize();

			outRect.setFillColor(bkColor);
			outRect.setOutlineColor(foreColor);
			outRect.setOutlineThickness(1.0);
			_p->window->draw(outRect);
		}

		// "шахматна€ доска" и агенты
		vector<shared_ptr<Entity>> ents = system()->findEntitiesByName(ChessboardName);
		if (!ents.empty()) {
			shared_ptr<Chessboard> board = static_pointer_cast<Chessboard>(ents[0]);
			float boardRectSize = std::min(viewSize.x, viewSize.y);
			int boardSize = board->size();
			float squareSize = boardRectSize / boardSize;

			{
				sf::Color bkColor = sf::Color(30, 30, 30);
				sf::Color foreColor = sf::Color(100, 100, 100);

				boardRect.left = viewPos.x;
				boardRect.top = viewPos.y;
				boardRect.width = boardRectSize;
				boardRect.height = boardRectSize;

				sf::RectangleShape rectangle;
				rectangle.setPosition(sf::Vector2f(boardRect.left, boardRect.top));
				rectangle.setSize(sf::Vector2f(boardRect.width, boardRect.height));

				rectangle.setFillColor(bkColor);
				rectangle.setOutlineColor(foreColor);
				_p->window->draw(rectangle);

				vector<shared_ptr<Entity>> ents = system()->findEntitiesByName(ChessboardName);
				if (!ents.empty()) {
					shared_ptr<Chessboard> board = static_pointer_cast<Chessboard>(ents[0]);
					float y = boardRect.top;
					for (int row = 0; row < boardSize; ++row) {
						float x = boardRect.left;
						for (int col = 0; col < boardSize; ++col) {
							sf::RectangleShape rect;
							rect.setPosition(sf::Vector2f(x, y));
							rect.setSize(sf::Vector2f(squareSize, squareSize));
							rect.setFillColor(bkColor);
							rect.setOutlineColor(foreColor);
							rect.setOutlineThickness(1.0);
							_p->window->draw(rect);
							x += squareSize;
						}
						y += squareSize;
					}
				}

				//_p->window->draw(text);
			}

			// агенты
			{

				for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
					auto agent = iter.value()->as<Agent>();
					if (agent) {
						sf::Color bkColor = sf::Color(200, 50, 50);
						sf::Color foreColor = sf::Color(100, 100, 100);
						sf::RectangleShape rect;

						auto spt = agent->as<Basis::Spatial>();
						std::shared_ptr<ChessboardTypes::Square> square = board->getSquare(spt->position().get<0>(), spt->position().get<1>());

						float x = boardRect.left + square->x * squareSize;
						float y = boardRect.top + square->y * squareSize;

						rect.setPosition(sf::Vector2f(x, y));
						rect.setSize(sf::Vector2f(squareSize, squareSize));
						rect.setFillColor(bkColor);
						_p->window->draw(rect);
					}

					auto stone = iter.value()->as<Stone>();
					if (stone) {
						sf::Color bkColor = sf::Color(70, 70, 70);
						sf::Color foreColor = sf::Color(70, 70, 70);

						auto spt = stone->as<Basis::Spatial>();
						std::shared_ptr<ChessboardTypes::Square> square = board->getSquare(spt->position().get<0>(), spt->position().get<1>());

						float x = boardRect.left + square->x * squareSize;
						float y = boardRect.top + square->y * squareSize;
						sf::RectangleShape rect;

						rect.setPosition(sf::Vector2f(x, y));
						rect.setSize(sf::Vector2f(squareSize, squareSize));
						rect.setFillColor(bkColor);
						_p->window->draw(rect);
					}
				}
			}
		}

		// истори€
		{
			auto activeAgent = Basis::toSingle<Agent>(system()->findEntitiesByName(_p->activeAgentName));
			if (activeAgent) {
				sf::Color bkColor = sf::Color(60, 60, 60);
				sf::Color foreColor = sf::Color(100, 100, 100);
				float margin = 5.0;

				histRect.left = boardRect.left + boardRect.width + margin;
				histRect.top = boardRect.top;
				histRect.width = viewSize.x - boardRect.width - 2 * margin;
				histRect.height = boardRect.height;

				sf::RectangleShape rectangle;
				rectangle.setPosition(sf::Vector2f(histRect.left, histRect.top));
				rectangle.setSize(sf::Vector2f(histRect.width, histRect.height));

				rectangle.setFillColor(bkColor);
				rectangle.setOutlineColor(foreColor);
				_p->window->draw(rectangle);
			}

			if (activeAgent) {
				auto histCont = Basis::toSingle<Basis::Container>(activeAgent->findEntitiesByName("History"));
				if (histCont) {
					sf::Color bkColor = sf::Color(20, 20, 20);
					sf::Color foreColor = sf::Color(200, 200, 200);
					int64_t numFrames = histCont->size();
					int64_t maxFrames = activeAgent->maxTimeFrames();
					float margin = 2.0;
					
					float frameHeight = histRect.height / maxFrames - margin;
					float currentY = histRect.top;

					auto items = histCont->items();
					for (auto it = items.cbegin(); it != items.cend(); ++it) {
						sf::FloatRect rect;
						rect.left = histRect.left;
						rect.top = currentY;
						rect.width = histRect.width;
						rect.height = frameHeight;

						sf::RectangleShape rectangle;
						rectangle.setPosition(sf::Vector2f(rect.left, rect.top));
						rectangle.setSize(sf::Vector2f(rect.width, rect.height));

						rectangle.setFillColor(bkColor);
						rectangle.setOutlineColor(foreColor);
						_p->window->draw(rectangle);

						currentY += (frameHeight + margin);

						drawTimeFrame(*it, rect.left, rect.top, rect.width, rect.height);
					}
				}
			}
		}

		_p->window->display();

		// зрение
		if (!_p->visionWindow) {
			_p->visionWindow = make_unique<sf::RenderWindow>(sf::VideoMode(200, 200), "Vision");
		}

		if (_p->visionWindow) {
			if (_p->visionWindow->isOpen()) {
				sf::Vector2u actualSize = _p->visionWindow->getSize();

				// окрестности агента
				{
					auto activeAgent = Basis::toSingle<Agent>(system()->findEntitiesByName(_p->activeAgentName));
					std::shared_ptr<NeighborhoodSensor> vision = nullptr;
					if (activeAgent) {
						vision = Basis::toSingle<NeighborhoodSensor>(activeAgent->findEntitiesByName(NeighborhoodSensorName));
					}

					if (vision) {
						sf::Color bkColor = sf::Color(60, 60, 60);
						sf::Color foreColor = sf::Color(100, 100, 100);
						float margin = 5.0;

						auto [w, h] = vision->getSize();
						if (w > 0 && h > 0) {
							float cellSizeX = (float)actualSize.x / (float)w;
							float cellSizeY = (float)actualSize.y / (float)h;
							float squareSize = std::min(cellSizeX, cellSizeY);

							float y = 0;
							for (int row = 0; row < h; ++row) {
								float x = 0;
								for (int col = 0; col < w; ++col) {
									auto [r, g, b] = vision->getPixel(col, row);
									sf::RectangleShape rect;
									rect.setPosition(sf::Vector2f(x, y));
									rect.setSize(sf::Vector2f(squareSize, squareSize));
									rect.setFillColor(sf::Color(r, g, b));
									rect.setOutlineColor(foreColor);
									rect.setOutlineThickness(1.0);
									_p->visionWindow->draw(rect);
									x += squareSize;
								}
								y += squareSize;
							}
						}
					}
				}

				_p->visionWindow->display();
			}
		}
	}
}

void ChessboardLifeViewer::drawTimeFrame(std::shared_ptr<Entity> timeFrame, float left, float top, float width, float height)
{
	float margin = 2.0;
	float itemWidth = 10.0;
	float itemHeight = 10.0;
	//sf::Color bkColor = sf::Color(200, 200, 20);
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

		//rectangle.setFillColor(bkColor);

		sf::Color color;
		Basis::tid typeId = (*it)->typeId();
		auto colorIter = _p->entityColors.find(typeId);
		if (colorIter != _p->entityColors.end()) {
			color = colorIter->second;
		}
		else {
			color = newColor(_p->entityColors);
			_p->entityColors.insert({ typeId, color });
		}
		rectangle.setFillColor(color);
		rectangle.setOutlineColor(foreColor);

		if ((*it)->name() == "Collision") {
			rectangle.setFillColor(collisionColor);
		}

		_p->window->draw(rectangle);

		currentX += (itemWidth + margin);
		//currentY += (itemHeight + margin);
	}
}

void ChessboardLifeViewer::getImage(int x, int y, ChessboardTypes::Image* img)
{
	if (!img)
		return;

	for (int i = 0; i < img->height; ++i) {
		for (int j = 0; j < img->width; ++j) {
			int indx = i * img->width + j;
			img->pixels[indx] = sf::Color(
				Basis::System::randomInt(0, 255),
				Basis::System::randomInt(0, 255),
				Basis::System::randomInt(0, 255)
			);
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
