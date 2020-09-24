#include "gba.h"
#include "player.h"
#include "enemy.h"
#include "door.h"
#include "stdio.h"

volatile unsigned short *videoBuffer = (volatile unsigned short *)0x6000000;
u32 vBlankCounter = 0;

void waitForVBlank(void) {
  while(SCANLINECOUNTER > 160);
  while(SCANLINECOUNTER < 160);
  vBlankCounter++;
  // TODO: IMPLEMENT

  // (1)
  // Write a while loop that loops until we're NOT in vBlank anymore:
  // (This prevents counting one VBlank more than once if your app is too fast)

  // (2)
  // Write a while loop that keeps going until we're in vBlank:

  // (3)
  // Finally, increment the vBlank counter:
}

static int __qran_seed = 42;
static int qran(void) {
  __qran_seed = 1664525 * __qran_seed + 1013904223;
  return (__qran_seed >> 16) & 0x7FFF;
}

int randint(int min, int max) { return (qran() * (max - min) >> 15) + min; }

void setPixel(int row, int col, u16 color) {
  videoBuffer[OFFSET(row, col, 240)] = color;
}

void drawRectDMA(int row, int col, int width, int height, volatile u16 color) {
  for (int i = 0; i < height; i++) {
    DMA[3].src = &color;
    DMA[3].dst = &videoBuffer[OFFSET(row + i, col, 240)];
    DMA[3].cnt = (width) | DMA_SOURCE_FIXED | DMA_ON;

  }
}

void drawFullScreenImageDMA(const u16 *image) {
  DMA[3].src = image;
  DMA[3].dst = videoBuffer;
  DMA[3].cnt = (240 * 160) | DMA_ON;
}

void drawImageDMA(int row, int col, int width, int height, const u16 *image) {
  for (int i = 0; i < height; i++) {
    DMA[3].src = image + OFFSET(i, 0, width);
    DMA[3].dst = videoBuffer + OFFSET(i + row, col, 240);
    DMA[3].cnt = width | DMA_ON;
  }
}
void drawsqd (const u16 *image, int width, u16 color) {
  for (int row = 0; row < width; row++) {
    DMA[3].src = &color;
    DMA[3].dst = &videoBuffer[OFFSET(0 + row, 0, (width - row - 1))];
    DMA[3].cnt = (width - row - 1) | DMA_SOURCE_FIXED | DMA_ON;
    DMA[3].src = image + OFFSET(row, 0, row + 1);
    DMA[3].dst = videoBuffer + OFFSET(row, 0, (row + 1));
    DMA[3].cnt = (row + 1) | DMA_ON;
  }
}

void fillScreenDMA(volatile u16 color) {
  DMA[3].src = &color;
  DMA[3].dst = videoBuffer;
  DMA[3].cnt = (240 * 160) | DMA_ON;
}
void drawChar(int row, int col, char ch, u16 color) {
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 8; j++) {
      if (fontdata_6x8[OFFSET(j, i, 6) + ch * 48]) {
        setPixel(row + j, col + i, color);
      }
    }
  }
}

void drawString(int row, int col, char *str, u16 color) {
  while (*str) {
    drawChar(row, col, *str++, color);
    col += 6;
  }
}

