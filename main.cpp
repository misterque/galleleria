#include <string>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
#include <vector>
#include "jitter.h"

#include <libxml/parser.h>
#include <libxml/tree.h>



#define PI 3.14159
#define ACSIZE 1

using namespace std;
    
#ifdef _WIN32 
#undef main  
#include <stdlib.h>
#endif

int LoadTexture(string filename)
{
    SDL_Surface *bitmap, *conv;
    GLuint texture[1];

    bitmap = IMG_Load(filename.data());
    
    if (!bitmap) return -1;
    
    conv = SDL_CreateRGBSurface(SDL_SWSURFACE, bitmap->w, bitmap->h, 32,
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
            0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    #else
            0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    #endif
    SDL_BlitSurface(bitmap, 0, conv, 0);

    glGenTextures(1, &texture[0]);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  
    glPixelStorei(GL_UNPACK_ROW_LENGTH, conv->pitch / conv->format->BytesPerPixel);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, conv->w, conv->h, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, conv->pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    
    SDL_FreeSurface(bitmap);

    return texture[0];
}
void accFrustum(GLdouble left, GLdouble right, GLdouble bottom,
    GLdouble top, GLdouble near, GLdouble far, GLdouble pixdx, 
    GLdouble pixdy, GLdouble eyedx, GLdouble eyedy, 
    GLdouble focus)
{
    GLdouble xwsize, ywsize; 
    GLdouble dx, dy;
    GLint viewport[4];

    glGetIntegerv (GL_VIEWPORT, viewport);

    xwsize = right - left;
    ywsize = top - bottom;
    dx = -(pixdx*xwsize/(GLdouble) viewport[2] + 
            eyedx*near/focus);
    dy = -(pixdy*ywsize/(GLdouble) viewport[3] + 
            eyedy*near/focus);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum (left + dx, right + dx, bottom + dy, top + dy, 
        near, far);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef (-eyedx, -eyedy, 0.0);
}

void accPerspective(GLdouble fovy, GLdouble aspect, 
    GLdouble near, GLdouble far, GLdouble pixdx, GLdouble pixdy, 
    GLdouble eyedx, GLdouble eyedy, GLdouble focus)
{
    GLdouble fov2,left,right,bottom,top;
    fov2 = ((fovy*PI) / 180.0) / 2.0;

    top = near / (cos(fov2) / sin(fov2));
    bottom = -top;
    right = top * aspect;
    left = -right;

    accFrustum (left, right, bottom, top, near, far,
        pixdx, pixdy, eyedx, eyedy, focus);
}

void DrawAlbum(int texture, float x, float y, float z, float r, float a){
  glPushMatrix();
  glTranslatef(x,y,z);
  glRotatef(r, 0, 1, 0);
 
  glEnable(GL_TEXTURE_2D);
  if (texture >= 0) glBindTexture(GL_TEXTURE_2D, texture);

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glAlphaFunc(GL_ALWAYS, 0.0f);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
    glColor4f(1,1,1,a);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(-0.5,1.1, 0);
    glTexCoord2f(0.0f, 1.0f);   glVertex3f(-0.5,0.1, 0);
    glTexCoord2f(1.0f, 1.0f);   glVertex3f(0.5,0.1, 0);
    glTexCoord2f(1.0f, 0.0f);   glVertex3f(0.5,1.1, 0);
  glEnd();


  glBegin(GL_QUADS);
    glColor4f(0,0,0,a);
    glTexCoord2f(0.0f, 0.0f);   glVertex3f(-0.5,-1.1, 0);
    glTexCoord2f(1.0f, 0.0f);   glVertex3f(0.5,-1.1, 0);
    
    glColor4f(0.5f,0.5f,0.5f,a);
    glTexCoord2f(1.0f, 1.0f);   glVertex3f(0.5,-0.1, 0);
    glTexCoord2f(0.0f, 1.0f);   glVertex3f(-0.5,-0.1, 0);
  glEnd();

  glPopMatrix(); 
	

}

struct album {
  string artist, album, playlist;
  GLuint front_id, back_id;
	
};

int clapp_int(int a, int min, int max){
  while( a<min || a>max) {
    if (a<min) a+= max-min+1;
    if (a>max) a-= max-min+1;
  }
	
}

   xmlDoc *doc = NULL;
string xml_get_string(xmlNode *cur_node){
  xmlChar *key;  
  key = xmlNodeListGetString(doc, cur_node->xmlChildrenNode, 1);
  string Str =  string(reinterpret_cast<char*> (key));      
  xmlFree(key);
  return Str;
}

  vector<album> albums;

int xml_parse_album(xmlNode *node){
  xmlNode *cur_node = NULL;
  album new_album;
  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"artist")))        new_album.artist    = xml_get_string(cur_node);
      if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"album_name")))    new_album.album     = xml_get_string(cur_node);
      if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"playlist")))      new_album.playlist  = xml_get_string(cur_node);
      if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"front")))         new_album.front_id  = LoadTexture("./covers/"+ xml_get_string(cur_node) );
      if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"back")))          new_album.back_id  = LoadTexture("./covers/"+ xml_get_string(cur_node) );

    }
  }
  cout<<"loaded: "<<new_album.artist<<" - "<<new_album.album<<endl;
  albums.push_back(new_album);
}

  
void xml_parse_albums(xmlNode *node){
  xmlNode *cur_node = NULL;
  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"album"))) xml_parse_album(cur_node->children);
  
    }
  }
}   


