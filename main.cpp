#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

void setupConsole() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string toLowerUtf8(const std::string& s) {
    std::string result;
    for (size_t i = 0; i < s.size(); ) {
        const unsigned char c = s[i];
        if (c < 128) {
            result += static_cast<char>(std::tolower(c));
            ++i;
        } else if (i + 1 < s.size()) {
            const unsigned char c1 = s[i];
            const unsigned char c2 = s[i + 1];
            if (c1 == 0xD0 && c2 >= 0x90 && c2 <= 0xAF) {
                result += static_cast<char>(0xD0);
                result += static_cast<char>(c2 + 0x20);
                i += 2;
            } else if (c1 == 0xD0 && c2 == 0x81) {
                result += static_cast<char>(0xD1);
                result += static_cast<char>(0x91);
                i += 2;
            } else {
                result += s[i++];
            }
        } else {
            result += s[i++];
        }
    }
    return result;
}

std::string normalizeAnswer(const std::string& s) {
    return toLowerUtf8(trim(s));
}

bool parseInt(const std::string& s, int& out) {
    const std::string t = trim(s);
    if (t.empty()) return false;
    try {
        size_t pos = 0;
        out = std::stoi(t, &pos);
        return pos == t.size();
    } catch (...) {
        return false;
    }
}

bool matchesAny(const std::string& answer, const std::vector<std::string>& accepted) {
    const std::string normalized = normalizeAnswer(answer);
    for (const auto& variant : accepted) {
        if (normalized == variant) return true;
    }
    return false;
}

// --- Головоломка (абстрактный базовый класс) ---

class Puzzle {
public:
    virtual ~Puzzle() = default;
    virtual void showDescription() const = 0;
    virtual bool checkAnswer(const std::string& answer) const = 0;
};

// Математическая головоломка
class MathPuzzle : public Puzzle {
    int a_, b_;
public:
    MathPuzzle(int a, int b) : a_(a), b_(b) {}
    void showDescription() const override {
        std::cout << "На стене высечена задача: " << a_ << " + " << b_ << " = ?\n";
    }
    bool checkAnswer(const std::string& answer) const override {
        int value = 0;
        if (!parseInt(answer, value)) return false;
        return value == a_ + b_;
    }
};

// Головоломка-загадка
class RiddlePuzzle : public Puzzle {
    std::vector<std::string> accepted_;
public:
    explicit RiddlePuzzle(std::vector<std::string> answers)
        : accepted_(std::move(answers)) {}
    void showDescription() const override {
        std::cout << "На свитке написана загадка:\n"
                  << "  Что всегда уходит, но никогда не приходит?\n";
    }
    bool checkAnswer(const std::string& answer) const override {
        return matchesAny(answer, accepted_);
    }
};

// Головоломка-анаграмма
class AnagramPuzzle : public Puzzle {
    std::string scrambled_;
    std::vector<std::string> accepted_;
public:
    AnagramPuzzle(std::string scrambled, std::vector<std::string> answers)
        : scrambled_(std::move(scrambled)), accepted_(std::move(answers)) {}
    void showDescription() const override {
        std::cout << "На двери замок с буквенным кодом.\n"
                  << "Подсказка: переставьте буквы слова «" << scrambled_
                  << "», чтобы получилось название времени суток.\n";
    }
    bool checkAnswer(const std::string& answer) const override {
        return matchesAny(answer, accepted_);
    }
};

// --- Локация ---

class Location {
    int id_;
    std::vector<std::string> form_;
    std::vector<int> connections_;
    bool isOpen_;
    std::unique_ptr<Puzzle> puzzle_;
    std::string name_;
    bool isFinal_;

public:
    Location(int id, std::vector<std::string> form, std::string name,
             bool isOpen, bool isFinal = false)
        : id_(id), form_(std::move(form)), name_(std::move(name)),
          isOpen_(isOpen), isFinal_(isFinal) {}

    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    bool isOpen() const { return isOpen_; }
    bool isFinal() const { return isFinal_; }

    void setPuzzle(std::unique_ptr<Puzzle> puzzle) { puzzle_ = std::move(puzzle); }
    void open() { isOpen_ = true; puzzle_.reset(); }
    void addConnection(int targetId) { connections_.push_back(targetId); }
    const std::vector<int>& getConnections() const { return connections_; }

    void draw() const {
        std::cout << "\n=== " << name_ << " ===\n";
        for (const auto& line : form_) {
            std::cout << line << '\n';
        }
        std::cout << (isOpen_ ? "[открыта]" : "[закрыта]") << '\n';
    }

    bool solvePuzzle() {
        if (isOpen_ || !puzzle_) return true;

        std::cout << "\n--- Головоломка ---\n";
        puzzle_->showDescription();

        std::string answer;
        std::cout << "Ваш ответ: ";
        std::getline(std::cin, answer);

        if (puzzle_->checkAnswer(answer)) {
            std::cout << "Верно! Локация открыта.\n";
            open();
            return true;
        }
        std::cout << "Неверно. Попробуйте снова позже.\n";
        return false;
    }
};

// --- Персонаж ---

class Character {
    int currentLocationId_;

public:
    explicit Character(int startLocationId) : currentLocationId_(startLocationId) {}

    int getLocationId() const { return currentLocationId_; }

