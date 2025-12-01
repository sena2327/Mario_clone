#include <SDL.h>
#include <SDL_image.h>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 512

//frame
const int FPS = 60;
const int frameDeray = 1000 / FPS;
Uint32 frameStart;
int frameTime;

//Stage
const float Gravity = 0.5f;
int cameraX = 0;
int cameraY = 0;

//各テクスチャーの管理（画像ファイルのパスを一箇所に集約）
namespace Assets{
    // マリオ
    constexpr const char* MARIO              = "img/mario.jpeg";
    constexpr const char* FIREMARIO          = "img/FireMario.jpeg";
    constexpr const char* STARMARIO          = "img/starmario.jpeg";
    //マリオの能力
    constexpr const char* FIRE              = "img/fire.jpeg";
    // ステージ
    constexpr const char* GOAL               = "img/goal.jpeg";
    constexpr const char* PIPE               = "img/pipe.png";

    // コイン・アイテム
    constexpr const char* COIN               = "img/coin.jpeg";
    constexpr const char* SUPERMASHROOM      = "img/supermashroom.png";
    constexpr const char* STAR               = "img/star.png";
    constexpr const char* FIREFLOWER         = "img/Fireflower.jpeg";

    // 敵
    constexpr const char* ENEMY_MASHROOM     = "img/mashroom.png";
    constexpr const char* ENEMY_GREENTURTLE  = "img/greenturtle.png";
    constexpr const char* ENEMY_GREENTURTLE_SHELL = "img/greenturtle_shell.jpeg"; // 甲羅用（必要に応じて使用）
    constexpr const char* ENEMY_FLOWER = "img/flower.jpeg"; 
}

//前方宣言
class item;
class Coin;
class SuperMashroom;
class Goal;
class Warp_Pipe;
class Fireball;

extern std::vector<Warp_Pipe*> warp_pipes;
extern std::vector<Fireball*> fire_balls;

class Stage{
    public:
        enum TileType{
            TILE_EMPTY = 0,
            TILE_GROUND = 1,
            TILE_BLOCK = 2,
            TILE_ITEMBOX = 3,
            TILE_COIN = 4,
            TILE_GOAL = 'G',
            TILE_START = 'S',
            TILE_ENEMY = 6,
            TILE_PIPE = 'P',
        };
        enum ITEM_IN_BOX{
            BOX_NONE = 0,
            BOX_COIN = 'c',
            BOX_SUPERMASHROOM = 'm',
            BOX_STAR = 's',
            BOX_FIREFLOWER = 'f',
        };
        enum EnemyType{
            NO_ENEMY = 0,
            ENEMY_MASHROOM = 'M',
            ENEMY_GREENTURTLE = 'T',
        };
        enum PIPETYPE{
            PIPE_WARP = 'W',
            PIPE_FLOWER = 'F',
            PIPE_NORMAL = '0',
        };
    private:
        TileType TILE_TABLE[256];
        ITEM_IN_BOX BOX_TABLE[256];
        EnemyType ENEMY_TABLE[256];
        PIPETYPE PIPE_TABLE[256];
        std::vector<std::vector<TileType>> tiles;
        std::vector<std::vector<ITEM_IN_BOX>> boxes;
        std::vector<std::vector<EnemyType>> enemies;
        std::vector<std::vector<PIPETYPE>> pipes;
    public:
        const int TILE_SIZE = 32;
        bool is_underground = false;
        int start_underground_row = 0;
        std::vector<std::string> raw_lines;
        int stageHeightInTiles(){
            return tiles.size();
        }

        int stageWidthInTiles(){
            return tiles[0].size();
        }

        void initTileTable() {
            for (int i = 0; i < 256; i++){
                TILE_TABLE[i] = TILE_EMPTY; // 全部0に 
                BOX_TABLE[i] = BOX_NONE;
                ENEMY_TABLE[i] = NO_ENEMY;
                PIPE_TABLE[i] = PIPE_NORMAL;
            }
            TILE_TABLE['0'] = TILE_EMPTY;
            TILE_TABLE['1'] = TILE_GROUND;
            TILE_TABLE['2'] = TILE_BLOCK;
            TILE_TABLE['3'] = TILE_ITEMBOX;
            TILE_TABLE['4'] = TILE_COIN;
            TILE_TABLE['G'] = TILE_GOAL;
            TILE_TABLE['S'] = TILE_START;
            TILE_TABLE['P'] = TILE_PIPE;
             //アイテムボックス入りのもの
            TILE_TABLE['c'] = TILE_ITEMBOX;
            BOX_TABLE['c'] = BOX_COIN;
            TILE_TABLE['m'] = TILE_ITEMBOX;
            BOX_TABLE['m'] = BOX_SUPERMASHROOM;
            TILE_TABLE['s'] = TILE_ITEMBOX;
            BOX_TABLE['s'] = BOX_STAR;
            TILE_TABLE['f'] = TILE_ITEMBOX;
            BOX_TABLE['f'] = BOX_FIREFLOWER;
            TILE_TABLE['s'] = TILE_ITEMBOX;
            BOX_TABLE['s'] = BOX_STAR;
            //敵
            TILE_TABLE['M'] = TILE_ENEMY;
            ENEMY_TABLE['M'] = ENEMY_MASHROOM;
            TILE_TABLE['T'] = TILE_ENEMY;
            ENEMY_TABLE['T']  = ENEMY_GREENTURTLE;
            //土管
            TILE_TABLE['W'] = TILE_PIPE;
            PIPE_TABLE['W'] = PIPE_WARP;
            TILE_TABLE['!'] = TILE_PIPE;
            PIPE_TABLE['!'] = PIPE_WARP;
            TILE_TABLE['#'] = TILE_PIPE;
            PIPE_TABLE['#'] = PIPE_WARP;
            TILE_TABLE['I'] = TILE_PIPE;
            PIPE_TABLE['I'] = PIPE_WARP;
            TILE_TABLE['O'] = TILE_PIPE;
            PIPE_TABLE['O'] = PIPE_WARP;
            TILE_TABLE['F'] = TILE_PIPE;
            PIPE_TABLE['F'] = PIPE_FLOWER;
        }

