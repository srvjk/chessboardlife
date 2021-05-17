#pragma once

#include "basis.h"

#ifdef PLATFORM_WINDOWS
#  ifdef CHESSBOARDLIFE_LIB
#    define MODULE_EXPORT __declspec(dllexport)
#  else
#    define MODULE_EXPORT __declspec(dllimport)
#  endif
#else
#  define MODULE_EXPORT
#endif

namespace ChessboardTypes 
{
	struct Square
	{
		int x;
		int y;
		float left;
		float top;
		float width;
		float height;
	};

	struct Image;
}

class MODULE_EXPORT Agent : public Basis::Entity
{
	struct Private;

public:
	Agent(Basis::System* s);
	void step();
	int energy() const;
	void setEnergy(int e);

protected:
	/// Сконструировать вспомогательные объекты и т.п.
	void constructHelpers();
	// Сгенерировать очередную порцию действий, исходя из текущей обстановки.
	void makeActions();
	/// @brief Запомнить событие или действие.
	void memorize(std::shared_ptr<Basis::Entity> ent);

public:
	/// @brief Получить максимальное количество тайм-фреймов (глубину истории).
	int64_t maxTimeFrames() const;

private:
	std::unique_ptr<Private> _p;
};

/// @brief Сенсор окружающего пространства.
///
/// Дает агенту простейшее "зрение".
class MODULE_EXPORT NeighborhoodSensor : public Basis::Entity
{
	struct Private;

public:
	NeighborhoodSensor(Basis::System* s);
	void step();
	std::tuple<int, int> getSize() const;
	std::tuple<int, int, int> getPixel(int x, int y) const;

private:
	std::unique_ptr<Private> _p;
};

class MODULE_EXPORT MoveAction : public Basis::Entity
{
	struct Private;

public:
	MoveAction(Basis::System* s);
	void setObject(Basis::Entity* ent);
	Basis::Entity* getObject() const;
	void step();
	void setMoveFunction(std::function<void()> func);

private:
	std::unique_ptr<Private> _p;
};

class MODULE_EXPORT StandByAction : public Basis::Entity
{
public:
	StandByAction(Basis::System* s);
};

class MODULE_EXPORT MoveNorthAction : public Basis::Entity
{
public:
	MoveNorthAction(Basis::System* s);
	void moveFunction();
};

class MODULE_EXPORT MoveSouthAction : public Basis::Entity
{
public:
	MoveSouthAction(Basis::System* s);
	void moveFunction();
};

class MODULE_EXPORT MoveEastAction : public Basis::Entity
{
public:
	MoveEastAction(Basis::System* s);
	void moveFunction();
};

class MODULE_EXPORT MoveWestAction : public Basis::Entity
{
public:
	MoveWestAction(Basis::System* s);
	void moveFunction();
};

class MODULE_EXPORT Stone : public Basis::Entity
{
public:
	Stone(Basis::System* s);
};

class MODULE_EXPORT Chessboard : public Basis::Entity
{
	struct Private;

public:
	Chessboard(Basis::System* s);
	void create(int size);
	int size() const;
	std::shared_ptr<ChessboardTypes::Square> getSquare(int x, int y);

private:
	std::unique_ptr<Private> _p;
};

class MODULE_EXPORT ChessboardLife : public Basis::Entity
{
	struct Private;

public:
	ChessboardLife(Basis::System* s);
	void step();

private:
	std::unique_ptr<Private> _p;
};

class MODULE_EXPORT ChessboardLifeViewer : public Basis::Entity
{
	struct Private;

public:
	ChessboardLifeViewer(Basis::System* s);
	void step();
	/// Скопировать в img фрагмент изображения мира вокруг точки (x, y)
	void getImage(int x, int y, ChessboardTypes::Image* dstImage);

private:
	std::unique_ptr<Private> _p;
};

extern "C" MODULE_EXPORT void setup(Basis::System* s);