    void interactWithClosedLocations(std::vector<Location>& locations) {
        const Location& current = locations[currentLocationId_];
        std::cout << "\nЗакрытые проходы из «" << current.getName() << "»:\n";

        bool found = false;
        for (int connId : current.getConnections()) {
            if (!locations[connId].isOpen()) {
                found = true;
                std::cout << "  [" << connId << "] "
                          << locations[connId].getName() << " (закрыта)\n";
            }
        }
        if (!found) {
            std::cout << "  Нет закрытых проходов.\n";
            return;
        }

        std::cout << "Выберите номер локации для головоломки (0 — отмена): ";
        std::string input;
        std::getline(std::cin, input);
        int choice = 0;
        if (!parseInt(input, choice)) {
            std::cout << "Неверный выбор.\n";
            return;
        }

        if (choice == 0) return;

        bool valid = false;
        for (int connId : current.getConnections()) {
            if (connId == choice && !locations[connId].isOpen()) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            std::cout << "Неверный выбор.\n";
            return;
        }

        locations[choice].solvePuzzle();
    }

    bool interactWithOpenLocations(std::vector<Location>& locations) {
        const Location& current = locations[currentLocationId_];
        std::cout << "\nДоступные проходы из «" << current.getName() << "»:\n";

        bool found = false;
        for (int connId : current.getConnections()) {
            if (locations[connId].isOpen()) {
                found = true;
                std::cout << "  [" << connId << "] "
                          << locations[connId].getName() << '\n';
            }
        }
        if (!found) {
            std::cout << "  Нет открытых проходов.\n";
            return false;
        }

        std::cout << "Выберите номер локации (0 — остаться): ";
        std::string input;
        std::getline(std::cin, input);
        int choice = 0;
        if (!parseInt(input, choice)) {
            std::cout << "Неверный выбор.\n";
            return false;
        }

        if (choice == 0) return false;

        bool valid = false;
        for (int connId : current.getConnections()) {
            if (connId == choice && locations[connId].isOpen()) {
                valid = true;
                break;
            }
        }
        if (!valid) {
            std::cout << "Неверный выбор.\n";
            return false;
        }

        currentLocationId_ = choice;
        locations[choice].draw();

        if (locations[choice].isFinal()) {
            std::cout << "\n*** Поздравляем! Вы выбрались из лабиринта! ***\n";
            return true;
        }
        return false;
    }
};

// --- Инициализация игрового мира ---

std::vector<Location> createWorld() {
    std::vector<Location> world;

    world.emplace_back(0,
        std::vector<std::string>{
            "  +---------------------------+",
            "  |  ##  ВХОД В ЛАБИРИНТ  ##  |",
            "  |                           |",
            "  |    @                      |",
            "  |    |\\                     |",
            "  |    | \\                    |",
            "  |   /   \\                   |",
            "  |  /     \\                  |",
            "  |         \\___              |",
            "  +---------------------------+"
        },
        "Вход в лабиринт", true);

    world.emplace_back(1,
        std::vector<std::string>{
            "  +---------------------------+",
            "  |     ЗАЛ С КОЛОННАМИ       |",
            "  |                           |",
            "  |    |  |  |  |  |  |       |",
            "  |    |  |  |  |  |  |       |",
            "  |    +--+--+--+--+--+       |",
            "  |         [?]               |",
            "  |                           |",
            "  +---------------------------+"
        },
        "Зал с колоннами", false);

    world.emplace_back(2,
        std::vector<std::string>{
            "  +---------------------------+",
            "  |       БИБЛИОТЕКА          |",
            "  |                           |",
            "  |  |\"\"\"|  |\"\"\"|  |\"\"\"|      |",
            "  |  |   |  |   |  |   |      |",
            "  |  |___|  |___|  |___|      |",
            "  |       ~ свиток ~          |",
            "  |                           |",
            "  +---------------------------+"
        },
        "Библиотека", false);

    world.emplace_back(3,
        std::vector<std::string>{
            "  +---------------------------+",
            "  |    ВЫХОД НА ПОВЕРХНОСТЬ   |",
            "  |                           |",
            "  |         .-\"\"\"-.           |",
            "  |        /       \\          |",
            "  |       |  ☀ ☀ ☀  |         |",
            "  |        \\       /          |",
            "  |         `-...-'           |",
            "  |                           |",
            "  +---------------------------+"
        },
        "Выход на поверхность", false, true);

    world[0].addConnection(1);
    world[0].addConnection(2);
    world[1].addConnection(3);
    world[2].addConnection(3);

    world[1].setPuzzle(std::make_unique<MathPuzzle>(17, 25));
    world[2].setPuzzle(std::make_unique<RiddlePuzzle>(std::vector<std::string>{"время"}));
    world[3].setPuzzle(std::make_unique<AnagramPuzzle>(
        "РЕЧВЕ", std::vector<std::string>{"вечер"}));

    return world;
}

void showMenu() {
    std::cout << "\n--- Меню ---\n"
              << "  1 — Решить головоломку (закрытая локация)\n"
              << "  2 — Перейти в открытую локацию\n"
              << "  3 — Показать текущую локацию\n"
              << "  0 — Выход из игры\n"
              << "Выбор: ";
}

int main() {
    setupConsole();

    std::vector<Location> world = createWorld();
    Character player(0);

    std::cout << "=== ЛАБИРИНТ: текстовый квест ===\n"
              << "Вы проснулись в подземном лабиринте.\n"
              << "Найдите выход на поверхность!\n";

    world[0].draw();

    bool won = false;
    while (!won) {
        showMenu();
        std::string input;
        std::getline(std::cin, input);
        int choice = -1;
        if (!parseInt(input, choice)) {
            std::cout << "Неверный выбор.\n";
            continue;
        }

        switch (choice) {
            case 1:
                player.interactWithClosedLocations(world);
                break;
            case 2:
                won = player.interactWithOpenLocations(world);
                break;
            case 3:
                world[player.getLocationId()].draw();
                break;
            case 0:
                std::cout << "До свидания!\n";
                return 0;
            default:
                std::cout << "Неверный выбор.\n";
        }
    }

    return 0;
}