        void load_stage(const char* filename){
            tiles.clear();
            boxes.clear();
            enemies.clear();
            pipes.clear();
            raw_lines.clear();

            std::ifstream file(filename);
            if (!file) {
                SDL_Log("ステージファイルが開けません: %s", filename);
                return;
            }

            std::string line;
            while(std::getline(file,line)){
                if(line.empty())continue;
                if(line[0] == '*'){
                    start_underground_row = (int)tiles.size();
                    continue;
                }
                raw_lines.push_back(line);
                std::vector<TileType> tile_row;
                std::vector<ITEM_IN_BOX> box_row;
                std::vector<EnemyType> enemy_row;
                std::vector<PIPETYPE> pipe_row;
                for(char c : line){
                    tile_row.push_back(TILE_TABLE[(unsigned char)c]);
                    box_row.push_back(BOX_TABLE[(unsigned char)c]);
                    enemy_row.push_back(ENEMY_TABLE[(unsigned char)c]);
                    pipe_row.push_back(PIPE_TABLE[(unsigned char)c]);
                }
                tiles.push_back(tile_row);
                boxes.push_back(box_row);
                enemies.push_back(enemy_row);
                pipes.push_back(pipe_row);
            }
        };

        void render(SDL_Renderer* renderer,int cameraX,int cameraY){
            int start_row = 0;;
            int end_row = (int)tiles.size();
            if(!is_underground){
                start_row = 0;
                end_row = start_underground_row;
            }
            else{
                start_row = start_underground_row;
                end_row = (int)tiles.size();
            }
            for(int row = start_row; row < end_row; ++row){
                for(int col = 0; col < (int)tiles[row].size(); ++col){
                    int worldX = col * TILE_SIZE;
                    int worldY = row * TILE_SIZE;
        
                    int screenX = worldX - cameraX;
                    int screenY = worldY - cameraY;

                    if (screenX + TILE_SIZE < 0 || screenX > SCREEN_WIDTH) continue;

                    SDL_Rect r;
                    r.x = screenX;
                    r.y = screenY;
                    r.w = TILE_SIZE;
                    r.h = TILE_SIZE;

                    TileType t = tiles[row][col];
                    if(t == TILE_GROUND){
                        SDL_SetRenderDrawColor(renderer,100,60,20,255);
                        SDL_RenderFillRect(renderer, &r);
                    }
                    else if(t == TILE_BLOCK){
                        SDL_SetRenderDrawColor(renderer,150,150,150,255);   
                        SDL_RenderFillRect(renderer, &r);                 
                    }
                    else if(t == TILE_ITEMBOX){
                        SDL_SetRenderDrawColor(renderer,255,200,0,255); 
                        SDL_RenderFillRect(renderer, &r);                       
                    }
                    else if(t == TILE_PIPE ){
                        SDL_SetRenderDrawColor(renderer,180,255,100,255); 
                        SDL_RenderFillRect(renderer, &r);  
                    }
                }
            }
        };

        bool is_solid_at_pixel(int px, int py)const{
            if(px < 0 || py < 0)return false;

            int col = px / TILE_SIZE;
            int row = py / TILE_SIZE;
    
            if (row >= (int)tiles.size()) return false;
            if (col >= (int)tiles[row].size()) return false;

            TileType t = tiles[row][col];
            return (t == TILE_GROUND || t == TILE_BLOCK || t == TILE_ITEMBOX|| t == TILE_PIPE);
        }

        void hit_blocks(int px, int py, std::vector<item*>& items,SDL_Renderer* redenderer);

        TileType get_tiletype(int row,int col){
            return tiles[row][col];
        }
        EnemyType get_enemytype(int row,int col){
            return enemies[row][col];
        }
        PIPETYPE get_pipetype(int row,int col){
            return pipes[row][col];
        }
    };

class GameObject{
    public:
        SDL_Rect dstRect;
        SDL_Texture* texture = nullptr;
        float vx,vy;
        bool is_alive;
        bool is_underground;
        virtual void init(int bx,int by){
            dstRect.x = bx;dstRect.y = by;
            is_alive = true;
        }
        virtual void render(SDL_Renderer* renderer,int cameraX,int cameraY){
            if(!texture || !is_alive)return;
            SDL_Rect Screen = dstRect;
            Screen.x = dstRect.x - cameraX;
            Screen.y = dstRect.y - cameraY;
            SDL_RenderCopy(renderer,texture,NULL,&Screen);
        };
};

class Goal{
    public:
        SDL_Rect dstRect = {0,0,32,32*7};
        SDL_Texture* texture = nullptr;
        bool load_texture(SDL_Renderer* renderer){
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::GOAL);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
        void render(SDL_Renderer* renderer,int cameraX,int cameraY){
            if(texture){                
                SDL_Rect Screen = dstRect;
                Screen.x = dstRect.x - cameraX;
                Screen.y = dstRect.y - cameraY;
                SDL_RenderCopy(renderer,texture,NULL,&Screen);}
        };
        void init(int bx,int by,Stage* stage){
            dstRect.x = bx;
            dstRect.y = by - stage->TILE_SIZE*6;
        };
};
//mario
class Mario : public GameObject{
    public:
        Mario(){
            vx = 4.0f;
            vy = 0.0f;
            dstRect.w = 32;
            dstRect.h = 32;
        }
        //jump
        bool is_jumping = false;
        float jump_power = -15;
        //コイン枚数
        int coin_count = 0;
        //状態
        enum MarioState{
            Super,
            Default,
            Flash,
            Star,
            Fire,
        };
        MarioState state = Default;
        MarioState prev_state = Default;
        Uint32 invincible = 0;
        bool can_warp = true;
        //テクスチャ
        SDL_Texture* default_texture;
        SDL_Texture* fire_texture;
        SDL_Texture* star_texture;

