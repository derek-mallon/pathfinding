#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <unordered_map>

namespace pool{

    struct Handle{
        unsigned int index;
        int count;
        Handle(unsigned int index,int count):index(index),count(count){}
    };

    template<class T>
        class Pool{
            std::vector<T> data;
            std::vector<unsigned int> next_frees;
            std::vector<int> time;
            std::vector<bool> alive;
            unsigned int next_free;
            unsigned int number_of_free;
            int counter;
            public:
            Pool():next_free(0),number_of_free(0),counter(0){}
            Handle add(T item){
                counter++;
                if(number_of_free){
                    auto index = next_free;
                    number_of_free--;
                    if(number_of_free)
                        next_free =  next_frees[next_free];
                    data[index] = item;
                    time[index] = counter;
                    alive[index] = true;
                    return Handle(index,counter);
                }
                bool resize = data.size() == data.capacity();
                data.push_back(item);
                if(resize){
                    next_frees.reserve(data.capacity());
                    time.reserve(data.capacity());
                }
                time.push_back(counter);
                next_frees.push_back(0);
                alive.push_back(true);
                return Handle(data.size()-1,counter);
            }
            bool valid(Handle handle){
                return time[handle.index] == handle.count && alive[handle.index];
            }
            void free(Handle handle){
                assert(valid(handle));
                alive[handle.index] = false;
                number_of_free++;
                next_frees[handle.index] = next_free;
                next_free = handle.index;
            }
            T& operator[] (Handle handle){
                assert(valid(handle));
                return data[handle.index];
            }
        };

}


namespace debug{
    class DebugPrimavtive: public sf::Drawable,sf::Transformable{
        sf::VertexArray vertices;
        public:
        DebugPrimavtive(){}
        DebugPrimavtive(sf::VertexArray vertices):vertices(vertices){}
        virtual void draw(sf::RenderTarget& target,sf::RenderStates states) const{
            states.transform *= getTransform();
            target.draw(vertices,states);
        }

    };
    class Debug: public sf::Drawable{
        std::vector<DebugPrimavtive> primatives;
        public:
        Debug(std::vector<DebugPrimavtive> primatives):primatives(primatives){}
        virtual void draw(sf::RenderTarget& target,sf::RenderStates states) const{
            for(const auto& primative : primatives){
                target.draw(primative);
            }
        }
    };
    struct Debugable{
        virtual Debug buildDebug() = 0;
    };
}
namespace std{
    template <>
        struct hash<sf::Vector2<float>>
        {
            size_t operator()(sf::Vector2<float> const & x) const noexcept
            {
                return (
                        (51 + std::hash<float>()(x.x)) * 51
                        + std::hash<float>()(x.y));
            }
        };
}
namespace nav{
    struct Polygon{
        Polygon(int a,int b,int c,int d):a(a),b(b),c(c),d(d){}
        unsigned int a,b,c,d;
        bool legit(int size){
            return ( a < size && b < size && c < size && d <size);
        }
    };


    class Mesh: public debug::Debugable{
        std::vector<sf::Vector2<float>> vertices;
        std::vector<Polygon> polygons;
        std::unordered_map<sf::Vector2<float>,int> map;
        public:
        int addVertex(sf::Vector2<float> vertex){
            std::unordered_map<sf::Vector2<float>,int>::const_iterator find = map.find(vertex);
            if(find == map.end()){
                auto i = vertices.size();
                map.insert(std::make_pair(vertex,i));
                vertices.push_back(vertex);
                return i;
            }else{
                return map.at(find->first);
            }
        }
        void addPolygon(Polygon poly){
            assert(poly.legit(vertices.size()));
            polygons.push_back(poly);
        }
        virtual debug::Debug buildDebug(){
            std::vector<debug::DebugPrimavtive> primatives(polygons.size());
            std::transform(polygons.begin(),polygons.end(),primatives.begin(),[this](Polygon polygon)->debug::DebugPrimavtive{
                    sf::VertexArray  _vertices(sf::LineStrip,4);
                    _vertices[0] = sf::Vertex(vertices[polygon.a],sf::Color::Blue);
                    _vertices[1] = sf::Vertex(vertices[polygon.b],sf::Color::Blue);
                    _vertices[2] = sf::Vertex(vertices[polygon.c],sf::Color::Blue);
                    _vertices[3] = sf::Vertex(vertices[polygon.d],sf::Color::Blue);

                    return debug::DebugPrimavtive(_vertices);
                    }
                    );
            return debug::Debug(primatives);
        }




    };
}


