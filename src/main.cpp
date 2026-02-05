#include <SDL.h>

#include <array>
#include <chrono>
#include <deque>
#include <random>
#include <string>

struct Vec2 {
  int x = 0;
  int y = 0;
};

static constexpr int kCell = 28;
static constexpr int kGrid = 18;
static constexpr int kBoard = kCell * kGrid;

struct Theme {
  SDL_Color bg1;
  SDL_Color bg2;
  SDL_Color panel;
  SDL_Color text;
  SDL_Color accent;
  SDL_Color snakeHead1;
  SDL_Color snakeHead2;
  SDL_Color snakeBody1;
  SDL_Color snakeBody2;
  SDL_Color snack1;
  SDL_Color snack2;
};

static const std::array<Theme, 4> kThemes = {{
    {{247, 231, 236, 255}, {239, 212, 222, 255}, {255, 245, 248, 255}, {64, 47, 52, 255},
     {216, 120, 142, 255}, {201, 177, 188, 255}, {159, 134, 150, 255}, {211, 189, 198, 255},
     {178, 153, 169, 255}, {247, 213, 198, 255}, {216, 120, 142, 255}},
    {{247, 235, 224, 255}, {239, 217, 198, 255}, {255, 246, 236, 255}, {63, 48, 38, 255},
     {222, 140, 106, 255}, {179, 194, 173, 255}, {142, 164, 143, 255}, {192, 207, 181, 255},
     {159, 178, 143, 255}, {248, 216, 181, 255}, {222, 140, 106, 255}},
    {{238, 243, 234, 255}, {219, 230, 210, 255}, {246, 250, 241, 255}, {47, 58, 49, 255},
     {155, 180, 138, 255}, {166, 184, 162, 255}, {127, 154, 130, 255}, {180, 199, 173, 255},
     {142, 170, 143, 255}, {240, 215, 185, 255}, {202, 169, 141, 255}},
    {{244, 238, 229, 255}, {227, 214, 199, 255}, {255, 247, 238, 255}, {60, 47, 42, 255},
     {214, 123, 95, 255}, {159, 193, 171, 255}, {126, 160, 138, 255}, {168, 192, 154, 255},
     {143, 181, 138, 255}, {246, 215, 178, 255}, {214, 123, 95, 255}},
}};

struct Game {
  std::deque<Vec2> snake;
  Vec2 dir{1, 0};
  Vec2 nextDir{1, 0};
  Vec2 snack{8, 8};
  int score = 0;
  int best = 0;
  int speed = 1;
  int snacks = 0;
  bool running = false;
  bool gameOver = false;
  int theme = 3;
};

static void drawRoundedRect(SDL_Renderer* r, int x, int y, int w, int h, int radius, SDL_Color color) {
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
  SDL_Rect rect{x + radius, y, w - 2 * radius, h};
  SDL_RenderFillRect(r, &rect);
  rect = {x, y + radius, w, h - 2 * radius};
  SDL_RenderFillRect(r, &rect);

  for (int dy = -radius; dy <= radius; ++dy) {
    for (int dx = -radius; dx <= radius; ++dx) {
      if (dx * dx + dy * dy <= radius * radius) {
        SDL_RenderDrawPoint(r, x + radius + dx, y + radius + dy);
        SDL_RenderDrawPoint(r, x + w - radius - 1 + dx, y + radius + dy);
        SDL_RenderDrawPoint(r, x + radius + dx, y + h - radius - 1 + dy);
        SDL_RenderDrawPoint(r, x + w - radius - 1 + dx, y + h - radius - 1 + dy);
      }
    }
  }
}

static Vec2 randomSnack(std::mt19937& rng, const std::deque<Vec2>& snake) {
  std::uniform_int_distribution<int> dist(0, kGrid - 1);
  Vec2 spot;
  while (true) {
    spot = {dist(rng), dist(rng)};
    bool ok = true;
    for (const auto& s : snake) {
      if (s.x == spot.x && s.y == spot.y) {
        ok = false;
        break;
      }
    }
    if (ok) break;
  }
  return spot;
}

static void resetGame(Game& g, std::mt19937& rng) {
  g.snake.clear();
  g.snake.push_back({4, 10});
  g.snake.push_back({3, 10});
  g.snake.push_back({2, 10});
  g.dir = {1, 0};
  g.nextDir = {1, 0};
  g.snack = randomSnack(rng, g.snake);
  g.score = 0;
  g.speed = 1;
  g.snacks = 0;
  g.running = false;
  g.gameOver = false;
}