        bool load_texture(SDL_Renderer* renderer){
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* default_surface = IMG_Load(Assets::MARIO);
            SDL_Surface* fire_surface = IMG_Load(Assets::FIREMARIO);
            SDL_Surface* star_surface = IMG_Load(Assets::STARMARIO);
            if (!default_surface || !fire_surface || !star_surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            default_texture = SDL_CreateTextureFromSurface(renderer,default_surface);
            fire_texture   = SDL_CreateTextureFromSurface(renderer,fire_surface);
            star_texture   = SDL_CreateTextureFromSurface(renderer,star_surface);

            SDL_FreeSurface(default_surface);
            SDL_FreeSurface(fire_surface);
            SDL_FreeSurface(star_surface);
            if (!default_texture || !fire_surface || !star_surface) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }
            return true;
        };
        //ジャンプ判定をする
        void jump(const Stage* stage){
            if(!is_jumping && (stage->is_solid_at_pixel(dstRect.x,dstRect.y + dstRect.h + 1) || stage->is_solid_at_pixel(dstRect.x + dstRect.w,dstRect.y + dstRect.h + 1))){
                is_jumping = true;
                vy = jump_power;
            }
        }
        //マリオの行動を更新
        void update(Stage* stage,SDL_Renderer* renderer,std::vector<item*>& items,const Uint8* keys){
            handle_vertical(stage,renderer,items,keys);
            handle_horizonal(keys,stage);
        }
        void render(SDL_Renderer* renderer,int cameraX,int cameraY)override{
            //状態で切り分け
            if(state == Super || state == Default){
                texture = default_texture;
            }
            else if(state == Fire){
                texture = fire_texture;
            }
            else if(state == Star){
                texture = star_texture;
            }
            if(!texture || !is_alive)return;
            if(state == Flash){
                Uint32 t = SDL_GetTicks();
                if(t % 2 == 1){return;}  
            }
            SDL_Rect Screen = dstRect;
            Screen.x = dstRect.x - cameraX;
            Screen.y = dstRect.y - cameraY;
            SDL_RenderCopy(renderer,texture,NULL,&Screen);
        };
        void power_up(Stage* stage,MarioState s){
            if(s == Super && !(state == Fire)){
                prev_state = state;
                state = Super;
                dstRect.h = 64;
                dstRect.y -= stage->TILE_SIZE;
            }
            else if(s == Fire){
                prev_state = state;
                state = Fire;
                dstRect.h = 64;
                dstRect.y -= stage->TILE_SIZE;             
            }
            else if(s == Star){
                prev_state = state;
                state = Star;
                invincible = SDL_GetTicks() + 5000;          
            }

        }
        void power_down(Stage* stage){
            //無敵状態でなければ
            if(SDL_GetTicks() >= invincible){
                if(state == Super){
                    prev_state = Default;
                    state = Flash;
                    invincible = SDL_GetTicks() + 1500;
                    dstRect.h = 32;
                    dstRect.y += stage->TILE_SIZE;    
                }
                else if(state == Fire){
                    prev_state = Super;
                    state = Flash;
                    invincible = SDL_GetTicks() + 1500;
                }
                else if(state == Default){
                    is_alive = false;
                }
            }
            else{
                return;
            }
        }
        void try_warp(Stage* stage);
        void fire(std::vector<Fireball*> fires,SDL_Renderer* renderer);
    private:
        Warp_Pipe* warp_point();
        //縦方向
        void handle_vertical(Stage* stage,SDL_Renderer* renderer,std::vector<item*>& items,const Uint8* keys){
            float max_fall_speed = 15.0f;
            if(vy >= max_fall_speed){
                vy = max_fall_speed;
            }
            else if(keys[SDL_SCANCODE_M] && vy > 0){
                vy += Gravity + 0.5f;
            }else{
                vy += Gravity;
            } 
            float newY = dstRect.y + vy;
            
            float head_y = dstRect.y;
            float foot_y = dstRect.y + dstRect.h;
            float Right_x = dstRect.x;
            float Left_x = dstRect.x + dstRect.w;
        
            bool foot_solidL = stage->is_solid_at_pixel(Left_x,foot_y);
            bool foot_solidR = stage->is_solid_at_pixel(Right_x,foot_y);
            bool head_solidL = stage->is_solid_at_pixel(Left_x,head_y);
            bool head_solidR = stage->is_solid_at_pixel(Right_x,head_y);
        
            int tileRow;
            //下が地面の時
            if (vy > 0 && (foot_solidL || foot_solidR)) {
                tileRow   = foot_y / stage->TILE_SIZE;
                int groundTop = tileRow * stage->TILE_SIZE;
                dstRect.y = groundTop - dstRect.h;
                is_jumping = false;
                vy = 0;
            }
            //上が固体の時
            else if(head_solidL || head_solidR){
                stage->hit_blocks(Left_x,head_y-4,items,renderer);
                tileRow  = head_y / stage->TILE_SIZE;
                int groundBottom = (tileRow + 1) * stage->TILE_SIZE + 1;
                dstRect.y = groundBottom;
                vy = -vy * 0.8;
            }
            //空中にいる時
            else{
                dstRect.y = newY;
            }
        };
        //横方向
        void handle_horizonal(const Uint8* keys,const Stage* stage){
            float newleft,newright;
            int TileCol;

            float head_y = dstRect.y + 2;
            float foot_y = dstRect.y + dstRect.h - 2;
            float Right_x = dstRect.x + dstRect.w;
            float Left_x = dstRect.x;


            if(keys[SDL_SCANCODE_A]){
                newleft = Left_x - vx;newright = Right_x - vx;
                if(stage->is_solid_at_pixel(newleft,head_y) || stage->is_solid_at_pixel(newleft,foot_y)){
                    TileCol = Left_x / stage->TILE_SIZE;
                    dstRect.x = TileCol * stage->TILE_SIZE + 1;                 
                }else{
                    dstRect.x = newleft;
                }
            }
            if(keys[SDL_SCANCODE_D]){
                newleft = Left_x + vx;newright = Right_x + vx;
                if(stage->is_solid_at_pixel(newright,head_y) || stage->is_solid_at_pixel(newright,foot_y)){
                    TileCol = Right_x / stage->TILE_SIZE;
                    dstRect.x = TileCol * stage->TILE_SIZE - 1;                 
                }else{
                    dstRect.x = newleft;
                }
            }
        };
    };

