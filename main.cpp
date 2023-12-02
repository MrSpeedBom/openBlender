#include<windows.h>
#include<GL/glu.h>
#include<GL/glut.h>
#include <vector>
#include <fstream>
#include <string>
#include <array>
#include <map>
#include <chrono>
#include <cstdint>
uint64_t last_time=0;
#include "Timer.h"

#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;
unsigned int texlen=0;
unsigned int stb_maketex(string file){
    cout<<"loading:"<<file<<endl;
int width, height, nrChannels;
glGenTextures(1, &texlen);
glBindTexture(GL_TEXTURE_2D, texlen);
// set the texture wrapping/filtering options (on currently bound texture)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// load and generate the texture
unsigned char *data = stbi_load(file.data(), &width, &height,&nrChannels, 0);
if(data)
{
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,GL_UNSIGNED_BYTE, data);
}
else
{
std::cout << "Failed to load texture" << std::endl;
}
stbi_image_free(data);
return texlen++;
}
string getdir(string dir)
{
    for(int i=dir.length()-1; i>=0; i--)
    {
        if(dir[i]=='/')
            return dir.substr(0,i+1);
    }
    return "";
}
void split(string s,string& l,string& r,char c)
{

    l=s;
    r="0";
    for(int i=0; i<s.length(); i++)
    {
        if(s[i]==c)
        {
            l=s.substr(0,i);
            r=s.substr(i+1,s.length()-i-1);
            break;
        }
    }

}
//c++1z
#include <math.h>
class point
{
public:
    float x,y,z;
    point() {}
    point(float gx,float gy,float gz)
    {
        x=gx;
        y=gy;
        z=gz;
    }
    float norm()
    {
        return sqrt(x*x+y*y+z*z);
    }
    point operator+(point b)
    {
        point res(x+b.x,y+b.y,z+b.z);
        return res;
    }
    point operator-(point b)
    {
        point res= {x-b.x,y-b.y,z-b.z};
        return res;
    }
    point operator*(float b)
    {
        point res=*this;
        res.x*=b;
        res.y*=b;
        res.z*=b;
        return res;
    }
    point operator-()
    {
        return point(-x,-y,-z);
    }
};
class Material
{
public:
    Material() {}
    Material(string n)
    {
        name=n;
    }
    string name;
    bool textured=0;
    GLuint tex=0;
    point color= {1,1,1};
};
class Face
{
public:
    vector<int> pid,tpid;
    int mtl=0;
};
class Group
{
public:
    Group() {}
    Group(string n)
    {
        name=n;
    }
    string name;
    bool rotation=0;
    point x,y;
    float lastt=-1;
    float rotspeed=0;
    bool farward;
    bool bound=0;
    float total=0;
    float boundl=-acos(-1)/2,boundr=acos(-1)/2;
    int first_face=-1,last_face;
    int first_point=-1,last_point;
};

vector<point> p;
vector<pair<float,float>> tp;
vector<Material> material;
vector<Face> face;
vector<Group> group;


void applyRotation(string who,string aroundwho,float speed=1,bool bound=0){
int id1=-1,id2=-1;
for(int i=0;i<group.size();i++){
    if(group[i].name==who)id1=i;
    if(group[i].name==aroundwho)id2=i;
}
if(id1==-1||id2==-1){
    cout<<"one of the objects was not foudn!\n";
}else{
    group[id1].rotation=1;
    group[id1].x=p[group[id2].first_point];
    group[id1].y=p[group[id2].first_point+1];
    group[id1].rotspeed=speed;
    cout<<group[id1].rotspeed<<endl;
    group[id1].bound=bound;
}

}
#define mat4x4 array<array<float,4>,4>


mat4x4 getrotation(float x,point vec)
{
    float g=vec.norm();
    vec={vec.x/g,vec.y/g,vec.z/g};
    return {
        cos(x)+vec.x*vec.x*(1-cos(x)),vec.x*vec.y*(1-cos(x))-vec.z*sin(x),vec.x*vec.z*(1-cos(x))+vec.y*sin(x),0,
        vec.y*vec.x*(1-cos(x))+vec.z*sin(x),cos(x)+vec.y*vec.y*(1-cos(x)),vec.y*vec.z*(1-cos(x))-vec.x*sin(x),0,
        vec.z*vec.x*(1-cos(x))-vec.y*sin(x),vec.z*vec.y*(1-cos(x))+vec.x*sin(x),cos(x)+vec.z*vec.z*(1-cos(x)),0,
        0,0,0,1
    };
}
mat4x4 getshift(point sh) {
    return {
        1,0,0,sh.x,
        0,1,0,sh.y,
        0,0,1,sh.z,
        0,0,0,1
    };
}
point mul(mat4x4 m,point p)
{
    point res;
    res.x=m[0][0]*p.x+m[0][1]*p.y+m[0][2]*p.z+m[0][3];
    res.y=m[1][0]*p.x+m[1][1]*p.y+m[1][2]*p.z+m[1][3];
    res.z=m[2][0]*p.x+m[2][1]*p.y+m[2][2]*p.z+m[2][3];
    float w=m[3][0]*p.x+m[3][1]*p.y+m[3][2]*p.z+m[3][3];
    res={res.x/w,res.y/w,res.z/w};
    return res;
}