static void step(Game& g, std::mt19937& rng) {
  g.dir = g.nextDir;
  Vec2 head = g.snake.front();
  head.x = (head.x + g.dir.x + kGrid) % kGrid;
  head.y = (head.y + g.dir.y + kGrid) % kGrid;

  for (const auto& s : g.snake) {
    if (s.x == head.x && s.y == head.y) {
      g.gameOver = true;
      g.running = false;
      return;
    }
  }

  g.snake.push_front(head);
  if (head.x == g.snack.x && head.y == g.snack.y) {
    g.score += 10;
    g.snacks += 1;
    if (g.snacks % 3 == 0 && g.speed < 8) g.speed += 1;
    if (g.score > g.best) g.best = g.score;
    g.snack = randomSnack(rng, g.snake);
  } else {
    g.snake.pop_back();
  }
}

static void drawBackground(SDL_Renderer* r, const Theme& t) {
  SDL_SetRenderDrawColor(r, t.bg1.r, t.bg1.g, t.bg1.b, 255);
  SDL_RenderClear(r);
  SDL_Rect overlay{0, 0, kBoard, kBoard};
  SDL_SetRenderDrawColor(r, t.bg2.r, t.bg2.g, t.bg2.b, 160);
  SDL_RenderFillRect(r, &overlay);
}

static void drawSnake(SDL_Renderer* r, const Game& g, const Theme& t) {
  int i = 0;
  for (const auto& s : g.snake) {
    SDL_Color c1 = i == 0 ? t.snakeHead1 : t.snakeBody1;
    SDL_Color c2 = i == 0 ? t.snakeHead2 : t.snakeBody2;
    SDL_Rect rect{ s.x * kCell + 3, s.y * kCell + 3, kCell - 6, kCell - 6 };
    drawRoundedRect(r, rect.x, rect.y, rect.w, rect.h, 8, c1);
    SDL_Rect inner{ rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4 };
    drawRoundedRect(r, inner.x, inner.y, inner.w, inner.h, 7, c2);
    if (i == 0) {
      SDL_SetRenderDrawColor(r, t.text.r, t.text.g, t.text.b, 120);
      SDL_RenderDrawPoint(r, rect.x + rect.w - 8, rect.y + rect.h / 2 - 2);
    }
    i++;
  }
}

static void drawSnack(SDL_Renderer* r, const Game& g, const Theme& t) {
  SDL_Rect rect{ g.snack.x * kCell + 4, g.snack.y * kCell + 4, kCell - 8, kCell - 8 };
  drawRoundedRect(r, rect.x, rect.y, rect.w, rect.h, 8, t.snack2);
  SDL_Rect inner{ rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4 };
  drawRoundedRect(r, inner.x, inner.y, inner.w, inner.h, 6, t.snack1);
}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
      "Cozy Snake Puzzle", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      kBoard, kBoard, SDL_WINDOW_SHOWN);
  if (!window) {
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Game game;
  std::mt19937 rng(static_cast<unsigned int>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count()));
  resetGame(game, rng);

  bool quit = false;
  auto lastTick = std::chrono::high_resolution_clock::now();

  while (!quit) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }
      if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
          case SDLK_UP: case SDLK_w:
            if (game.dir.y == 1) break;
            game.nextDir = {0, -1};
            game.running = true;
            break;
          case SDLK_DOWN: case SDLK_s:
            if (game.dir.y == -1) break;
            game.nextDir = {0, 1};
            game.running = true;
            break;
          case SDLK_LEFT: case SDLK_a:
            if (game.dir.x == 1) break;
            game.nextDir = {-1, 0};
            game.running = true;
            break;
          case SDLK_RIGHT: case SDLK_d:
            if (game.dir.x == -1) break;
            game.nextDir = {1, 0};
            game.running = true;
            break;
          case SDLK_SPACE:
            if (!game.gameOver) game.running = !game.running;
            break;
          case SDLK_r:
            resetGame(game, rng);
            break;
          case SDLK_1:
          case SDLK_2:
          case SDLK_3:
          case SDLK_4:
            game.theme = static_cast<int>(e.key.keysym.sym - SDLK_1);
            break;
          default:
            break;
        }
      }
    }

    auto now = std::chrono::high_resolution_clock::now();
    int stepMs = 140 - (game.speed - 1) * 8;
    if (game.running && !game.gameOver &&
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTick).count() >= stepMs) {
      lastTick = now;
      step(game, rng);
    }

    const Theme& t = kThemes[game.theme];
    drawBackground(renderer, t);
    drawSnack(renderer, game, t);
    drawSnake(renderer, game, t);
    SDL_RenderPresent(renderer);

    SDL_Delay(8);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