class Fireball : public GameObject{
    public:
        Fireball(){
            dstRect.h = 8;
            dstRect.w = 8;
        }
        Uint32 duration = 0;
        void init(int bx,int by,Mario* mario){
            //マリオの方向で分ける
            if(mario->vx >= 0){
                dstRect.x = mario->dstRect.x + mario->dstRect.w;dstRect.y = mario->dstRect.y;                
            }else{
                dstRect.x = mario->dstRect.x + mario->dstRect.w;dstRect.y = mario->dstRect.y;
            }
            is_alive = true;
            duration = SDL_GetTicks() + 5000;
            vx = 4;
            vy = 0;
        }
        bool load_texture(SDL_Renderer* renderer){
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::FIRE);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }
            return true;
        };
        void update(Stage* stage){
            handle_vertical(stage);
            handle_horizonal(stage);
            if(SDL_GetTicks() >= duration){
                is_alive = false;
            }
        }        
    private:
        void handle_vertical(const Stage* stage){
        vy += Gravity;        
        float newY = dstRect.y + vy;
        
        float foot_y = dstRect.y + dstRect.h;
        float Right_x = dstRect.x;
        float Left_x = dstRect.x + dstRect.w;

        bool foot_solidL = stage->is_solid_at_pixel(Left_x,foot_y);
        bool foot_solidR = stage->is_solid_at_pixel(Right_x,foot_y);

        if(vy > 0 &&  (foot_solidL || foot_solidR)){
            int tileRow   = foot_y / stage->TILE_SIZE;
            int groundTop = tileRow * stage->TILE_SIZE;
            dstRect.y = groundTop - dstRect.h;
            vy = -5;
        }
        else{
            dstRect.y = newY;
        }
    }
        void handle_horizonal(const Stage* stage){
        float newleft,newright;

        float head_y = dstRect.y + 1;
        float foot_y = dstRect.y + dstRect.h - 1;
        float Right_x = dstRect.x + dstRect.w;
        float Left_x = dstRect.x;

        newleft = Left_x + vx;newright = Right_x + vx;
        if(vx < 0){
            if(stage->is_solid_at_pixel(newleft,head_y) || stage->is_solid_at_pixel(newleft,foot_y)){
                vx = -vx;
            }else{
                dstRect.x = newleft;
            }
        }
        if(vx > 0){
            if(stage->is_solid_at_pixel(newright,head_y) || stage->is_solid_at_pixel(newright,foot_y)){
                vx = -vx;            
            }else{
                dstRect.x = newleft;
            }
        }
    }
};
    //enemy
class Enemy : public GameObject{
    public:
        Enemy(){
            dstRect.h = 32;
            dstRect.w = 32;
            vx = -2;
            vy = 0;
        }
        virtual void is_collision_mario(Mario* mario,Stage* stage){
            if (!is_alive) return;
        
            float m_foot = mario->dstRect.y + mario->dstRect.h;
            float e_head = dstRect.y;
            float margin = 10;
        
            if(SDL_HasIntersection(&mario->dstRect,&dstRect)){
                if(mario->state == Mario::Star){
                    is_alive = false;
                    return;
                }
                if(m_foot <= e_head + margin){
                    is_alive = false;
                    mario->vy = -10;
                }
                else{
                    mario->power_down(stage);
                }
            }
        
        }
        void is_collision_fireball(Fireball* fire,Stage* stage){
            if (!is_alive || !fire->is_alive) return;
            if (SDL_HasIntersection(&fire->dstRect,&dstRect)){
                is_alive = false;
            }
        }

        void update(const Stage* stage){
            handle_horizonal(stage);
            handle_vertical(stage);
        }

        virtual bool load_texture(SDL_Renderer* renderer){
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::ENEMY_MASHROOM);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };

        virtual void render(SDL_Renderer* renderer,int cameraX,int cameraY){
            if(texture && is_alive){                
                SDL_Rect Screen = dstRect;
                Screen.x = dstRect.x - cameraX;
                Screen.y = dstRect.y - cameraY;
                SDL_RenderCopy(renderer,texture,NULL,&Screen);}
        };
    protected:
        virtual void handle_horizonal(const Stage* stage){
            float newleft,newright;

            float head_y = dstRect.y + 1;
            float foot_y = dstRect.y + dstRect.h - 1;
            float Right_x = dstRect.x + dstRect.w;
            float Left_x = dstRect.x;

            newleft = Left_x + vx;newright = Right_x + vx;
            if(vx < 0){
                if(stage->is_solid_at_pixel(newleft,head_y) || stage->is_solid_at_pixel(newleft,foot_y)){
                    vx = -vx;
                }else{
                    dstRect.x = newleft;
                }
            }
            if(vx > 0){
                if(stage->is_solid_at_pixel(newright,head_y) || stage->is_solid_at_pixel(newright,foot_y)){
                    vx = -vx;            
                }else{
                    dstRect.x = newleft;
                }
            }
        }
        virtual void handle_vertical(const Stage* stage){
            vy += Gravity;        
            float newY = dstRect.y + vy;
            
            float foot_y = dstRect.y + dstRect.h;
            float Right_x = dstRect.x;
            float Left_x = dstRect.x + dstRect.w;

            bool foot_solidL = stage->is_solid_at_pixel(Left_x,foot_y);
            bool foot_solidR = stage->is_solid_at_pixel(Right_x,foot_y);

            if(vy > 0 &&  (foot_solidL || foot_solidR)){
                int tileRow   = foot_y / stage->TILE_SIZE;
                int groundTop = tileRow * stage->TILE_SIZE;
                dstRect.y = groundTop - dstRect.h;
                vy = 0;
            }
            else{
                dstRect.y = newY;
            }
        }
    };

class Mashroom : public Enemy{
};