void loadalbums() {
   xmlNode *root_element = NULL;


    /*parse the file and get the DOM */
    doc = xmlReadFile("albums.xml", NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file");
    }

    /*Get the root element node */
    root_element = xmlDocGetRootElement(doc);

    xml_parse_albums(root_element->children);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();



}

int main (int argc, char** argv) 
{

  bool done = false;
  
  
  SDL_Event event;
  SDL_Surface *screen;

  SDL_Init ( SDL_INIT_TIMER || SDL_INIT_VIDEO );

  screen = SDL_SetVideoMode(1024, 768, 16, SDL_OPENGL | SDL_RESIZABLE );
  SDL_WM_SetCaption("Galleleria", NULL); 
  
  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 5 );
  SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 5 );
  glClearDepth(1.0f);// Depth Buffer Setup
  glEnable(GL_DEPTH_TEST);// Enables Depth Testing
  glDepthFunc(GL_LEQUAL);// The Type Of Depth Test To Do

  glViewport(0, 0, screen->w, screen->h);
  glEnable(GL_TEXTURE_2D);
  
  glEnable(GL_BLEND);// Enable Blending       (disable alpha testing)
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glShadeModel(GL_SMOOTH);// Enables Smooth Shading
  gluPerspective(45.0f,(GLfloat)640/(GLfloat)480,0.1f,100.0f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

				    
  

  loadalbums();



  float rot=0;
  int select=3;
 while (!done)
  {

    while(SDL_PollEvent(&event)) {
      switch(event.type)
      {
        case SDL_KEYDOWN:
          if (event.key.keysym.sym==SDLK_ESCAPE) done=true;
          if (event.key.keysym.sym==SDLK_RIGHT) select--;                 // quit
          if (event.key.keysym.sym==SDLK_LEFT)  select++;                 // quit
          if (event.key.keysym.sym==SDLK_SPACE) {
	    string play = "xmms ./albums/" + albums[select].playlist + "&"; 
            system("xmms -Soff &");
            system(play.data());
	  }
	break;
	case SDL_VIDEORESIZE:
          SDL_FreeSurface(screen);
          screen = SDL_SetVideoMode(event.resize.w,
			  	    event.resize.h,
				    16, SDL_OPENGL | SDL_RESIZABLE );
          glViewport(0, 0, screen->w, screen->h);

	break;
			    
      };
    }
	       
    select = clapp_int(select, 0, albums.size()-1); 
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClearAccum(0.0, 0.0, 0.0, 0.0);

    glClear(GL_ACCUM_BUFFER_BIT);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    GLint viewport[4];
    int jitter;

    glGetIntegerv (GL_VIEWPORT, viewport);
    
    //glClear(GL_ACCUM_BUFFER_BIT);
    
    for (jitter = 0; jitter < ACSIZE; jitter++) {
      //  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        accPerspective (50.0, 
            (GLdouble) viewport[2]/(GLdouble) viewport[3], 
            1.0, 15.0, j8[jitter].x, j8[jitter].y,
            0.0, 0.0, 1.0);
      glTranslatef(0,-1.3,0);
      glRotatef(20, 1, 0, 0);
      float a;
      a=0.8/ACSIZE;
      
      DrawAlbum(albums[clapp_int(select-3,0,albums.size()-1)].front_id, -1.00, 0, -3, 90,     a/5);
      DrawAlbum(albums[clapp_int(select-2,0,albums.size()-1)].front_id, -0.75, 0, -3, 90,     a/2);
      DrawAlbum(albums[clapp_int(select-1,0,albums.size()-1)].front_id, -0.50, 0, -3, 90,     a);

      
      DrawAlbum(albums[clapp_int(select+3,0,albums.size()-1)].back_id, 1.00, 0, -3, -90,     a/5);
      DrawAlbum(albums[clapp_int(select+2,0,albums.size()-1)].back_id, 0.75, 0, -3, -90,     a/2);
      DrawAlbum(albums[clapp_int(select+1,0,albums.size()-1)].back_id, 0.50, 0, -3, -90,     a);
      
      
      DrawAlbum(albums[select].front_id, 0, 0, -2, rot,     a);
      DrawAlbum(albums[select].back_id,  0, 0, -2, rot+180, a);
      
      // glAccum(GL_ADD, 1.0/ACSIZE);
    }
    //glAccum(GL_RETURN, 1.0);
    
    SDL_GL_SwapBuffers( );
    SDL_Delay(20); 
    rot+=1;
  }
  // quit 'em
  SDL_Quit();
  return 0;

}