void Rotate(Group &g){
    point vec=g.y-g.x;
    float tt=Timer::current_time();
    if(g.lastt==-1){g.lastt=tt;return;}
    for(int i=g.first_point;i!=-1&&i<=g.last_point;i++){
        p[i]=p[i]-g.x;
        p[i]=mul(getrotation(g.rotspeed*(tt-g.lastt),vec),p[i]);
        p[i]=p[i]+g.x;
    }
    g.lastt=tt;
}

map<string,int> loadMaterials(string filename)
{
    cout<<"loading materials form :"+filename<<endl;
    material.push_back(Material());//default material
    map<string,int> res;
    string got;
    ifstream in(filename);
    if(!in)
    {
        cout<<"file :\""+filename+"\" not found"<<endl;
        return res;
    }
    string line,get="";
    int g;
    vector<pair<int,int>> words;

    while(getline(in,line))
    {
        line+=" ";
        for(int i=0,j=0; i+j<=line.length();)
        {
            if(line[i+j]==' ')
            {
                if(j==i)
                {
                    i++;
                    j=0;
                }
                else
                {
                    words.push_back({i,j});

                    i+=j+1;
                    j=0;
                }
            }
            else
            {
                j++;
            }

        }

        if(words.size()==0)
            continue;
        else if(line.substr(words[0].first,words[0].second)=="newmtl")
        {
            g=material.size();
            res[line.substr(words[1].first,words[1].second)]=g;
            material.push_back(Material{line.substr(words[1].first,words[1].second)});
        }
        else if(line.substr(words[0].first,words[0].second)=="Kd"||line.substr(words[0].first,words[0].second)=="kd")
        {
            material[g].color=point(atof(line.substr(words[1].first,words[1].second).data()),
                                    atof(line.substr(words[2].first,words[2].second).data()),
                                    atof(line.substr(words[3].first,words[3].second).data()));
        }else if(line.substr(words[0].first,words[0].second)=="map_Kd"){
            material[g].textured=1;
            material[g].tex=stb_maketex(getdir(filename)+line.substr(words[1].first,words[1].second));
            cout<<material[g].tex<<"<<"<<endl;
        }


        words.clear();
    }
    cout<<"materials loaded successfully!\n ";

    return res;
}
void loadFile(string fileName,point scale=point(1,1,1),point shift=point(0,0,0)) //only one file
{
    int usedmtl=0;
    int v=0,f=0;
    p.clear();
    face.clear();
    map<string,int> getmat;
    string dir=getdir(fileName);
    ifstream in(fileName);
    if(!in)
    {
        cout<<"file :\""+fileName+"\" not found"<<endl;
        return;
    }
    string line,get="",l,r;
    int g;
    vector<pair<int,int>> words;

    while(getline(in,line))
    {
        line+=" ";
        for(int i=0,j=0; i+j<=line.length();)
        {
            if(line[i+j]==' ')
            {
                if(j==0)
                {
                    i++;
                    j=0;
                }
                else
                {
                    words.push_back({i,j});

                    i+=j+1;
                    j=0;
                }
            }
            else
            {
                j++;
            }

        }
        if(words.size()==0)
            continue;//this is important
        if(line.substr(words[0].first,words[0].second)=="v")
        {
            v++;
            if(group[group.size()-1].first_point==-1)
            {
                group[group.size()-1].first_point=group[group.size()-1].last_point=p.size();
            }
            else
            {
                group[group.size()-1].last_point=p.size();
            }
            p.push_back(point(atof(line.substr(words[1].first,words[1].second).data()),
                              atof(line.substr(words[2].first,words[2].second).data()),
                              atof(line.substr(words[3].first,words[3].second).data())
                             ));
        }
        else if(line.substr(words[0].first,words[0].second)=="vt")
        {
            tp.push_back({atof(line.substr(words[1].first,words[1].second).data()),atof(line.substr(words[2].first,words[2].second).data())});
        }
        else if(line.substr(words[0].first,words[0].second)=="f")
        {
            f++;
            if(group[group.size()-1].first_face==-1)
            {
                group[group.size()-1].first_face=face.size();
                group[group.size()-1].last_face=face.size();
            }
            else
            {
                group[group.size()-1].last_face=face.size();
            }
            g=face.size();
            face.push_back({});
            face[g].mtl=usedmtl;

            for(int i=1; i<words.size(); i++)
            {
                split(line.substr(words[i].first,words[i].second),l,r,'/');
                face[g].pid.push_back(atoi(l.data())-1);
                face[g].tpid.push_back(atoi(r.data())-1);
            }
        }
        else if(line.substr(words[0].first,words[0].second)=="mtllib")
        {
            getmat=loadMaterials(dir+line.substr(words[1].first,words[1].second));
        }
        else if(line.substr(words[0].first,words[0].second)=="o")
        {
            cout<<"loading object:"<<line.substr(words[1].first,words[1].second) <<endl;
            group.push_back(Group{line.substr(words[1].first,words[1].second)});
        }
        else if(line.substr(words[0].first,words[0].second)=="usemtl")
        {
            usedmtl=getmat[line.substr(words[1].first,words[1].second)];
        }
        words.clear();
    }
    cout<<"total of "<<v<<"  vertices and "<<f<<" faces\n";
    for(int i=0;i<tp.size();i++){
        if(tp[i].first<0)tp[i].first++;
        if(tp[i].second<0)tp[i].second++;
        tp[i].second=1-tp[i].second;
    }
}
void draw()
{
    for(int i=0;i<group.size();i++){
        if(group[i].rotation){
            Rotate(group[i]);
        }
    }
    for(Group g : group)
    {

        glPushMatrix();
        for(int i=g.first_face; i!=-1&&i<=g.last_face; i++)
        {
            if(material[face[i].mtl].textured){
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D,material[face[i].mtl].tex);
            }
            else
            glColor3f(material[face[i].mtl].color.x,material[face[i].mtl].color.y,material[face[i].mtl].color.z);

            glBegin(GL_POLYGON);
            for(int j=0; j<face[i].pid.size(); j++)
            {
                if(material[face[i].mtl].textured){
                    glTexCoord2f(tp[face[i].tpid[j]].first,tp[face[i].tpid[j]].second);
                }
                glVertex3f(p[face[i].pid[j]].x,p[face[i].pid[j]].y,p[face[i].pid[j]].z);
            }
            if(material[face[i].mtl].textured){
                glDisable(GL_TEXTURE_2D);
            }

            glPopMatrix();
            glEnd();

        }
    }

}
void updatePosition();