class GreemTurtle : public Enemy{
    private:
        SDL_Texture* texture_turtle = nullptr;
        SDL_Texture* texture_shell = nullptr;
        enum State{
            WALK,
            STAMPED,
            KICKED,
        };
        State state = WALK;
    public:
        void is_collision_mario(Mario* mario,Stage* stage)override{
            if (!is_alive) return;
        
            float m_foot = mario->dstRect.y + mario->dstRect.h;
            float e_head = dstRect.y;
            float margin = 10;
        
            if(SDL_HasIntersection(&mario->dstRect,&dstRect)){
                if(mario->state == Mario::Star){
                    is_alive = false;
                    return;
                }
                if(m_foot <= e_head + margin){
                    //踏まれたら甲羅になる
                    if(state == WALK){
                        state = STAMPED;
                        vx = 0;
                    }
                    //甲羅状態で踏まれたら走る
                    else if(state == STAMPED){
                        state = KICKED;
                        mario->invincible = SDL_GetTicks() + 800;
                        if(mario->dstRect.x > dstRect.x){
                            vx = -4;
                        }
                        else{
                            vx = 4;
                        }
                    }
                    else if(state == KICKED){
                        state = STAMPED;
                        vx = 0;
                    }
                    mario->vy = -10;
                }
                else{
                    if(state == STAMPED){
                        state = KICKED;
                        mario->invincible = SDL_GetTicks() + 800;
                        if(mario->dstRect.x > dstRect.x){
                            vx = -4;
                        }
                        else{
                            vx = 4;
                        }
                    }
                    else{
                        mario->power_down(stage);
                    }
                }
            }
        }
        bool load_texture(SDL_Renderer* renderer)override{
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface_turtle = IMG_Load(Assets::ENEMY_GREENTURTLE);
            SDL_Surface* surface_shell = IMG_Load(Assets::ENEMY_GREENTURTLE_SHELL);
            if (!surface_turtle || !surface_shell) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture_turtle = SDL_CreateTextureFromSurface(renderer,surface_turtle);
            texture_shell = SDL_CreateTextureFromSurface(renderer,surface_shell);
            SDL_FreeSurface(surface_turtle);
            SDL_FreeSurface(surface_shell);
            if (!texture_shell || !texture_turtle) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }
            texture = texture_turtle;

            return true;
        };
        void render(SDL_Renderer* renderer,int cameraX,int cameraY)override{
            if(texture && is_alive){                
                SDL_Rect Screen = dstRect;
                Screen.x = dstRect.x - cameraX;
                Screen.y = dstRect.y - cameraY;
                if(state == WALK){
                    texture = texture_turtle;
                    SDL_RenderCopy(renderer,texture,NULL,&Screen);
                }
                else{
                    texture = texture_shell;
                    SDL_RenderCopy(renderer,texture,NULL,&Screen);
                }
            }
        };
};

class Flower : public Enemy{
    private:
        //HIDDEN,APPEARING,APPEARED,HIDING state machine
        enum State{
            HIDDEN,
            APPEARING,
            APPEARED,
            HIDING,
        };
        State state = HIDDEN;
        Uint32 state_start = 0;
        int base_y = 0;
    public:
        void render(SDL_Renderer* renderer,int cameraX,int cameraY)override{
            if (!texture || !is_alive) return;

            // ドカンの上端。ここより下は描画しない
            int clipY = base_y;  // 必要なら + stage->TILE_SIZE など調整
        
            int spriteTop    = dstRect.y;
            int spriteBottom = dstRect.y + dstRect.h;
        
            // パックン全体がドカンの中に隠れている場合
            if (spriteTop >= clipY) {
                return;
            }
        
            // 表示する下端は「スプライトの下端」と「ドカン上端」の小さい方
            int visibleBottom = spriteBottom;
            if (visibleBottom > clipY) {
                visibleBottom = clipY;
            }
        
            int visibleHeight = visibleBottom - spriteTop;
            if (visibleHeight <= 0) return;
        
            // テクスチャのサイズを取得
            int texW = 0, texH = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &texW, &texH);
        
            // テクスチャ側での見える高さ（縦方向を同じ割合でトリム）
            int srcH = texH * visibleHeight / dstRect.h;
            int srcY = texH - srcH;  // 下から伸びてくるタイプならこういう指定もアリ
        
            SDL_Rect src;
            src.x = 0;
            src.y = srcY;
            src.w = texW;
            src.h = srcH;
        
            SDL_Rect dst;
            dst.x = dstRect.x - cameraX;
            dst.y = spriteTop - cameraY;        // 画面上での上端はそのまま
            dst.w = dstRect.w;
            dst.h = visibleHeight;    // 下は clipY までに抑える
        
            SDL_RenderCopy(renderer, texture, &src, &dst);
        };
        bool load_texture(SDL_Renderer* renderer)override{
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::ENEMY_FLOWER);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
        void handle_horizonal(const Stage* stage)override{}
        //上下に出たり消えたりする
        void handle_vertical(const Stage* stage) override{
            const Uint32 HIDDEN_TIME   = 2000;
            const Uint32 APPEAR_TIME   = 1000;
            const Uint32 APPEARED_TIME = 2000;
            const Uint32 HIDE_TIME     = 1000;

            Uint32 now = SDL_GetTicks();

            // 初回呼び出し時に基準位置と開始時間を記録
            if (state_start == 0) {
                state_start = now;
                base_y = dstRect.y;
            }

            float bottom_y = static_cast<float>(base_y);
            float top_y    = bottom_y - stage->TILE_SIZE;
            Uint32 elapsed = now - state_start;

            switch (state){
                case HIDDEN:
                    dstRect.y = static_cast<int>(bottom_y);
                    if (elapsed >= HIDDEN_TIME) {
                        state = APPEARING;
                        state_start = now;
                    }
                    break;

                case APPEARING: {
                    float t = std::min(1.0f, elapsed / static_cast<float>(APPEAR_TIME));
                    dstRect.y = static_cast<int>(bottom_y - stage->TILE_SIZE * t);
                    if (elapsed >= APPEAR_TIME) {
                        state = APPEARED;
                        state_start = now;
                        dstRect.y = static_cast<int>(top_y);
                    }
                    break;
                }

                case APPEARED:
                    dstRect.y = static_cast<int>(top_y);
                    if (elapsed >= APPEARED_TIME) {
                        state = HIDING;
                        state_start = now;
                    }
                    break;

                case HIDING: {
                    float t = std::min(1.0f, elapsed / static_cast<float>(HIDE_TIME));
                    dstRect.y = static_cast<int>(top_y + stage->TILE_SIZE * t);
                    if (elapsed >= HIDE_TIME) {
                        state = HIDDEN;
                        state_start = now;
                        dstRect.y = static_cast<int>(bottom_y);
                    }
                    break;
                }
            }
        };
};
//item
class item : public GameObject{
    public:
        item(){
            dstRect.h = 32;
            dstRect.w = 32;
            vx = 2;
            vy = 0;
        }
        ~item() = default;
        virtual void on_touch(Mario* mario,Stage* stage){}
        bool check_touch(Mario* mario,Stage* stage){
            if(!is_alive)return false;
            if(SDL_HasIntersection(&mario->dstRect,&dstRect)){
                on_touch(mario,stage);
                is_alive = false;
                return true;
            };
            return false;
        };
        virtual void update(const Stage* stage){
            handle_horizonal(stage);
            handle_vertical(stage);
        }
        virtual bool load_texture(SDL_Renderer* renderer){
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::SUPERMASHROOM);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
        void render(SDL_Renderer* renderer,int cameraX,int cameraY){
            if(texture && is_alive){                
                SDL_Rect Screen = dstRect;
                Screen.x = dstRect.x - cameraX;
                Screen.y = dstRect.y - cameraY;
                SDL_RenderCopy(renderer,texture,NULL,&Screen);}
        };
    protected:
        virtual void handle_vertical(const Stage* stage){
            vy += Gravity;        
            float newY = dstRect.y + vy;
            
            float foot_y = dstRect.y + dstRect.h;
            float Right_x = dstRect.x;
            float Left_x = dstRect.x + dstRect.w;

            bool foot_solidL = stage->is_solid_at_pixel(Left_x,foot_y);
            bool foot_solidR = stage->is_solid_at_pixel(Right_x,foot_y);

            if(vy > 0 &&  (foot_solidL || foot_solidR)){
                int tileRow   = foot_y / stage->TILE_SIZE;
                int groundTop = tileRow * stage->TILE_SIZE;
                dstRect.y = groundTop - dstRect.h;
                vy = 0;
            }
            else{
                dstRect.y = newY;
            }
        }
        virtual void handle_horizonal(const Stage* stage){
            float newleft,newright;

            float head_y = dstRect.y + 1;
            float foot_y = dstRect.y + dstRect.h - 1;
            float Right_x = dstRect.x + dstRect.w;
            float Left_x = dstRect.x;

            newleft = Left_x + vx;newright = Right_x + vx;
            if(vx < 0){
                if(stage->is_solid_at_pixel(newleft,head_y) || stage->is_solid_at_pixel(newleft,foot_y)){
                    vx = -vx;
                }else{
                    dstRect.x = newleft;
                }
            }
            if(vx > 0){
                if(stage->is_solid_at_pixel(newright,head_y) || stage->is_solid_at_pixel(newright,foot_y)){
                    vx = -vx;            
                }else{
                    dstRect.x = newleft;
                }
            }
        }
    
    };

