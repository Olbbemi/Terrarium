#include "roots/SqliteTodoRepository.hpp"

#include <cstdint>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

#include "toolshed/sqlite/Database.hpp"
#include "trunk/domain/Todo.hpp"

namespace planning::adapter_sqlite {

namespace {

using namespace std::chrono;

int priorityToInt(domain::Priority p) {
    switch (p) {
        case domain::Priority::HIGH: return 0;
        case domain::Priority::MEDIUM: return 1;
        case domain::Priority::LOW: return 2;
    }
    return 1;
}

domain::Priority intToPriority(int v) {
    if (v == 0) return domain::Priority::HIGH;
    if (v == 2) return domain::Priority::LOW;
    return domain::Priority::MEDIUM;
}

int64_t toEpochDays(sys_days d) {
    return static_cast<int64_t>(d.time_since_epoch().count());
}

sys_days fromEpochDays(int64_t v) { return sys_days{days{v}}; }

std::vector<std::string> loadTags(SQLite::Database& db, const std::string& id) {
    SQLite::Statement q(db, "SELECT tag FROM todo_tags WHERE todo_id = ?");
    q.bind(1, id);
    std::vector<std::string> tags;
    while (q.executeStep()) tags.push_back(q.getColumn(0).getString());
    return tags;
}

// SELECT 컬럼: id,title,done,priority,due_date
const char* kCols = "id, title, done, priority, due_date";

domain::Todo rowToTodo(SQLite::Database& db, SQLite::Statement& q) {
    const std::string idStr = q.getColumn(0).getString();
    auto id = uuids::uuid::from_string(idStr).value();
    std::string title = q.getColumn(1).getString();
    const bool done = q.getColumn(2).getInt() != 0;
    domain::Priority priority = intToPriority(q.getColumn(3).getInt());
    std::optional<sys_days> due;
    if (!q.getColumn(4).isNull()) due = fromEpochDays(q.getColumn(4).getInt64());

    domain::Todo todo(id, title, priority, loadTags(db, idStr), due);
    if (done) todo.markDone();
    return todo;
}

void insertTags(SQLite::Database& db, const domain::Todo& t) {
    const std::string id = uuids::to_string(t.id());
    for (const auto& tag : t.tags()) {
        SQLite::Statement s(
            db, "INSERT INTO todo_tags (todo_id, tag) VALUES (?, ?)");
        s.bind(1, id);
        s.bind(2, tag);
        s.exec();
    }
}

void insertTodoRow(SQLite::Database& db, const domain::Todo& t) {
    SQLite::Statement s(
        db,
        "INSERT INTO todos (id, title, done, priority, due_date) "
        "VALUES (?, ?, ?, ?, ?)");
    s.bind(1, uuids::to_string(t.id()));
    s.bind(2, t.title());
    s.bind(3, t.isDone() ? 1 : 0);
    s.bind(4, priorityToInt(t.priority()));
    if (t.dueDate()) {
        s.bind(5, toEpochDays(*t.dueDate()));
    } else {
        s.bind(5);
    }
    s.exec();
}

}  // namespace

SqliteTodoRepository::SqliteTodoRepository(toolshed::sqlite::Database& db)
    : db_(db.handle()) {}

std::optional<domain::Todo> SqliteTodoRepository::findById(
    domain::Todo::Id id) const {
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols + " FROM todos WHERE id = ?");
    q.bind(1, uuids::to_string(id));
    if (q.executeStep()) return rowToTodo(db_, q);
    return std::nullopt;
}

std::vector<domain::Todo> SqliteTodoRepository::findByDueDate(
    std::chrono::sys_days due) const {
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols + " FROM todos WHERE due_date = ?");
    q.bind(1, toEpochDays(due));
    std::vector<domain::Todo> out;
    while (q.executeStep()) out.push_back(rowToTodo(db_, q));
    return out;
}

std::vector<domain::Todo> SqliteTodoRepository::findOverdue(
    std::chrono::sys_days today) const {
    SQLite::Statement q(db_, std::string("SELECT ") + kCols +
                                 " FROM todos WHERE done = 0 AND due_date IS NOT "
                                 "NULL AND due_date < ?");
    q.bind(1, toEpochDays(today));
    std::vector<domain::Todo> out;
    while (q.executeStep()) out.push_back(rowToTodo(db_, q));
    return out;
}

std::vector<domain::Todo> SqliteTodoRepository::findByTag(
    const std::string& tag) const {
    SQLite::Statement q(db_, std::string("SELECT ") +
                                 "t.id, t.title, t.done, t.priority, t.due_date "
                                 "FROM todos t JOIN todo_tags tt ON t.id = "
                                 "tt.todo_id WHERE tt.tag = ?");
    q.bind(1, tag);
    std::vector<domain::Todo> out;
    while (q.executeStep()) out.push_back(rowToTodo(db_, q));
    return out;
}

std::vector<domain::Todo> SqliteTodoRepository::findByPriority(
    domain::Priority p) const {
    SQLite::Statement q(
        db_, std::string("SELECT ") + kCols + " FROM todos WHERE priority = ?");
    q.bind(1, priorityToInt(p));
    std::vector<domain::Todo> out;
    while (q.executeStep()) out.push_back(rowToTodo(db_, q));
    return out;
}

std::vector<domain::Todo> SqliteTodoRepository::findAll() const {
    SQLite::Statement q(db_, std::string("SELECT ") + kCols + " FROM todos");
    std::vector<domain::Todo> out;
    while (q.executeStep()) out.push_back(rowToTodo(db_, q));
    return out;
}

void SqliteTodoRepository::save(const domain::Todo& t) {
    SQLite::Transaction tx(db_);
    insertTodoRow(db_, t);
    insertTags(db_, t);
    tx.commit();
}

void SqliteTodoRepository::update(const domain::Todo& t) {
    SQLite::Transaction tx(db_);
    SQLite::Statement s(
        db_,
        "UPDATE todos SET title = ?, done = ?, priority = ?, due_date = ? "
        "WHERE id = ?");
    s.bind(1, t.title());
    s.bind(2, t.isDone() ? 1 : 0);
    s.bind(3, priorityToInt(t.priority()));
    if (t.dueDate()) {
        s.bind(4, toEpochDays(*t.dueDate()));
    } else {
        s.bind(4);
    }
    s.bind(5, uuids::to_string(t.id()));
    s.exec();

    SQLite::Statement del(db_, "DELETE FROM todo_tags WHERE todo_id = ?");
    del.bind(1, uuids::to_string(t.id()));
    del.exec();
    insertTags(db_, t);
    tx.commit();
}

void SqliteTodoRepository::remove(domain::Todo::Id id) {
    SQLite::Statement s(db_, "DELETE FROM todos WHERE id = ?");
    s.bind(1, uuids::to_string(id));
    s.exec();  // todo_tags 는 FK ON DELETE CASCADE
}

}  // namespace planning::adapter_sqlite