namespace collision_map{

    static const sf::Color WALKABLE_COLOR(0,255,0,255);
    static const sf::Color UNWALKABLE_COLOR(255,0,0,255);

    enum struct Status{
        WALKABLE,
        UNWALKABLE,
        VISITED,
    };
    
    sf::Rect<int> largest_rect(int ox,int oy,std::vector<std::vector<Status>>& map){
        int dn = map.size()-1;
        int ds = 0;
        int dl = ox;
        int dr = ox;
        int x = 0;
        int y = 0;
        while(dr+1 < map[0].size() && map[oy][dr+1] == Status::WALKABLE){
            dr++;
            int y = oy;
            while(y+1 < map.size() && map[y+1][dr] == Status::WALKABLE){
                y++;
            }
            if(y < dn){
                dn = y;
            }
            y = oy;
            while(y > 1 && map[y-1][dr] == Status::WALKABLE){
                y--;
            }
            if(y > ds){
                ds = y;
            }
        }
        while(dl > 1 && map[oy][dl-1] == Status::WALKABLE){
            dl--;
            int y = oy;
            while(y+1 < map.size() && map[y+1][dl] == Status::WALKABLE){
                y++;
            }
            if(y < dn){
                dn = y;
            }
            y = oy;
            while(y > 1 && map[y-1][dl] == Status::WALKABLE){
                y--;
            }
            if(y > ds){
                ds = y;
            }
        }

        if(dr == dl){
            int y = oy;
            while(y+1 < map.size() && map[y+1][dr] == Status::WALKABLE){
                y++;
            }
            if(y < dn){
                dn = y;
            }
            y = oy;
            while(y > 1 && map[y-1][dr] == Status::WALKABLE){
                y--;
            }
            if(y > ds){
                ds = y;
            }
        }

        for(y=ds;y<=dn;y++){
           for(x=dl;x<=dr;x++){
                map[y][x] = Status::VISITED;
            }
        }
    
        
        return sf::Rect<int>(dl,ds,dr-dl+1,dn-ds+1);
    }

    class Map: public debug::Debugable{
        std::vector<std::vector<Status>> raw_map;
        unsigned int tile_width;
        public:
        Map(std::string map_path,unsigned int tile_width=10):tile_width(tile_width){
            sf::Image image;
            image.loadFromFile(map_path);
            auto size = image.getSize();
            raw_map.resize(size.y);
            for(int y=0;y<size.y;y++){
                raw_map[y].resize(size.x);
                for(int x=0;x<size.x;x++){
                    if(image.getPixel(x,y) == UNWALKABLE_COLOR)
                        raw_map[y][x] = Status::UNWALKABLE;
                    else
                        raw_map[y][x] = Status::WALKABLE;
                }
            }
        }