class Coin : public item{
    public:
        void on_touch(Mario* mario,Stage* stage)override{
            mario->coin_count += 1;
        }
        bool load_texture(SDL_Renderer* renderer)override{
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::COIN);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
        void handle_horizonal(const Stage* stage)override{}
        void handle_vertical(const Stage* stage)override{}
};

class SuperMashroom : public item{
    public:
        void on_touch(Mario* mario,Stage* stage)override{
            mario->power_up(stage,Mario::Super);
        }
        bool load_texture(SDL_Renderer* renderer)override{
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::SUPERMASHROOM);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
};

class Star : public item{
    public:
        Star(){
            vy = -10;
        }
        void on_touch(Mario* mario,Stage* stage)override{
            mario->power_up(stage,Mario::Star);
        }
        bool load_texture(SDL_Renderer* renderer)override{
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::STAR);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
        void handle_vertical(const Stage* stage)override{
            vy += Gravity;        
            float newY = dstRect.y + vy;
            
            float foot_y = dstRect.y + dstRect.h;
            float Right_x = dstRect.x;
            float Left_x = dstRect.x + dstRect.w;

            bool foot_solidL = stage->is_solid_at_pixel(Left_x,foot_y);
            bool foot_solidR = stage->is_solid_at_pixel(Right_x,foot_y);

            if(vy > 0 &&  (foot_solidL || foot_solidR)){
                int tileRow   = foot_y / stage->TILE_SIZE;
                int groundTop = tileRow * stage->TILE_SIZE;
                dstRect.y = groundTop - dstRect.h;
                vy = -10;
            }
            else{
                dstRect.y = newY;
            }
        }

};
class FireFlower : public item{
    public:
        FireFlower(){
            vx = 0;
        }
        void on_touch(Mario* mario,Stage* stage)override{
            mario->power_up(stage,Mario::Fire);
        }
        bool load_texture(SDL_Renderer* renderer)override{
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::FIREFLOWER);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
        void handle_vertical(const Stage* stage)override{
            vy += Gravity;        
            float newY = dstRect.y + vy;
            
            float foot_y = dstRect.y + dstRect.h;
            float Right_x = dstRect.x;
            float Left_x = dstRect.x + dstRect.w;

            bool foot_solidL = stage->is_solid_at_pixel(Left_x,foot_y);
            bool foot_solidR = stage->is_solid_at_pixel(Right_x,foot_y);

            if(vy > 0 &&  (foot_solidL || foot_solidR)){
                int tileRow   = foot_y / stage->TILE_SIZE;
                int groundTop = tileRow * stage->TILE_SIZE;
                dstRect.y = groundTop - dstRect.h;
                vy = 0;
            }
            else{
                dstRect.y = newY;
            }
        }
        void handle_horizonal(const Stage* stage)override{}
};
//土管
class Pipe{
    public:
        SDL_Rect dstRect;
        SDL_Texture* texture = nullptr;
        void init(int bx,int by,int pipe_h,int pipe_w){
            dstRect.x = bx;
            dstRect.y = by;
            dstRect.h = pipe_h;
            dstRect.w = pipe_w;
        };
        virtual void update(const Stage* stage){
            handle_horizonal(stage);
            handle_vertical(stage);
        }
        virtual bool load_texture(SDL_Renderer* renderer){
            IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
            SDL_Surface* surface = IMG_Load(Assets::PIPE);
            if (!surface) {
                SDL_Log("SDL_LoadBMP Error: %s", SDL_GetError());
                return false;
            }
        
            texture = SDL_CreateTextureFromSurface(renderer,surface);
            SDL_FreeSurface(surface);
            if (!texture) {
                SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
                return false;
            }

            return true;
        };
        void render(SDL_Renderer* renderer,int cameraX,int cameraY){
            if(texture){                
                SDL_Rect Screen = dstRect;
                Screen.x = dstRect.x - cameraX;
                Screen.y = dstRect.y - cameraY;
                SDL_RenderCopy(renderer,texture,NULL,&Screen);}
        };
    protected:
        virtual void handle_vertical(const Stage* stage){
        }
        virtual void handle_horizonal(const Stage* stage){
        }
};