void drawCenteredString(int row, int col, int width, int height, char *str,
                        u16 color) {
  u32 len = 0;
  char *strCpy = str;
  while (*strCpy) {
    len++;
    strCpy++;
  }

  u32 strWidth = 6 * len;
  u32 strHeight = 8;

  int new_row = row + ((height - strHeight) >> 1);
  int new_col = col + ((width - strWidth) >> 1);
  drawString(new_row, new_col, str, color);
}
void movePlayer(Player* hero, int up, int down, int right, int left) {
  up = up != 0 ? 1 : 0;
  down = down != 0 ? 1 : 0;
  right = right != 0 ? 1 : 0;
  left = left != 0 ? 1 : 0;
  hero->x += 2 * (down - up);
  if (hero->x + hero->size_x > 160) {
    hero->x = 160 - hero->size_x;
  }
  if (hero->x < 0) {
    hero->x = 0;
  }
  hero->y += 2 * (right - left);
  if (hero->y + hero->size_y > 240) {
    hero->y = 240 - hero->size_y;
  }
  if (hero->y < 0) {
    hero->y = 0;
  }
}
int moveEnemy(Enemy* curr, int speed, Player* p) {
  curr->x += curr->direction_x * speed;
  curr->y += curr->direction_y * speed;
  if (curr->x + curr->size_x > 160) {
    curr->x = 160 - curr->size_x;
    curr->direction_x *= -1;
  } else if (curr->x < 0) {
    curr->x = 0;
    curr->direction_x *= -1;
  }
  if (curr->y + curr->size_y > 240) {
    curr->y = 240 - curr->size_y;
    curr->direction_y *= -1;
  } else if (curr->y < 0) {
    curr->y = 0;
    curr->direction_y *= -1;
  }
  return collision(curr, p);
}
int collision(Enemy* e, Player* p) {
  int px = p->x;
  int py = p->y;
  int isColide = 0;
  // while (i < level) {
  int ex = e->x;
  int ey = e->y;
  //Top Left
  if (px >= ex && px <= ex + e->size_x && py >= ey && py <= ey + e->size_y) {
    isColide = 1;
  }
  //Top right
  px += p->size_x;
  if (px >= ex && px <= ex + e->size_x && py >= ey && py <= ey + e->size_y) {
    isColide = 1;
  }
  //Bottom right
  py += p->size_y;
  if (px >= ex && px <= ex + e->size_x && py >= ey && py <= ey + e->size_y) {
    isColide = 1;
  }
  //Bottem left
  px -= p->size_x;
  if (px >= ex && px <= ex + e->size_x && py >= ey && py <= ey + e->size_y) {
    isColide = 1;
  }

return isColide;
}
void initialize(Player* player, Enemy enemies[MAX_ENEMIES], int level) {
  //drawImageDMA(60, 60, 25, 25, enemy_image);
  int dir[2] = {-1, 1};
  player->x = 0;
  player->y = 0;
  player->size_x = 25;
  player->size_y = 25;
  player->shield = 0;
  drawImageDMA(0, 0, 25, 25, player_image);
  for (int i = 0; i < level; i++) {
    enemies[i].x = randint(26, 240) + i;
    enemies[i].y = randint(26, 160) + i;
    enemies[i].size_x = 25;
    enemies[i].size_y = 25;
    enemies[i].direction_x = dir[randint(0, 2)];
    enemies[i].direction_y = dir[randint(0, 2)];
    drawImageDMA(enemies[i].x, enemies[i].y, 25, 25, enemy_image);
  }
  drawImageDMA(68, 215, 25, 25, door);
}
int isWin(Player* p) {
  int px = p->x;
  int py = p->y;
  int size_x = 25;
  int size_y = 25;
  int ex = 68;
  int ey = 215;
  if (px >= ex && px <= ex + size_x && py >= ey && py <= ey + size_y) {
      return 1;
    }
    //Top right
    px += p->size_x;
    if (px >= ex && px <= ex + size_x && py >= ey && py <= ey + size_y) {
      return 1;
    }
    //Bottom right
    py += p->size_y;
    if (px >= ex && px <= ex + size_x && py >= ey && py <= ey + size_y) {
      return 1;
    }
    //Bottem left
    px -= p->size_x;
    if (px >= ex && px <= ex + size_x && py >= ey && py <= ey + size_y) {
      return 1;
    }
    return 0;
}
void drawPieces(Player* player, Enemy enemis[MAX_ENEMIES], int enemy_count) {
  drawImageDMA(player->x, player->y, 25, 25, player_image);
  for (int i = 0; i < enemy_count; i++) {
      drawImageDMA(68, 215, 25, 25, door);
    drawImageDMA(enemis[i].x, enemis[i].y, 25, 25, enemy_image);
    drawImageDMA(68, 215, 25, 25, door);
  }
}
void coverPieces(Player player, Enemy enemy[MAX_ENEMIES], int enemy_count, u16 color) {
  drawRectDMA(player.x, player.y, 25, 25, color);
  for (int i = 0; i < enemy_count; i++) {
      drawImageDMA(68, 215, 25, 25, door);
    drawRectDMA(enemy[i].x, enemy[i].y, 25, 25, color);
      drawImageDMA(68, 215, 25, 25, door);
  }
}
void drawStatsWin(int level, int enemy_count) {
  drawRectDMA(0, 0, 240, 160, WHITE);
  char str[50];
  sprintf(str, "Level: %d", level);
  drawCenteredString(70, 60, 10, 10, str, GREEN);
  sprintf(str, "Number of Chungus Faced: %d", enemy_count);
  drawCenteredString(80, 80, 10, 10, str, GREEN);
}