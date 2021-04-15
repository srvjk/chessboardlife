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

struct Square
{
	int x;
	int y;
	float left;
	float top;
	float width;
	float height;
};

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
	std::shared_ptr<Square> getSquare(int x, int y);

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

private:
	void drawTimeFrame(std::shared_ptr<Entity> timeFrame, float left, float top, float width, float height);

private:
	std::unique_ptr<Private> _p;
};

extern "C" MODULE_EXPORT void setup(Basis::System* s);