class Warp_Pipe : public Pipe{
    public:
        char pipe_anker;
        bool can_in = true;
        bool can_out = true;
        Warp_Pipe* pair = nullptr;
        void init(int bx,int by,int pipe_h,int pipe_w,bool ci,bool co,char pa){
            Pipe::init(bx,by,pipe_h,pipe_w);
            can_in = ci;
            can_out = co;
            pipe_anker = pa;
        }
};

Warp_Pipe* Mario::warp_point() {
    int mario_center_x = dstRect.x + dstRect.w / 2;
    int mario_foot_y   = dstRect.y + dstRect.h;

    for (auto* wp : warp_pipes) {
        int top   = wp->dstRect.y;
        int left  = wp->dstRect.x;
        int right = wp->dstRect.x + wp->dstRect.w;

        bool x_inside   = (mario_center_x >= left && mario_center_x < right);
        bool y_near_top = (abs(mario_foot_y - top) <= 2);

        if (x_inside && y_near_top) {
            return wp;
        }
    }
    return nullptr;
}

void Stage::hit_blocks(int px, int py, std::vector<item*>& items,SDL_Renderer* redenderer){
    int col = px / TILE_SIZE;
    int row = py / TILE_SIZE;

    int worldX = col * TILE_SIZE;
    int worldY = row * TILE_SIZE;

    TileType t = tiles[row][col];
    if(t == TILE_BLOCK){
        tiles[row][col] = TILE_EMPTY;
    }
    else if(t == TILE_ITEMBOX){
        ITEM_IN_BOX i = boxes[row][col];
        if(i == BOX_COIN){
            auto* c = new Coin();
            c->init(worldX,worldY - TILE_SIZE);
            c->load_texture(redenderer);
            items.push_back(c);
        }
        else if(i == BOX_SUPERMASHROOM){
            auto* c = new SuperMashroom();
            c->init(worldX,worldY - TILE_SIZE);
            c->load_texture(redenderer);
            items.push_back(c);
        }
        else if(i == BOX_STAR){
            auto* c = new Star();
            c->init(worldX,worldY - TILE_SIZE);
            c->load_texture(redenderer);
            items.push_back(c);           
        }
        else if(i == BOX_FIREFLOWER){
            auto* c = new FireFlower();
            c->init(worldX,worldY - TILE_SIZE);
            c->load_texture(redenderer);
            items.push_back(c);           
        }
        tiles[row][col] = TILE_BLOCK;
    }
}

void Mario::try_warp(Stage* stage) {
    float foot_y = dstRect.y + dstRect.h;
    float left_x = dstRect.x;
    float right_x = dstRect.x + dstRect.w;

    bool foot_solidL = stage->is_solid_at_pixel(left_x,  foot_y + 1);
    bool foot_solidR = stage->is_solid_at_pixel(right_x, foot_y + 1);
    if (!(foot_solidL || foot_solidR)) {
        return;
    }

    int pipeRow      = static_cast<int>(foot_y) / stage->TILE_SIZE;
    int pipeColLeft  = static_cast<int>(left_x)  / stage->TILE_SIZE;
    int pipeColRight = static_cast<int>(right_x) / stage->TILE_SIZE;

    bool on_warp =
        stage->get_pipetype(pipeRow, pipeColLeft)  == Stage::PIPE_WARP ||
        stage->get_pipetype(pipeRow, pipeColRight) == Stage::PIPE_WARP;

    if (!on_warp) {
        return;
    }

    Warp_Pipe* wp = warp_point();
    if (!wp) return;
    if (!wp->can_in) return;

    Warp_Pipe* next_wp = wp->pair;
    if (!next_wp || !next_wp->can_out) return;

    dstRect.x = next_wp->dstRect.x;
    dstRect.y = next_wp->dstRect.y - dstRect.h;

    stage->is_underground = !stage->is_underground;
    if (stage->is_underground) {
        cameraY = stage->start_underground_row * stage->TILE_SIZE;
    } else {
        cameraY = 0;
    }
}

void Mario::fire(std::vector<Fireball*> fires,SDL_Renderer* renderer){
     if(state == Fire || (state == Star && prev_state == Fire)){
        Fireball* f = new Fireball();
        f->init(this->dstRect.x,this->dstRect.y,this);
        f->load_texture(renderer);
        fire_balls.push_back(f);  
     }
     else{
        return;
     }

}

Mario mario;
Goal goal;
std::vector<item*> items;
std::vector<Enemy*> enemies;
std::vector<Pipe*> pipes;
std::vector<Warp_Pipe*> warp_pipes;
std::vector<Fireball*> fire_balls;