        void generateNavMesh(nav::Mesh& mesh){
            for(int y=0;y<raw_map.size();y++){
                for(int x=0;x<raw_map.size();x++){
                    if(raw_map[y][x] == Status::WALKABLE){
                        auto rect = largest_rect(x,y,raw_map);
                        auto a  = mesh.addVertex(sf::Vector2<float>(rect.left*tile_width,rect.top*tile_width));
                        auto b  = mesh.addVertex(sf::Vector2<float>((rect.left+rect.width)*tile_width,rect.top*tile_width));
                        auto c  = mesh.addVertex(sf::Vector2<float>((rect.left+rect.width)*tile_width,(rect.top+rect.height)*tile_width));
                        auto d  = mesh.addVertex(sf::Vector2<float>(rect.left*tile_width,(rect.top+rect.height)*tile_width));
                        mesh.addPolygon(nav::Polygon(a,b,c,d));
                    }
                }
            }
        }
        virtual debug::Debug buildDebug(){
            std::vector<debug::DebugPrimavtive> primatives(raw_map.size()* raw_map[0].size());
            for(int y=0;y<raw_map.size();y++){
                for(int x=0;x<raw_map[0].size();x++){
                    if(raw_map[y][x] == Status::UNWALKABLE){
                        sf::VertexArray  _vertices(sf::Quads,4);
                        _vertices[0] = sf::Vertex(sf::Vector2f(x*tile_width,y*tile_width),sf::Color::Red);
                        _vertices[1] = sf::Vertex(sf::Vector2f(x*tile_width+tile_width,y*tile_width),sf::Color::Red);
                        _vertices[2] = sf::Vertex(sf::Vector2f(x*tile_width+tile_width,y*tile_width+tile_width),sf::Color::Red);
                        _vertices[3] = sf::Vertex(sf::Vector2f(x*tile_width,y*tile_width+tile_width),sf::Color::Red);

                        primatives.push_back(debug::DebugPrimavtive(_vertices));
                    }else if(raw_map[y][x] == Status::WALKABLE){
                        sf::VertexArray  _vertices(sf::Quads,4);
                        _vertices[0] = sf::Vertex(sf::Vector2f(x*tile_width,y*tile_width),sf::Color::Green);
                        _vertices[1] = sf::Vertex(sf::Vector2f(x*tile_width+tile_width,y*tile_width),sf::Color::Green);
                        _vertices[2] = sf::Vertex(sf::Vector2f(x*tile_width+tile_width,y*tile_width+tile_width),sf::Color::Green);
                        _vertices[3] = sf::Vertex(sf::Vector2f(x*tile_width,y*tile_width+tile_width),sf::Color::Green);

                        primatives.push_back(debug::DebugPrimavtive(_vertices));
                    }
                }
            }
            return debug::Debug(primatives);
        }
    };
}
int main(){


    using namespace sf;
    using namespace std;
    using namespace nav;
    using namespace debug;
    using namespace collision_map;

    RenderWindow window(VideoMode(1600,1200),"SFML");
    CircleShape shape(100.f);
    shape.setFillColor(Color::Green);


    Clock clock;

    Font font;
    if(!font.loadFromFile("font.ttf")){
        return 1;
    }

    Text text;

    text.setFont(font);
    text.setCharacterSize(24);
    text.setFillColor(Color::Red);

    text.setStyle(Text::Bold);

    Mesh mesh;

    Map map("map.png",40);

    map.generateNavMesh(mesh);
    auto debug = map.buildDebug();

    auto mesh_debug =  mesh.buildDebug();


    auto elapsed = clock.restart();
    auto fps = 1.0/elapsed.asMilliseconds() * 1000;
    auto fps_str = to_string(fps);

    int count = 0;
    while (window.isOpen()){

        elapsed = clock.restart();
        fps = 1.0/elapsed.asMilliseconds() * 1000;

        fps_str = to_string(fps);

        text.setString("FPS:"+fps_str);
        text.setPosition(window.getSize().x-text.getLocalBounds().width,0);
        Event event;
        while(window.pollEvent(event)){
            if(event.type == Event::Closed)
                window.close();
            if(event.type == Event::KeyReleased){
                if(event.key.code == Keyboard::Return){
                    map.generateNavMesh(mesh);
                    debug = map.buildDebug();
                    mesh_debug =  mesh.buildDebug();
                    std::cout << "count:" << ++count <<std::endl;
                }
            }
        }

        window.clear();
        window.draw(text);
        window.draw(debug);
        window.draw(mesh_debug);
        window.display();
    }
}
