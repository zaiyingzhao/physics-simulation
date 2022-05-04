#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

typedef struct condition
{
  int width; // 見えている範囲の幅
  int height; // 見えている範囲の高さ
  double G; // 重力定数
  double dt; // シミュレーションの時間幅
  double cor; // 壁の反発係数
  double magnetic; //磁場の強さ、z方向成分(奥から手前への向きを正)のみを持つ一様磁場とする
} Condition;

// 個々の物体を表す構造体
typedef struct object
{
  double m;
  double y;
  double prev_y; // 壁からの反発に使用
  double vy;
  double prev_vy;
  double x;
  double prev_x; // 壁からの反発に使用
  double vx;
  double prev_vx;
  double q; //電荷
} Object; 


void my_plot_objects(Object objs[], const size_t numobj, const double t, const Condition cond);
void my_update_velocities(Object objs[], const size_t numobj, const Condition cond);
void my_update_positions(Object objs[], const size_t numobj, const Condition cond);
void my_bounce(Object objs[], const size_t numobj, const Condition cond);

int main(int argc, char **argv) {
  const Condition cond =
    {
     .width  = 75,
     .height = 40,
     .G = 1.0,
     .dt = 1.0,
     .cor = 0.8,
     .magnetic = 10.0 //ここはいろいろ試してみる
    };
  
  size_t objnum = 3;
  Object objects[objnum];

  // objects[objnum-1] は巨大な物体を画面外に... 地球のようなものを想定
  objects[0] = (Object){ .m = 60.0, .y = -19.9, .vy = 4.0, .x = 0.0, .vx = 2.0, .q = 1.0};
  objects[1] = (Object){ .m = 60.0, .y = -19.9, .vy = 0.0, .x = -10.0, .vx = 3.0, .q = 1.0};
  objects[2] = (Object){ .m = 100000.0, .y =  1000.0, .vy = 0.0, .x = 0.0, .vx = 0.0, .q = 1.0};

  // シミュレーション. ループは整数で回しつつ、実数時間も更新する
  const double stop_time = 400;
  double t = 0;
  for (size_t i = 0 ; t <= stop_time ; i++){
    t = i * cond.dt;
    my_update_velocities(objects, objnum, cond);
    my_update_positions(objects, objnum, cond);
    my_bounce(objects, objnum, cond); // 壁があると仮定した場合に壁を跨いでいたら反射させる
    
    // 表示の座標系は width/2, height/2 のピクセル位置が原点となるようにする
    my_plot_objects(objects, objnum, t, cond);
    
    usleep(200 * 1000); // 200 x 1000us = 200 ms ずつ停止
    printf("\e[%dA", cond.height+3);// 壁とパラメータ表示分で3行
  }
  return EXIT_SUCCESS;
}

void my_plot_objects(Object objs[], const size_t numobj, const double t, const Condition cond) {
    printf("+");
    for (int _ = 0; _ < cond.width-1; _++) {
        printf("-");
    }
    printf("+\n");
    int trigger;
    for (int i = - cond.height / 2; i < cond.height / 2 ; i++) {
        printf("|");
        for (int j = - cond.width / 2; j < cond.width / 2; j++) {
            trigger = 0;
            for (int number = 0; number < numobj - 1; number++) {
                if (i <= objs[number].y && objs[number].y < i+1 && j <= objs[number].x && objs[number].x < j+1) {
                    printf("o");
                    trigger = 1;
                    break;
                }
            }
            if (trigger == 0) {
                printf(" ");
            }
        }
        printf("|\n");
    }
    printf("+");
    for (int _ = 0; _ < cond.width-1; _++) {
        printf("-");
    }
    printf("+\n");
    printf("t = %f, objs[0].x = %f, objs[0].y = %f, objs[1].x = %f, objs[1].y = %f\n", t, objs[0].x, objs[0].y, objs[1].x, objs[1].y);
}

void my_update_velocities(Object objs[], const size_t numobj, const Condition cond) {
    double accx[numobj];
    double accy[numobj];
    for (int i = 0; i < numobj; i++) { //すべての要素を0にする
        accx[i] = 0;
        accy[i] = 0;
    }
    for (int i = 0; i < numobj; i++) {
        for (int j = 0; j < numobj; j++) {
            if (j != i) {
                accx[i] += cond.G * objs[j].m * (-objs[i].x + objs[j].x) / pow(pow(objs[i].x - objs[j].x, 2.0) + pow(objs[i].y - objs[j].y, 2.0), 1.5);
                accy[i] += cond.G * objs[j].m * (-objs[i].y + objs[j].y) / pow(pow(objs[i].x - objs[j].x, 2.0) + pow(objs[i].y - objs[j].y, 2.0), 1.5);
            }
        }
        accx[i] += - objs[i].q * objs[i].vy * cond.magnetic / objs[i].m; //ローレンツ力を考慮
        accy[i] += objs[i].q * objs[i].vx * cond.magnetic / objs[i].m; //ローレンツ力を考慮
    } 
    for (int i = 0; i < numobj; i++) {
        objs[i].prev_vx = objs[i].vx;
        objs[i].vx += accx[i] * cond.dt;
        objs[i].prev_vy = objs[i].vy;
        objs[i].vy += accy[i] * cond.dt;
    }
}

void my_update_positions(Object objs[], const size_t numobj, const Condition cond) {
    for (int i = 0; i < numobj; i++) {
        objs[i].y += objs[i].prev_vy * cond.dt;
        objs[i].x += objs[i].prev_vx * cond.dt;
    }
}

void my_bounce(Object objs[], const size_t numobj, const Condition cond) {
    for (int i = 0; i < numobj-1; i++) { //巨大物体には適用しない
        if (objs[i].y > cond.height / 2 && objs[i].prev_y < cond.height / 2) {
            objs[i].y = (1 + cond.cor) / 2 * cond.height - cond.cor * objs[i].y;
            objs[i].vy = - cond.cor * objs[i].vy;
        } 
        if (objs[i].y < -cond.height / 2 && -cond.height / 2 < objs[i].prev_y) {
            objs[i].y = - (1 + cond.cor) / 2 * cond.height - cond.cor * objs[i].y;
            objs[i].vy = - cond.cor * objs[i].vy;
        }
        if (objs[i].x > cond.width / 2 && objs[i].prev_x < cond.width / 2) {
            objs[i].x = (1 + cond.cor) / 2 * cond.width - cond.cor * objs[i].x;
            objs[i].vx = - cond.cor * objs[i].vx;
        } 
        if (objs[i].x < -cond.width / 2 && -cond.width / 2 < objs[i].prev_x) {
            objs[i].x = - (1 + cond.cor) / 2 * cond.width - cond.cor * objs[i].x;
            objs[i].vx = - cond.cor * objs[i].vx;
        }
    }
}