int main(){
    if (SDL_Init(SDL_INIT_VIDEO)  != 0){
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "My Mario",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,SCREEN_HEIGHT,
        0
    );
    if (!window) {
        SDL_Log("SDL_CreateWindow Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window,-1,0);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer Error: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Stage stage;
    stage.initTileTable();
    stage.load_stage("1-1.map");
    //タイル情報から生成
    for(int row = 0; row < stage.stageHeightInTiles(); row++){
        for(int col = 0; col < stage.stageWidthInTiles(); col++){
            if(stage.get_tiletype(row,col) == Stage::TILE_COIN){
                int worldX = col * stage.TILE_SIZE;
                int worldY = row * stage.TILE_SIZE;

                Coin* c = new Coin();
                if(row <= stage.start_underground_row){
                    c->is_underground = false;
                }else{
                    c->is_underground = true;
                }
                c->init(worldX,worldY);
                c->load_texture(renderer);
                items.push_back(c);      
            }
            else if(stage.get_tiletype(row,col) == Stage::TILE_ENEMY){
                int worldX = col * stage.TILE_SIZE;
                int worldY = row * stage.TILE_SIZE;
                Enemy* e = nullptr;
                auto kind = stage.get_enemytype(row,col);
                if(kind == Stage::ENEMY_MASHROOM){
                    e = new Mashroom();
                    e->load_texture(renderer);
                }
                else if(kind == Stage::ENEMY_GREENTURTLE){
                    e = new GreemTurtle();
                    e->load_texture(renderer);          
                }
                if(e){
                    e->init(worldX,worldY);
                    if(row <= stage.start_underground_row){
                        e->is_underground = false;
                    }else{
                        e->is_underground = true;
                    }
                    enemies.push_back(e);   
                }   
            }
            else if(stage.get_tiletype(row,col) == Stage::TILE_GOAL){
                int worldX = col * stage.TILE_SIZE;
                int worldY = row * stage.TILE_SIZE;
                goal.init(worldX,worldY,&stage);
                goal.load_texture(renderer);  
            }
            else if(stage.get_tiletype(row,col) == Stage::TILE_START){
                int worldX = col * stage.TILE_SIZE;
                int worldY = row * stage.TILE_SIZE;
                mario.init(worldX,worldY);
                mario.load_texture(renderer);  
            }
            else if(stage.get_tiletype(row,col) == Stage::TILE_PIPE){
                //左か右がドカンならスキップ
                bool left_is_pipe = (col > 0) &&
                (stage.get_tiletype(row, col-1) == Stage::TILE_PIPE);
                bool up_is_pipe = (row > 0) &&
                    (stage.get_tiletype(row-1, col) == Stage::TILE_PIPE);
            
                if (left_is_pipe || up_is_pipe) continue;
        
                
                int worldX = col * stage.TILE_SIZE;
                int worldY = row * stage.TILE_SIZE;
                //土管の大きさを取得
                int pipe_h = 1;
                while (row + pipe_h < stage.stageHeightInTiles() && stage.get_tiletype(row + pipe_h, col) == Stage::TILE_PIPE) {
                    pipe_h++;
                }
                int pipe_w = 1;
                while (col + pipe_w < stage.stageWidthInTiles() && stage.get_tiletype(row, col + pipe_w) == Stage::TILE_PIPE) {
                    pipe_w++;
                }
                auto ptype = stage.get_pipetype(row,col);
                if(ptype == Stage::PIPE_WARP){
                    bool can_in = false;
                    bool can_out = false;
                    char anker;
                    if(stage.raw_lines[row+1][col] == 'I'){
                        can_in = true;
                    }
                    if(stage.raw_lines[row+1][col+1] == 'O'){
                        can_out = true;
                    }
                    anker = stage.raw_lines[row][col+1];
                    Warp_Pipe* pipe = new Warp_Pipe();
                    pipe->init(worldX,worldY,pipe_h * stage.TILE_SIZE,pipe_w * stage.TILE_SIZE,can_in,can_out,anker);
                    pipe->load_texture(renderer);
                    pipes.push_back(pipe);
                    warp_pipes.push_back(pipe);

                }
                else{
                    Pipe* pipe = new Pipe();
                    pipe->init(worldX,worldY,pipe_h * stage.TILE_SIZE,pipe_w * stage.TILE_SIZE);
                    pipe->load_texture(renderer);
                    pipes.push_back(pipe);   
                }
            }
        }
    }

    std::unordered_map<char, Warp_Pipe*> anchor_map;
    
    for (auto* wp : warp_pipes) {
        char a = wp->pipe_anker;
        if (a == '\0') continue;
        auto it = anchor_map.find(a);
        if (it == anchor_map.end()) {
            anchor_map[a] = wp;
        } else {
            Warp_Pipe* other = it->second;
            wp->pair    = other;
            other->pair = wp;
            anchor_map.erase(it);
        }
    }

    bool running = true;
    SDL_Event e;

    while(running){
        frameStart = SDL_GetTicks();

        while(SDL_PollEvent(&e)){
            if(e.type == SDL_QUIT){
                running = false;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
            if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE){
                mario.jump(&stage);
            }    
            if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_m){
                if (e.key.repeat == 0) {
                    mario.try_warp(&stage);
                }
            }
            if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_n){
                if (e.key.repeat == 0) {
                    mario.fire(fire_balls,renderer);
                }
            }
        }

        //キーの状態を取得
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        mario.update(&stage,renderer,items,keys);
        for(auto* e : enemies){
            e->update(&stage);
            e->is_collision_mario(&mario,&stage);
            for(auto* f : fire_balls){
                e->is_collision_fireball(f,&stage);
            }
        }
        for(auto* it : items){
            it->update(&stage);
            it->check_touch(&mario,&stage);
        }
        for (auto it = fire_balls.begin(); it != fire_balls.end(); ) {
            Fireball* f = *it;
            f->update(&stage);
        
            if (!f->is_alive) {
                delete f;
                it = fire_balls.erase(it);
            } else {
                ++it;
            }
        }
        // カメラをマリオに追従させる
        cameraX = mario.dstRect.x + mario.dstRect.w/2 - SCREEN_WIDTH/2;

        // ステージ範囲からはみ出ないようにクランプ
        int stagePixelWidth = stage.stageWidthInTiles() * stage.TILE_SIZE;
        if (cameraX < 0) cameraX = 0;
        if (cameraX > stagePixelWidth - SCREEN_WIDTH)
            cameraX = stagePixelWidth - SCREEN_WIDTH;
        

        SDL_SetRenderDrawColor(renderer,0,0,255,255);
        SDL_RenderClear(renderer);
        //レンダリング
        stage.render(renderer,cameraX,cameraY);
        goal.render(renderer,cameraX,cameraY);
        mario.render(renderer,cameraX,cameraY);
        for (auto* e : enemies){
            e->render(renderer,cameraX,cameraY);
        }
        for (auto* it : items){
            it->render(renderer,cameraX,cameraY);
        }
        for (auto* p : pipes){
            p->render(renderer,cameraX,cameraY);
        }
        for (auto* f : fire_balls){
            f->render(renderer,cameraX,cameraY);
        }
        SDL_RenderPresent(renderer);
        //マリオを無敵状態から戻す
        if(mario.state == Mario::Flash && SDL_GetTicks() >= mario.invincible){
            mario.state = mario.prev_state;
            mario.invincible = 0;
        }
        else if(mario.state == Mario::Star && SDL_GetTicks() >= mario.invincible){
            mario.state = mario.prev_state;
            mario.invincible = 0;         
        }

        frameTime = SDL_GetTicks() - frameStart;

        if(frameDeray > frameTime){
            SDL_Delay(frameDeray - frameTime);
        }
    }

    for (auto* it : items) {
        delete it;
    }
    for (auto* e : enemies){
        delete e;
    }
    for (auto* p : pipes){
        delete p;
    }
    for (auto* f : fire_balls){
        delete f;
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}