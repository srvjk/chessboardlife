#include "chessboardlife.h"
#include <iostream>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

using namespace std;

static const string ChessboardName = "Chessboard";

TimeFrame::TimeFrame(Basis::System* s) :
	Basis::Entity(s)
{
}

History::History(Basis::System* s) :
	Basis::Entity(s)
{

}

void History::newStep()
{

}

void History::memorize(std::shared_ptr<Basis::Entity> ent)
{

}

struct Agent::Private 
{
	int energy = 0;
	std::list<std::shared_ptr<Basis::Entity>> actions;
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

void Agent::step()
{
	// выбираем очередное действие из очереди;
	// если очередь опустела, формируем новую.
	if (_p->actions.empty()) {
		makeActions();
	}
	if (_p->actions.empty())
		return;

	shared_ptr<History> history = nullptr;
	for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
		auto ent = iter.value()->as<History>();
		if (ent) {
			history = ent;
			break;
		}
	}

	// выполняем действия;
	// каждое выполненное действие помещаем в историю
	auto act = _p->actions.front();
	_p->actions.pop_front();
	auto moveAct = act->as<MoveAction>();
	if (moveAct) {
		moveAct->step();
	}
	system()->removeEntity(act->id());

	if (history) {
		history->memorize(act);
	}
}

int Agent::energy() const
{
	return _p->energy;
}

void Agent::setEnergy(int e)
{
	_p->energy = e;
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

	Basis::point3d p = spt->position();
	double y = p.get<1>();
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

	Basis::point3d p = spt->position();
	double x = p.get<0>();
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

EnergyIncreaseEvent::EnergyIncreaseEvent(Basis::System* s)
{
	auto exe = addFacet<Basis::Executable>();
	if (exe)
		exe->setStepFunction(std::bind(&EnergyIncreaseEvent::step, this));
}

void EnergyIncreaseEvent::step()
{
	Entity* p = parent();
	if (!p)
		return;

	shared_ptr<Agent> agent = p->as<Agent>();
	if (agent) {
		int e = agent->energy();
		e = e + 1;
		agent->setEnergy(e);
	}

	//die();
}

EnergyDecreaseEvent::EnergyDecreaseEvent(Basis::System* s)
{
	auto exe = addFacet<Basis::Executable>();
	if (exe)
		exe->setStepFunction(std::bind(&EnergyDecreaseEvent::step, this));
}

void EnergyDecreaseEvent::step()
{

}

struct Chessboard::Private 
{
	int size = 0;
	vector<std::shared_ptr<Square>> squares;
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
			auto square = std::make_shared<Square>();
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

std::shared_ptr<Square> Chessboard::getSquare(int x, int y)
{
	int index = y * _p->size + x;
	if (index < _p->squares.size())
		return _p->squares[index];

	return nullptr;
}

struct ChessboardLife::Private
{
	int boardSize = 16;              /// размеры "шахматной доски"
	std::list<TimeFrame> timeFrames; /// временнЫе фреймы (история событий и действий)
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
	
	{
		auto agent = sys->newEntity<Agent>();
		agent->setEnergy(100);
		auto spt = agent->as<Basis::Spatial>();
		if (spt)
			spt->setPosition({ 0.0, 0.0 });
	}

	{
		auto history = sys->newEntity<History>();
	}
}

void ChessboardLife::step()
{
	//std::cout << "ChessboardLife::step()" << endl;
	shared_ptr<Agent> agent = nullptr;
	shared_ptr<History> history = nullptr;
	for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
		auto ent = iter.value()->as<Agent>();
		if (ent) {
			agent = ent;
			break;
		}

		ent = iter.value()->as<History>();
		if (ent) {
			history = ent;
		}
	}

	if (history)
		history->newStep();

	if (agent)
		agent->step();
}

struct ChessboardLifeViewer::Private
{
	std::unique_ptr<sf::RenderWindow> window = nullptr;
	sf::Font generalFont;
};

ChessboardLifeViewer::ChessboardLifeViewer(Basis::System* s) :
	Basis::Entity(s),
	_p(std::make_unique<Private>())
{
	auto exe = addFacet<Basis::Executable>();
	if (exe)
		exe->setStepFunction(std::bind(&ChessboardLifeViewer::step, this));
}

// Если размеры окна заданы в явном виде, создаём именно такое окно (это может быть полезно в процессе разработки).
// Если размеры окна не заданы, переходим в полноэкранный режим.
// В любом случае вычисляем размеры максимально возможной в данном режиме области, допускающей нужное форматное отношение,
// и всё рисуем внутри этой области.
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

	// вычисляем размер и положение области рисования, исходя из размеров окна и нужного форматного отношения:
	double aspectRatio = 0.5625;
	sf::Vector2f viewPos;
	sf::Vector2f viewSize;
	float textMargin = 5; // поля вокруг текста
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
	}

	if (_p->window->isOpen()) {
		sf::Event event;
		while (_p->window->pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				_p->window->close();
		}

		_p->window->clear();

		sf::FloatRect boardRect; // область "шахматной доски"

		// рисуем границы области отображения
		{
			sf::Color bkColor = sf::Color(0, 0, 0);
			sf::Color foreColor = sf::Color(100, 100, 100);

			sf::RectangleShape outRect;
			outRect.setPosition(viewPos);
			outRect.setSize(viewSize);

			outRect.setFillColor(bkColor);
			outRect.setOutlineColor(foreColor);
			outRect.setOutlineThickness(1.0);
			_p->window->draw(outRect);
		}

		// "шахматная доска" и агенты
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
				sf::Color bkColor = sf::Color(200, 50, 50);
				sf::Color foreColor = sf::Color(100, 100, 100);
				sf::RectangleShape rect;

				for (auto iter = system()->entityIterator(); iter.hasMore(); iter.next()) {
					auto agent = iter.value()->as<Agent>();
					if (agent) {
						auto spt = agent->as<Basis::Spatial>();
						std::shared_ptr<Square> square = board->getSquare(spt->position().get<0>(), spt->position().get<1>());

						float x = boardRect.left + square->x * squareSize;
						float y = boardRect.top + square->y * squareSize;

						rect.setPosition(sf::Vector2f(x, y));
						rect.setSize(sf::Vector2f(squareSize, squareSize));
						rect.setFillColor(bkColor);
						_p->window->draw(rect);
					}
				}
			}
		}

		_p->window->display();
	}
}

void setup(Basis::System* s)
{
	std::cout << "ChessboardLife::setup()" << endl;

	s->registerEntity<Chessboard>();
	s->registerEntity<ChessboardLife>();
	s->registerEntity<ChessboardLifeViewer>();
	s->registerEntity<Agent>();
	s->registerEntity<TimeFrame>();
	s->registerEntity<History>();
	s->registerEntity<EnergyIncreaseEvent>();
	s->registerEntity<EnergyDecreaseEvent>();
	s->registerEntity<MoveAction>();
	s->registerEntity<StandByAction>();
	s->registerEntity<MoveNorthAction>();
	s->registerEntity<MoveSouthAction>();
	s->registerEntity<MoveEastAction>();
	s->registerEntity<MoveWestAction>();
}