point position(-3,0,0);
float angle1=0,angle2=0;

float speed=0.1;
point getVector(float xx,float yy,int bb=0)
{
    if(bb==1)
    {
        xx-=3.141592653589/2;
        return point{cos(xx),sin(xx),0};
    }
    if(bb==2)
        yy+=3.141592653589/2;
    return point{cos(xx)*cos(yy),sin(xx)*cos(yy),sin(yy)};
}

void MyInit()
{
    glClearColor(0,0,0,1);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1,1,-1,1,1,30);
    glMatrixMode(GL_MODELVIEW);
}

void idleFunction()
{
    glutPostRedisplay();
}
void Draw()
{
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // Clear color and depth buffers
    glMatrixMode(GL_MODELVIEW);     // To operate on model-view matrix
    point cen=getVector(angle1,angle2,0)+position;
    point up=getVector(angle1,angle2,2);
    glLoadIdentity();
    gluLookAt(position.x,position.y,position.z,cen.x,cen.y,cen.z,up.x,up.y,up.z);
    draw();
    glutSwapBuffers();
}
static void specialKey(int key,int xx,int yy)
{
    switch(key)
    {

    case GLUT_KEY_LEFT:
    {
        angle1+=speed;
        break;
    }
    case GLUT_KEY_UP:
    {
        angle2+=speed;
        break;
    }
    case GLUT_KEY_RIGHT:
    {
        angle1-=speed;
        break;
    }
    case GLUT_KEY_DOWN:
    {
        angle2-=speed;
        break;
    }
    }
}
point vecrl,vecfb;
void Key(unsigned char ch,int x,int y)
{

    vecrl=getVector(angle1,angle2,1)*speed;
    vecfb=getVector(angle1+acos(-1)/2,angle2,1)*speed;
    switch(ch)
    {
    case 's':
    {

        position=position-vecfb;
        break;
    }
    case 'w':
    {
        position=position+vecfb;
        break;
    }
    case 'a':
    {
        position=position-vecrl;
        break;
    }
    case 'd':
    {
        position=position+vecrl;
        break;
    }
    case 'z':
    {
        position.z+=speed;
        break;
    }
    case 'x':
    {
        position.z-=speed;
        break;
    }
    }
}

const int W=800,H=600;
int main(int argc,char* argv[ ])
{
    cout<<"camera direction left right up down arrow"<<endl;
    cout<<"camera move with a s d w z x"<<endl;
    glutInit(&argc,argv);
    glutInitWindowSize(W,H);
    glutInitWindowPosition(100,150);
    glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
    glutCreateWindow("Color Cube with Camera");
    MyInit();
    glutIdleFunc(idleFunction);
    glutDisplayFunc(Draw);
    glutKeyboardFunc(Key);
    glutSpecialFunc(specialKey);

    loadFile("ship.obj");
    applyRotation("ship","diameter",1,1);
    Timer::start();
    glutMainLoop();

    return 0;
}
